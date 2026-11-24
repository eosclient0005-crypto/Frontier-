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

/// Matches FluidVorticityUniformBlock in FluidCurlVorticityCompute.slang byte-for-byte (176 bytes).
struct alignas(16) FluidVorticityUniformBlock
{
    FluidVolumeUniformBlock Volume {};
    std::array<float, 4>    VorticityControl { 1.0f / 60.0f, 0.0f, 0.0f, 0.0f }; // dt, confinement, 0, 0
};
static_assert(sizeof(FluidVorticityUniformBlock) == 176u, "FluidVorticityUniformBlock must be 176 bytes");

/// Matches FluidThermodynamicsUniformBlock in FluidThermodynamicsCompute.slang byte-for-byte (176 bytes).
struct alignas(16) FluidThermodynamicsUniformBlock
{
    FluidVolumeUniformBlock Volume {};
    std::array<float, 4>    ThermoControl { 1.0f / 60.0f, 0.65f, 1.0f, 0.0f }; // dt, cooling, combustion, ambientTemp
};
static_assert(sizeof(FluidThermodynamicsUniformBlock) == 176u, "FluidThermodynamicsUniformBlock must be 176 bytes");

/// Matches FluidDivergenceUniformBlock in FluidDivergencePressureCompute.slang byte-for-byte (176 bytes).
struct alignas(16) FluidDivergenceUniformBlock
{
    FluidVolumeUniformBlock Volume {};
    std::array<float, 4>    DivergenceControl { 0.0f, 1.0f, 3.5f, 0.0f }; // expansion, fireThreshold, fireSoftness, 0
};
static_assert(sizeof(FluidDivergenceUniformBlock) == 176u, "FluidDivergenceUniformBlock must be 176 bytes");

/// Matches FluidPressureUniformBlock in FluidDivergencePressureCompute.slang byte-for-byte (176 bytes).
struct alignas(16) FluidPressureUniformBlock
{
    FluidVolumeUniformBlock Volume {};
    std::array<float, 4>    PressureControl { 1.0f, 0.0f, 0.0f, 0.0f }; // retain (0.96 on iter 0, 1.0 else)
};
static_assert(sizeof(FluidPressureUniformBlock) == 176u, "FluidPressureUniformBlock must be 176 bytes");

/// Matches FluidGradientShiftUniformBlock in FluidGradientShiftCompute.slang byte-for-byte (192 bytes).
struct alignas(16) FluidGradientShiftUniformBlock
{
    FluidVolumeUniformBlock Volume           {};
    std::array<int32_t, 4>  ShiftDelta       { 0, 0, 0, 0 };
    std::array<int32_t, 4>  OccupancyControl { 2, 3, 0, 0 }; // stride, band, slotCount, 0
};
static_assert(sizeof(FluidGradientShiftUniformBlock) == 192u, "FluidGradientShiftUniformBlock must be 192 bytes");

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

//------------------------------------------------------------------------------------------------------------------------
//                                        SLICE S16 KERNELS & FULL SOLVER STEP
//------------------------------------------------------------------------------------------------------------------------

/// Exact CPU mirror of FluidCurlVorticityCompute.slang::CurlComputeEntry.
void RunFluidCurlCpu(
    const FluidVorticityUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    FluidVolumeGridCpu& outputCurl);

/// Exact CPU mirror of FluidCurlVorticityCompute.slang::VorticityComputeEntry.
void RunFluidVorticityCpu(
    const FluidVorticityUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& curl,
    FluidVolumeGridCpu& outputVelocity);

/// Exact CPU mirror of FluidThermodynamicsCompute.slang (outflow sponge + ignition 0.9 combustion + cooling).
void RunFluidThermodynamicsCpu(
    const FluidThermodynamicsUniformBlock& uniforms,
    const FluidVolumeGridCpu& state,
    FluidVolumeGridCpu& outputState);

/// Exact CPU mirror of FluidDivergencePressureCompute.slang::DivergenceComputeEntry (expansion 0.08*state.w + flame light).
void RunFluidDivergenceCpu(
    const FluidDivergenceUniformBlock& uniforms,
    const FluidVolumeGridCpu& velocity,
    const FluidVolumeGridCpu& state,
    FluidVolumeGridCpu& outputDivergence);

/// Exact CPU mirror of FluidDivergencePressureCompute.slang::PressureComputeEntry (warm-start retain + 6.28 fire light).
void RunFluidPressureJacobiCpu(
    const FluidPressureUniformBlock& uniforms,
    const FluidVolumeGridCpu& pressure,
    const FluidVolumeGridCpu& divergence,
    FluidVolumeGridCpu& outputPressure);

/// Exact CPU mirror of FluidGradientShiftCompute.slang::GradientComputeEntry (projection + solid zero + wall clamping).
void RunFluidGradientCpu(
    const FluidGradientShiftUniformBlock& uniforms,
    const FluidVolumeGridCpu& pressure,
    const FluidVolumeGridCpu& velocity,
    FluidVolumeGridCpu& outputVelocity);

/// Exact CPU mirror of FluidGradientShiftCompute.slang::ShiftComputeEntry (WorldGrid whole-cell relocation).
void RunFluidShiftCpu(
    const FluidGradientShiftUniformBlock& uniforms,
    const FluidVolumeGridCpu& source,
    FluidVolumeGridCpu& output);

/// Exact CPU mirror of FluidGradientShiftCompute.slang::OccupancyComputeEntry (writes 3*SlotCount RGBA floats for SparseTileBank::ScheduleTiles).
void RunFluidOccupancyCpu(
    const SparseTileBank& bank,
    const FluidVolumeGridCpu& stateTiles,
    const FluidVolumeGridCpu& velocityTiles,
    int32_t stride,
    int32_t band,
    std::vector<float>& outputReadbackRgba);

struct FluidStepSettings
{
    FluidBoundaryOrdinal  Boundary            = FluidBoundaryOrdinal::OpenTop;
    FluidAdvectionOrdinal Advection           = FluidAdvectionOrdinal::MacCormack;
    float                 VelocityDissipation = 0.996f;
    float                 Dissipation         = 0.992f;
    float                 Cooling             = 0.65f;
    float                 Combustion          = 1.0f;
    float                 AmbientTemperature  = 0.0f;
    float                 Buoyancy            = 1.4f;
    float                 Gravity             = 0.25f;
    float                 ForceResponse       = 1.0f;
    float                 Vorticity           = 0.0f;
    float                 Expansion           = 0.0f;
    float                 FireThreshold       = 1.0f;
    float                 FireSoftness        = 3.5f;
    uint32_t              PressureIterations  = 24u;
};

struct FluidCpuSimulationState
{
    SparseTileBank     TileBank         {};
    FluidVolumeGridCpu Velocity         {};
    FluidVolumeGridCpu State            {};
    FluidVolumeGridCpu Curl             {};
    FluidVolumeGridCpu Divergence       {};
    FluidVolumeGridCpu Pressure         {};
    FluidVolumeGridCpu ScratchA         {};
    FluidVolumeGridCpu ScratchB         {};
    FluidVolumeGridCpu ScratchC         {};
    uint32_t           WorldTileCells   = 0u;
    double             SimulationTime   = 0.0;
    uint64_t           StepCount        = 0u;

    void InitializeFixed(
        const std::array<uint32_t, 3>& gridCells,
        float cellSizeMeters,
        const std::array<float, 3>& centreMeters);

    void InitializeWorldGrid(
        uint32_t tileSpan,
        uint32_t tileCells,
        float cellSizeMeters,
        const std::array<float, 3>& targetMeters);
};

/// Executes one complete 1:1 FluidSolver.js::step() update (Inject -> Advect -> Thermo -> Forces -> Vorticity -> Projection).
void StepFluidSimulationCpu(
    FluidCpuSimulationState& sim,
    const FluidStepSettings& settings,
    std::span<const FluidReleaseSourceParameters> sources,
    std::span<const Frontier::SimulationSpace::ForceFields::ForceModelGpuRecord> forceRecords,
    std::span<const FluidColliderSphere> colliders,
    float deltaTimeSeconds);

/// Shifts all resident fields by whole cells when WorldGrid follows a moving target (FluidSolver.js relocateGrid).
bool RelocateFluidSimulationCpu(
    FluidCpuSimulationState& sim,
    const std::array<float, 3>& targetMeters,
    FluidBoundaryOrdinal boundary,
    std::span<const FluidColliderSphere> colliders);

/// Measures maximum interior divergence magnitude |div(v)| across non-solid cells for validation.
[[nodiscard]] float MeasureMaxFluidDivergenceCpu(
    const FluidVolumeUniformBlock& volume,
    const FluidVolumeGridCpu& velocity,
    uint32_t interiorMargin = 2u) noexcept;

} // namespace Frontier::SimulationSpace::Fluid
