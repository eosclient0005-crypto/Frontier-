/*============================================================================================================================================
                                                           FLUIDGPUSOLVER.H
============================================================================================================================================*/
// 🧩 Stage D (Slices S15-S16) 1:1 3D Eulerian + Sparse Infinite GPU fluid solver uniform ABI & bit-accurate CPU reference mirrors
// Reference: Experimental/FluidEditor/Engine/FluidShaders.js L532-L1076 & FluidSolver.js L410-L810

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/SimulationSpace/Fluid/FluidRegionTypes.h"
#include "Engine/SimulationSpace/Fluid/SparseTileBank.h"
#include "Engine/SimulationSpace/ForceFields/ForceFieldSampler.h"
#include "Engine/SimulationSpace/ForceFields/ForceModelTypes.h"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Frontier::SimulationSpace::Fluid
{

//------------------------------------------------------------------------------------------------------------------------
//                                                RELEASE SOURCE & GPU UNIFORMS
//------------------------------------------------------------------------------------------------------------------------

struct FluidReleaseSourceParameters
{
    std::string               SourceId       {};
    bool                      Enabled        = true;
    ReleaseSourceShapeOrdinal Shape          = ReleaseSourceShapeOrdinal::Sphere;
    std::array<float, 3>      PositionMeters { 0.0f, 0.0f, 0.0f };
    float                     RadiusMeters   = 0.35f;
    std::array<float, 3>      VelocityMps    { 0.0f, 2.0f, 0.0f };
    std::array<float, 3>      Orientation    { 0.0f, 1.0f, 0.0f };
    float                     Density        = 1.0f;
    float                     Temperature    = 2.0f;
    float                     Fuel           = 1.0f;
    float                     Jitter         = 0.2f;
    float                     PulseSeconds   = 0.0f;
    float                     EnvelopeWeight = 1.0f;
};

/// Matches FluidAdvectUniformBlock in FluidAdvectCompute.slang byte-for-byte (176 bytes).
struct alignas(16) FluidAdvectUniformBlock
{
    FluidVolumeUniformBlock Volume {};
    std::array<float, 4>    AdvectParams { 1.0f / 60.0f, 0.996f, 1.0f, 0.0f }; // dt, dissipation, direction (+1/-1), pad
};
static_assert(sizeof(FluidAdvectUniformBlock) == 176u, "FluidAdvectUniformBlock must be 176 bytes");

/// Matches FluidMacCormackUniformBlock in FluidMacCormackCompute.slang byte-for-byte (176 bytes).
struct alignas(16) FluidMacCormackUniformBlock
{
    FluidVolumeUniformBlock Volume {};
    std::array<float, 4>    MacCormackParams { 1.0f / 60.0f, 0.996f, 0.0f, 0.0f }; // dt, dissipation, pad, pad
};
static_assert(sizeof(FluidMacCormackUniformBlock) == 176u, "FluidMacCormackUniformBlock must be 176 bytes");

/// Matches FluidInjectUniformBlock in FluidInjectCompute.slang byte-for-byte (560 bytes).
struct alignas(16) FluidInjectUniformBlock
{
    FluidVolumeUniformBlock             Volume              {};
    std::array<float, 4>                InjectTiming        { 0.0f, 0.0f, 1.0f / 60.0f, 0.0f };
    std::array<std::array<float, 4>, 6> SourcePosRadius     {};
    std::array<std::array<float, 4>, 6> SourceVelDensity    {};
    std::array<std::array<float, 4>, 6> SourceTempFuelShape {};
    std::array<std::array<float, 4>, 6> SourceOrientPulse   {};
};
static_assert(sizeof(FluidInjectUniformBlock) == 560u, "FluidInjectUniformBlock must be 560 bytes");

/// Matches FluidForcesUniformBlock in FluidForcesCompute.slang byte-for-byte (192 bytes).
struct alignas(16) FluidForcesUniformBlock
{
    FluidVolumeUniformBlock Volume {};
    std::array<float, 4>    ForceKinematics { 1.0f / 60.0f, 1.2f, 0.25f, 0.0f }; // dt, buoyancy, gravity, ambientTemp
    std::array<float, 4>    ForceControl    { 1.0f, 0.0f, 0.0f, 0.0f };          // forceResponse, simTime, forceCount, pad
};
static_assert(sizeof(FluidForcesUniformBlock) == 192u, "FluidForcesUniformBlock must be 192 bytes");

//------------------------------------------------------------------------------------------------------------------------
//                                              CPU 3D VOLUME BUFFER
//------------------------------------------------------------------------------------------------------------------------

struct FluidVolumeGridCpu
{
    uint32_t                          Width  = 0u;
    uint32_t                          Height = 0u;
    uint32_t                          Depth  = 0u;
    std::vector<std::array<float, 4>> Cells  {};

    void Resize(uint32_t w, uint32_t h, uint32_t d);
    void Clear() noexcept;

    [[nodiscard]] size_t LinearIndex(int32_t x, int32_t y, int32_t z) const noexcept;
    [[nodiscard]] std::array<float, 4> FetchClamped(int32_t x, int32_t y, int32_t z) const noexcept;
    [[nodiscard]] std::array<float, 4> SampleTrilinear(float cx, float cy, float cz) const noexcept;
};

//------------------------------------------------------------------------------------------------------------------------
//                                        SLICE S15 UNIFORM BUILDERS & KERNELS
//------------------------------------------------------------------------------------------------------------------------

[[nodiscard]] FluidInjectUniformBlock BuildFluidInjectUniforms(
    const FluidVolumeUniformBlock& volume,
    uint32_t mode,
    std::span<const FluidReleaseSourceParameters> sources,
    float deltaTimeSeconds,
    float simulationTimeSeconds) noexcept;

/// Exact CPU mirror of FluidInjectCompute.slang::EvaluateReleaseSourceMask (Sphere/Disc/Box + jitter + pulse).
[[nodiscard]] float EvaluateReleaseSourceMaskCpu(
    const FluidInjectUniformBlock& uniforms,
    int32_t sourceIndex,
    const std::array<float, 3>& worldPos) noexcept;

/// Exact CPU mirror of FluidAdvectCompute.slang (RK2 forward +1 / reverse -1).
void RunFluidAdvectCpu(
    const FluidAdvectUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& source,
    FluidVolumeGridCpu& output);

/// Exact CPU mirror of FluidMacCormackCompute.slang (Selle 2008 overshoot-fallback + Courant [1..2] blend).
void RunFluidMacCormackCpu(
    const FluidMacCormackUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& source,
    const FluidVolumeGridCpu& forward,
    const FluidVolumeGridCpu& backward,
    FluidVolumeGridCpu& output);

/// Convenience 3-step Selle 2008 MacCormack advection helper (Forward -> Reverse -> Limiter Combine).
void AdvectFluidFieldMacCormackCpu(
    const FluidVolumeUniformBlock& volume,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& source,
    float deltaTimeSeconds,
    float dissipation,
    FluidVolumeGridCpu& scratchForward,
    FluidVolumeGridCpu& scratchBackward,
    FluidVolumeGridCpu& output);

/// Exact CPU mirror of FluidInjectCompute.slang (mode 0 velocity, mode 1 state).
void RunFluidInjectCpu(
    const FluidInjectUniformBlock& uniforms,
    const FluidVolumeGridCpu& target,
    FluidVolumeGridCpu& output);

/// Exact CPU mirror of FluidForcesCompute.slang (buoyancy + gravity + collider masking + velocity-saturated ForceModels).
void RunFluidForcesCpu(
    const FluidForcesUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& state,
    std::span<const Frontier::SimulationSpace::ForceFields::ForceModelGpuRecord> forceRecords,
    FluidVolumeGridCpu& output);

} // namespace Frontier::SimulationSpace::Fluid
