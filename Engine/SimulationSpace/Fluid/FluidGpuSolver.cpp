/*============================================================================================================================================
                                                          FLUIDGPUSOLVER.CPP
============================================================================================================================================*/
// 🧩 Stage D (Slices S15-S16) 1:1 3D Eulerian + Sparse Infinite GPU fluid solver uniform ABI & bit-accurate CPU reference mirrors
// Reference: Experimental/FluidEditor/Engine/FluidShaders.js L532-L1076 & FluidSolver.js L410-L810

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/SimulationSpace/Fluid/FluidGpuSolver.h"

#include <algorithm>
#include <cmath>

namespace Frontier::SimulationSpace::Fluid
{

namespace
{

[[nodiscard]] float ClampF(const float x, const float lo, const float hi) noexcept { return std::clamp(x, lo, hi); }
[[nodiscard]] float FractF(const float x) noexcept { return x - std::floor(x); }
[[nodiscard]] float LerpF(const float a, const float b, const float t) noexcept { return a + (b - a) * t; }
[[nodiscard]] float StepF(const float edge, const float x) noexcept { return (x < edge) ? 0.0f : 1.0f; }
[[nodiscard]] float SmoothStepF(const float e0, const float e1, const float x) noexcept
{
    const float denom = e1 - e0;
    const float t = (std::fabs(denom) > 1.0e-9f) ? ClampF((x - e0) / denom, 0.0f, 1.0f) : 0.0f;
    return t * t * (3.0f - 2.0f * t);
}

[[nodiscard]] std::array<float, 4> Lerp4(
    const std::array<float, 4>& a,
    const std::array<float, 4>& b,
    const float t) noexcept
{
    return {
        LerpF(a[0], b[0], t),
        LerpF(a[1], b[1], t),
        LerpF(a[2], b[2], t),
        LerpF(a[3], b[3], t)
    };
}

} // namespace

//------------------------------------------------------------------------------------------------------------------------
//                                              CPU 3D VOLUME BUFFER
//------------------------------------------------------------------------------------------------------------------------

void FluidVolumeGridCpu::Resize(const uint32_t w, const uint32_t h, const uint32_t d)
{
    Width  = w;
    Height = h;
    Depth  = d;
    Cells.assign(static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(d), { 0.0f, 0.0f, 0.0f, 0.0f });
}

void FluidVolumeGridCpu::Clear() noexcept
{
    std::fill(Cells.begin(), Cells.end(), std::array<float, 4> { 0.0f, 0.0f, 0.0f, 0.0f });
}

size_t FluidVolumeGridCpu::LinearIndex(const int32_t x, const int32_t y, const int32_t z) const noexcept
{
    return (static_cast<size_t>(z) * static_cast<size_t>(Height) + static_cast<size_t>(y)) * static_cast<size_t>(Width)
         + static_cast<size_t>(x);
}

std::array<float, 4> FluidVolumeGridCpu::FetchClamped(const int32_t x, const int32_t y, const int32_t z) const noexcept
{
    if (Cells.empty() || Width == 0u || Height == 0u || Depth == 0u) return { 0.0f, 0.0f, 0.0f, 0.0f };
    const int32_t cx = std::clamp(x, 0, static_cast<int32_t>(Width)  - 1);
    const int32_t cy = std::clamp(y, 0, static_cast<int32_t>(Height) - 1);
    const int32_t cz = std::clamp(z, 0, static_cast<int32_t>(Depth)  - 1);
    return Cells[LinearIndex(cx, cy, cz)];
}

std::array<float, 4> FluidVolumeGridCpu::SampleTrilinear(const float cx, const float cy, const float cz) const noexcept
{
    if (Cells.empty() || Width == 0u || Height == 0u || Depth == 0u) return { 0.0f, 0.0f, 0.0f, 0.0f };
    const float px = ClampF(cx - 0.5f, 0.0f, static_cast<float>(Width)  - 1.0f);
    const float py = ClampF(cy - 0.5f, 0.0f, static_cast<float>(Height) - 1.0f);
    const float pz = ClampF(cz - 0.5f, 0.0f, static_cast<float>(Depth)  - 1.0f);

    const int32_t x0 = static_cast<int32_t>(std::floor(px));
    const int32_t y0 = static_cast<int32_t>(std::floor(py));
    const int32_t z0 = static_cast<int32_t>(std::floor(pz));
    const float fx = px - static_cast<float>(x0);
    const float fy = py - static_cast<float>(y0);
    const float fz = pz - static_cast<float>(z0);

    const auto x00 = Lerp4(FetchClamped(x0, y0,     z0),     FetchClamped(x0 + 1, y0,     z0),     fx);
    const auto x10 = Lerp4(FetchClamped(x0, y0 + 1, z0),     FetchClamped(x0 + 1, y0 + 1, z0),     fx);
    const auto x01 = Lerp4(FetchClamped(x0, y0,     z0 + 1), FetchClamped(x0 + 1, y0,     z0 + 1), fx);
    const auto x11 = Lerp4(FetchClamped(x0, y0 + 1, z0 + 1), FetchClamped(x0 + 1, y0 + 1, z0 + 1), fx);
    return Lerp4(Lerp4(x00, x10, fy), Lerp4(x01, x11, fy), fz);
}

//------------------------------------------------------------------------------------------------------------------------
//                                        SLICE S15 UNIFORM BUILDERS & KERNELS
//------------------------------------------------------------------------------------------------------------------------

FluidInjectUniformBlock BuildFluidInjectUniforms(
    const FluidVolumeUniformBlock& volume,
    const uint32_t mode,
    const std::span<const FluidReleaseSourceParameters> sources,
    const float deltaTimeSeconds,
    const float simulationTimeSeconds) noexcept
{
    FluidInjectUniformBlock u {};
    u.Volume = volume;
    uint32_t packed = 0u;
    for (const FluidReleaseSourceParameters& s : sources)
    {
        if (!s.Enabled || packed >= kMaximumReleaseSources) continue;
        if (s.EnvelopeWeight <= 0.0f) continue;
        u.SourcePosRadius[packed]     = { s.PositionMeters[0], s.PositionMeters[1], s.PositionMeters[2], s.RadiusMeters };
        u.SourceVelDensity[packed]    = { s.VelocityMps[0], s.VelocityMps[1], s.VelocityMps[2], s.Density * s.EnvelopeWeight };
        u.SourceTempFuelShape[packed] = { s.Temperature * s.EnvelopeWeight, s.Fuel * s.EnvelopeWeight, s.Jitter, static_cast<float>(s.Shape) };
        u.SourceOrientPulse[packed]   = { s.Orientation[0], s.Orientation[1], s.Orientation[2], s.PulseSeconds };
        ++packed;
    }
    u.InjectTiming = {
        static_cast<float>(mode),
        static_cast<float>(packed),
        deltaTimeSeconds,
        simulationTimeSeconds
    };
    return u;
}

float EvaluateReleaseSourceMaskCpu(
    const FluidInjectUniformBlock& uniforms,
    const int32_t sourceIndex,
    const std::array<float, 3>& worldPos) noexcept
{
    if (sourceIndex < 0 || sourceIndex >= static_cast<int32_t>(kMaximumReleaseSources)) return 0.0f;
    const auto& a = uniforms.SourcePosRadius[static_cast<size_t>(sourceIndex)];
    const auto& c = uniforms.SourceTempFuelShape[static_cast<size_t>(sourceIndex)];
    const auto& d = uniforms.SourceOrientPulse[static_cast<size_t>(sourceIndex)];

    const float cellSize = std::max(uniforms.Volume.GridCentreCellSize[3], 1.0e-5f);
    const float rx = worldPos[0] - a[0];
    const float ry = worldPos[1] - a[1];
    const float rz = worldPos[2] - a[2];
    const float radius = std::max(a[3], cellSize);
    const int32_t shape = static_cast<int32_t>(c[3] + 0.5f);
    const float edge = cellSize * 1.5f;
    float mask = 0.0f;

    if (shape == 0)
    {
        const float dist = std::sqrt(rx * rx + ry * ry + rz * rz);
        mask = 1.0f - SmoothStepF(radius - edge, radius, dist);
    }
    else if (shape == 1)
    {
        const float nLen = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        const float nx = (nLen > 1.0e-5f) ? (d[0] / nLen) : 0.0f;
        const float ny = (nLen > 1.0e-5f) ? (d[1] / nLen) : 1.0f;
        const float nz = (nLen > 1.0e-5f) ? (d[2] / nLen) : 0.0f;
        const float proj = rx * nx + ry * ny + rz * nz;
        const float along = std::fabs(proj);
        const float px = rx - nx * proj, py = ry - ny * proj, pz = rz - nz * proj;
        const float radial = std::sqrt(px * px + py * py + pz * pz);
        mask = (1.0f - SmoothStepF(radius - edge, radius, radial))
             * (1.0f - SmoothStepF(edge * 0.6f, edge * 1.4f, along));
    }
    else
    {
        const float qx = std::fabs(rx) - radius;
        const float qy = std::fabs(ry) - radius;
        const float qz = std::fabs(rz) - radius;
        const float boxDist = std::max({ qx, qy, qz });
        mask = 1.0f - SmoothStepF(-edge, 0.0f, boxDist);
    }

    const float simTime = uniforms.InjectTiming[3];
    const float jitter  = c[2];
    if (jitter > 0.0f)
    {
        const float scale = 2.2f / radius;
        const float n = Frontier::SimulationSpace::ForceFields::ValueNoise({
            worldPos[0] * scale,
            worldPos[1] * scale + simTime * 1.6f,
            worldPos[2] * scale + simTime * 0.9f
        });
        mask *= LerpF(1.0f, SmoothStepF(0.25f, 0.8f, n) * 1.4f, ClampF(jitter, 0.0f, 1.0f));
    }

    const float pulse = d[3];
    if (pulse > 0.0f)
    {
        const float phase = FractF(simTime / pulse);
        mask *= std::exp(-phase * 7.0f) * StepF(phase, 0.45f);
    }

    return ClampF(mask, 0.0f, 1.0f);
}

void RunFluidAdvectCpu(
    const FluidAdvectUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& source,
    FluidVolumeGridCpu& output)
{
    const uint32_t W = source.Width, H = source.Height, D = source.Depth;
    if (output.Width != W || output.Height != H || output.Depth != D) output.Resize(W, H, D);

    const float dt          = uniforms.AdvectParams[0];
    const float dissipation = uniforms.AdvectParams[1];
    const float direction   = uniforms.AdvectParams[2];
    const float invH        = 1.0f / std::max(uniforms.Volume.GridCentreCellSize[3], 1.0e-5f);

    for (uint32_t z = 0u; z < D; ++z)
    {
        for (uint32_t y = 0u; y < H; ++y)
        {
            for (uint32_t x = 0u; x < W; ++x)
            {
                const float cx = static_cast<float>(x) + 0.5f;
                const float cy = static_cast<float>(y) + 0.5f;
                const float cz = static_cast<float>(z) + 0.5f;
                const auto vel = velocity.FetchClamped(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(z));
                const float mx = cx - direction * 0.5f * dt * vel[0] * invH;
                const float my = cy - direction * 0.5f * dt * vel[1] * invH;
                const float mz = cz - direction * 0.5f * dt * vel[2] * invH;
                const auto midVel = velocity.SampleTrilinear(mx, my, mz);
                const float ox = cx - direction * dt * midVel[0] * invH;
                const float oy = cy - direction * dt * midVel[1] * invH;
                const float oz = cz - direction * dt * midVel[2] * invH;
                const auto sampled = source.SampleTrilinear(ox, oy, oz);
                output.Cells[output.LinearIndex(static_cast<int32_t>(x), static_cast<int32_t>(y), static_cast<int32_t>(z))] = {
                    sampled[0] * dissipation,
                    sampled[1] * dissipation,
                    sampled[2] * dissipation,
                    sampled[3] * dissipation
                };
            }
        }
    }
}

void RunFluidMacCormackCpu(
    const FluidMacCormackUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& source,
    const FluidVolumeGridCpu& forward,
    const FluidVolumeGridCpu& backward,
    FluidVolumeGridCpu& output)
{
    const uint32_t W = source.Width, H = source.Height, D = source.Depth;
    if (output.Width != W || output.Height != H || output.Depth != D) output.Resize(W, H, D);

    const float dt          = uniforms.MacCormackParams[0];
    const float dissipation = uniforms.MacCormackParams[1];
    const float invH        = 1.0f / std::max(uniforms.Volume.GridCentreCellSize[3], 1.0e-5f);
    const float maxX        = static_cast<float>(W) - 1.0f;
    const float maxY        = static_cast<float>(H) - 1.0f;
    const float maxZ        = static_cast<float>(D) - 1.0f;

    for (uint32_t z = 0u; z < D; ++z)
    {
        for (uint32_t y = 0u; y < H; ++y)
        {
            for (uint32_t x = 0u; x < W; ++x)
            {
                const int32_t ix = static_cast<int32_t>(x);
                const int32_t iy = static_cast<int32_t>(y);
                const int32_t iz = static_cast<int32_t>(z);
                const float cx = static_cast<float>(x) + 0.5f;
                const float cy = static_cast<float>(y) + 0.5f;
                const float cz = static_cast<float>(z) + 0.5f;

                const auto vel = velocity.FetchClamped(ix, iy, iz);
                const auto midVel = velocity.SampleTrilinear(
                    cx - 0.5f * dt * vel[0] * invH,
                    cy - 0.5f * dt * vel[1] * invH,
                    cz - 0.5f * dt * vel[2] * invH);
                const float ox = cx - dt * midVel[0] * invH;
                const float oy = cy - dt * midVel[1] * invH;
                const float oz = cz - dt * midVel[2] * invH;

                const auto fwd = forward.FetchClamped(ix, iy, iz);
                const auto bwd = backward.FetchClamped(ix, iy, iz);
                const auto src = source.FetchClamped(ix, iy, iz);
                std::array<float, 4> corrected {};
                for (size_t c = 0u; c < 4u; ++c)
                {
                    corrected[c] = fwd[c] + 0.5f * (src[c] * dissipation - bwd[c]);
                }

                const int32_t bx = static_cast<int32_t>(std::floor(ClampF(ox - 0.5f, 0.0f, maxX)));
                const int32_t by = static_cast<int32_t>(std::floor(ClampF(oy - 0.5f, 0.0f, maxY)));
                const int32_t bz = static_cast<int32_t>(std::floor(ClampF(oz - 0.5f, 0.0f, maxZ)));
                std::array<float, 4> lo {  1.0e9f,  1.0e9f,  1.0e9f,  1.0e9f };
                std::array<float, 4> hi { -1.0e9f, -1.0e9f, -1.0e9f, -1.0e9f };
                for (int32_t dz = 0; dz < 2; ++dz)
                {
                    for (int32_t dy = 0; dy < 2; ++dy)
                    {
                        for (int32_t dx = 0; dx < 2; ++dx)
                        {
                            const auto s = source.FetchClamped(bx + dx, by + dy, bz + dz);
                            for (size_t c = 0u; c < 4u; ++c)
                            {
                                const float sc = s[c] * dissipation;
                                lo[c] = std::min(lo[c], sc);
                                hi[c] = std::max(hi[c], sc);
                            }
                        }
                    }
                }

                bool overshoot = false;
                for (size_t c = 0u; c < 4u; ++c)
                {
                    if (corrected[c] < lo[c] || corrected[c] > hi[c]) { overshoot = true; break; }
                }

                const float speed   = std::sqrt(vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2]);
                const float courant = speed * dt * invH;
                const float blendW  = 1.0f - SmoothStepF(1.0f, 2.0f, courant);
                std::array<float, 4> result {};
                for (size_t c = 0u; c < 4u; ++c)
                {
                    const float val = overshoot ? fwd[c] : LerpF(fwd[c], corrected[c], blendW);
                    result[c] = ClampF(val, lo[c], hi[c]);
                }
                output.Cells[output.LinearIndex(ix, iy, iz)] = result;
            }
        }
    }
}

void AdvectFluidFieldMacCormackCpu(
    const FluidVolumeUniformBlock& volume,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& source,
    const float deltaTimeSeconds,
    const float dissipation,
    FluidVolumeGridCpu& scratchForward,
    FluidVolumeGridCpu& scratchBackward,
    FluidVolumeGridCpu& output)
{
    FluidAdvectUniformBlock fwdU { volume, { deltaTimeSeconds, dissipation,  1.0f, 0.0f } };
    RunFluidAdvectCpu(fwdU, velocity, source, scratchForward);

    FluidAdvectUniformBlock bwdU { volume, { deltaTimeSeconds, 1.0f, -1.0f, 0.0f } };
    RunFluidAdvectCpu(bwdU, velocity, scratchForward, scratchBackward);

    FluidMacCormackUniformBlock mcU { volume, { deltaTimeSeconds, dissipation, 0.0f, 0.0f } };
    RunFluidMacCormackCpu(mcU, velocity, source, scratchForward, scratchBackward, output);
}

void RunFluidInjectCpu(
    const FluidInjectUniformBlock& uniforms,
    const FluidVolumeGridCpu& target,
    FluidVolumeGridCpu& output)
{
    const uint32_t W = target.Width, H = target.Height, D = target.Depth;
    if (output.Width != W || output.Height != H || output.Depth != D) output.Resize(W, H, D);

    const int32_t mode  = static_cast<int32_t>(uniforms.InjectTiming[0] + 0.5f);
    const int32_t count = std::min(static_cast<int32_t>(uniforms.InjectTiming[1] + 0.5f), static_cast<int32_t>(kMaximumReleaseSources));
    const float dt      = uniforms.InjectTiming[2];
    const float blend   = std::min(1.0f, dt * 10.0f);

    for (uint32_t z = 0u; z < D; ++z)
    {
        for (uint32_t y = 0u; y < H; ++y)
        {
            for (uint32_t x = 0u; x < W; ++x)
            {
                const int32_t ix = static_cast<int32_t>(x);
                const int32_t iy = static_cast<int32_t>(y);
                const int32_t iz = static_cast<int32_t>(z);
                const auto world = WorldFromFluidCell(uniforms.Volume, {
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f,
                    static_cast<float>(z) + 0.5f
                });
                auto value = target.FetchClamped(ix, iy, iz);

                for (int32_t i = 0; i < count; ++i)
                {
                    const float mask = EvaluateReleaseSourceMaskCpu(uniforms, i, world);
                    if (mask <= 0.0f) continue;
                    const auto& b = uniforms.SourceVelDensity[static_cast<size_t>(i)];
                    const auto& c = uniforms.SourceTempFuelShape[static_cast<size_t>(i)];
                    if (mode == 0)
                    {
                        if (b[0] * b[0] + b[1] * b[1] + b[2] * b[2] > 0.0f)
                        {
                            const float w = mask * blend;
                            value[0] = LerpF(value[0], b[0], w);
                            value[1] = LerpF(value[1], b[1], w);
                            value[2] = LerpF(value[2], b[2], w);
                        }
                    }
                    else
                    {
                        value[0] = std::min(value[0] + mask * b[3] * dt * 6.0f, std::max(value[0], b[3] * 1.5f));
                        value[1] = LerpF(value[1], std::max(value[1], c[0]), mask * blend);
                        value[2] = std::min(value[2] + mask * c[1] * dt * 6.0f, std::max(value[2], c[1] * 1.5f));
                    }
                }
                output.Cells[output.LinearIndex(ix, iy, iz)] = value;
            }
        }
    }
}

void RunFluidForcesCpu(
    const FluidForcesUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& state,
    const std::span<const Frontier::SimulationSpace::ForceFields::ForceModelGpuRecord> forceRecords,
    FluidVolumeGridCpu& output)
{
    const uint32_t W = velocity.Width, H = velocity.Height, D = velocity.Depth;
    if (output.Width != W || output.Height != H || output.Depth != D) output.Resize(W, H, D);

    const float dt            = uniforms.ForceKinematics[0];
    const float buoyancy      = uniforms.ForceKinematics[1];
    const float gravity       = uniforms.ForceKinematics[2];
    const float ambientTemp   = uniforms.ForceKinematics[3];
    const float forceResponse = uniforms.ForceControl[0];
    const float simTime       = uniforms.ForceControl[1];

    for (uint32_t z = 0u; z < D; ++z)
    {
        for (uint32_t y = 0u; y < H; ++y)
        {
            for (uint32_t x = 0u; x < W; ++x)
            {
                const int32_t ix = static_cast<int32_t>(x);
                const int32_t iy = static_cast<int32_t>(y);
                const int32_t iz = static_cast<int32_t>(z);
                const auto world = WorldFromFluidCell(uniforms.Volume, {
                    static_cast<float>(x) + 0.5f,
                    static_cast<float>(y) + 0.5f,
                    static_cast<float>(z) + 0.5f
                });
                if (IsSolidAtFluidWorld(uniforms.Volume, world))
                {
                    output.Cells[output.LinearIndex(ix, iy, iz)] = { 0.0f, 0.0f, 0.0f, 0.0f };
                    continue;
                }

                const auto vel = velocity.FetchClamped(ix, iy, iz);
                const auto st  = state.FetchClamped(ix, iy, iz);
                const float temp = st[1] - ambientTemp;
                float ax = 0.0f;
                float ay = buoyancy * temp - gravity * st[0];
                float az = 0.0f;

                if (!forceRecords.empty())
                {
                    const auto gasForce = Frontier::SimulationSpace::ForceFields::EvaluateSaturatedGasForceField(
                        forceRecords,
                        { world[0], world[1], world[2] },
                        { vel[0], vel[1], vel[2] },
                        simTime,
                        forceResponse);
                    ax += gasForce[0];
                    ay += gasForce[1];
                    az += gasForce[2];
                }

                output.Cells[output.LinearIndex(ix, iy, iz)] = {
                    vel[0] + ax * dt,
                    vel[1] + ay * dt,
                    vel[2] + az * dt,
                    0.0f
                };
            }
        }
    }
}

} // namespace Frontier::SimulationSpace::Fluid
