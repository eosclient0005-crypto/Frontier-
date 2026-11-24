/*============================================================================================================================================
                                                       PARTICLEGPUSIMULATION.H
============================================================================================================================================*/
// 🧩 Core ParticleSpace GPU uniform layout, anchor table builder, per-cloud state manager & bit-accurate CPU mirror of ParticleUpdateCompute.slang
// Reference: Experimental/FluidEditor/Engine/ParticleShaders.js L9-L752 & ParticleSimulation.js L1-L340

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/ParticleSpace/CarrierSolidRegistry.h"
#include "Engine/ParticleSpace/FireworkPatternSampler.h"
#include "Engine/ParticleSpace/ParticleCloudTypes.h"
#include "Engine/SimulationSpace/ForceFields/ForceModelTypes.h"

#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Frontier::ParticleSpace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                 GPU ABI DATA STRUCTURES
//------------------------------------------------------------------------------------------------------------------------

/// Matches ParticleStateRecord in ParticleUpdateCompute.slang byte-for-byte (32 bytes, 16-byte aligned).
struct alignas(16) ParticleStateGpuRecord
{
    std::array<float, 4> PositionAge  { 0.0f, -1.0e4f, 0.0f, 1.0f }; // xyz = position (m), w = age (s)
    std::array<float, 4> VelocitySeed { 0.0f, 0.0f, 0.0f, 0.0f };    // xyz = velocity (m/s), w = seed [0..1]
};
static_assert(sizeof(ParticleStateGpuRecord) == 32u, "ParticleStateGpuRecord must be 32 bytes for Slang SSBO parity");

/// Matches ParticleShapeUniformBlock in ParticleShapeAnchor.slang byte-for-byte (176 bytes, 16-byte aligned).
struct alignas(16) ParticleShapeUniformBlock
{
    std::array<float, 4> OriginRadius        { 0.0f, 0.0f, 0.0f, 0.5f };
    std::array<float, 4> AxisFill            { 0.0f, 1.0f, 0.0f, 0.0f };
    std::array<float, 4> ShapeCounts         { 0.0f, 2048.0f, 8.0f, 0.0f };
    std::array<float, 4> AnchorLayout        { 1.0f, 0.0f, 48.0f, 0.0f };
    std::array<float, 4> MocapParams         { 0.0f, 0.0f, 1.0f, 1.0f };
    std::array<float, 4> MocapKickEffector   { 0.0f, 0.0f, 0.0f, 1.8f };
    std::array<float, 4> EffectorMotion      { 1.6f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> EffectorAxisDerez   { 0.0f, 0.0f, 0.0f, 0.0f };
    std::array<float, 4> DerezFrontParams    { 0.0f, 0.14f, 0.0f, 0.0f };
    std::array<float, 4> DerezEdgeColourWing { 0.0f, 0.9411765f, 1.0f, 10.0f };
    std::array<float, 4> GlitchParams        { 0.0f, 14.0f, 0.0f, 0.0f };
};
static_assert(sizeof(ParticleShapeUniformBlock) == 176u, "ParticleShapeUniformBlock must be 176 bytes");

/// Matches ParticleUpdateUniformBlock in ParticleUpdateCompute.slang byte-for-byte (304 bytes, 16-byte aligned).
struct alignas(16) ParticleUpdateUniformBlock
{
    ParticleShapeUniformBlock Shape {};
    std::array<float, 4>      GridCentreHalfX   { 0.0f, 2.0f, 0.0f, 2.5f };
    std::array<float, 4>      GridHalfYZTimeDt  { 2.0f, 2.5f, 0.0f, 0.01666667f };
    std::array<float, 4>      SpawnKinematics   { 1.0f, 8.0f, 3.0f, 2.0f };
    std::array<float, 4>      LaunchForces      { 0.5f, 0.0f, 0.0f, 0.5f };
    std::array<float, 4>      CouplingNoise     { 0.0f, 1.0f, 0.0f, 1.3f };
    std::array<float, 4>      SpringBounce      { 6.0f, 3.0f, 0.2f, 0.5f };
    std::array<float, 4>      DissolveSwarm     { 0.0f, 0.6f, 0.0f, 0.0f };
    std::array<float, 4>      SwarmControlFlags { 0.0f, 0.0f, 0.0f, 0.0f };
};
static_assert(sizeof(ParticleUpdateUniformBlock) == 304u, "ParticleUpdateUniformBlock must be 304 bytes");

/// Matches ParticleTrailUpdateUniformBlock in ParticleTrailUpdateCompute.slang byte-for-byte (32 bytes, 16-byte aligned).
struct alignas(16) ParticleTrailUpdateUniformBlock
{
    std::array<float, 4> LayoutHeadAdvance { 2304.0f, 24.0f, 0.0f, 0.0f }; // capacity, trailLength, head, advance
    std::array<float, 4> ResetLifetimePad  { 1.0f, 3.0f, 0.0f, 0.0f };     // reset, lifetime (0 for Cloner), pad, pad
};
static_assert(sizeof(ParticleTrailUpdateUniformBlock) == 32u, "ParticleTrailUpdateUniformBlock must be 32 bytes");

struct DerezEvaluationReading
{
    float WaveU    = 0.5f;
    float Progress = 0.0f;
    float EdgeBand = 0.0f;
    bool  Released = false;
};

struct ParticleSimulationRegion
{
    std::array<float, 3> CentreMeters     { 0.0f, 2.0f, 0.0f };
    std::array<float, 3> HalfExtentMeters { 2.5f, 2.0f, 2.5f };
};

struct ParticleSimulationStatistics
{
    uint32_t ActiveCloudCount     = 0u;
    uint32_t ActiveParticleCount  = 0u;
    uint32_t DispatchWorkgroups   = 0u;
};

using GasVelocityQueryFn = std::function<std::array<float, 3>(const std::array<float, 3>& worldPos)>;

/// Per-cloud ping-pong particle state, previous positions, and baked anchor texels.
struct ParticleCloudStateRecord
{
    std::string                         NodeId             {};
    uint32_t                            TextureSide        = 48u;
    uint32_t                            Capacity           = 2304u;
    uint32_t                            ActiveCount        = 2048u;
    bool                                NeedsReset         = true;
    std::string                         FormationSignature {};
    std::string                         AnchorSignature    {};
    AnchorTexelTable                    AnchorTexels       {};
    std::vector<ParticleStateGpuRecord> ReadStates         {};
    std::vector<ParticleStateGpuRecord> WriteStates        {};
    std::vector<std::array<float, 4>>   PreviousPositions  {};
    ParticleUpdateUniformBlock          LastUniformBlock   {};

    // On-demand spline ribbon history ring buffer (ParticleSimulation.js L82-L95, L147-L172)
    uint32_t                            TrailLength        = 0u;
    uint32_t                            TrailHead          = 0u;
    float                               TrailClock         = 0.0f;
    bool                                TrailNeedsReset    = true;
    std::vector<std::array<float, 4>>   TrailSamples       {};
    std::vector<std::array<float, 4>>   TrailScratch       {};
    ParticleTrailUpdateUniformBlock     LastTrailUniforms  {};
};

//------------------------------------------------------------------------------------------------------------------------
//                                           STATELESS CPU SHADER MIRROR
//------------------------------------------------------------------------------------------------------------------------

/// Populates ParticleShapeUniformBlock from evaluated cloud parameters & anchor layout.
[[nodiscard]] ParticleShapeUniformBlock BuildParticleShapeUniforms(
    const ParticleCloudParameters& params,
    uint32_t textureSide,
    uint32_t anchorWidth,
    uint32_t anchorCount) noexcept;

/// Populates ParticleUpdateUniformBlock for compute dispatch or CPU stepping.
[[nodiscard]] ParticleUpdateUniformBlock BuildParticleUpdateUniforms(
    const ParticleCloudNode& node,
    uint32_t textureSide,
    uint32_t anchorWidth,
    uint32_t anchorCount,
    const ParticleSimulationRegion& region,
    float simulationTimeSeconds,
    float deltaTimeSeconds,
    bool resetFlag,
    uint32_t forceRecordCount,
    bool hasGasVolume) noexcept;

/// Bit-accurate CPU mirror of ParticleShapeAnchor.slang::ShapePointAtTime.
[[nodiscard]] std::array<float, 3> EvaluateShapePointAtTime(
    const ParticleShapeUniformBlock& shape,
    std::span<const float> anchorTexelRgba,
    int32_t index,
    const std::array<float, 3>& rnd,
    bool lattice,
    float timeSeconds,
    std::array<float, 3>* outShapeVelocity = nullptr) noexcept;

/// Bit-accurate CPU mirror of ParticleShapeAnchor.slang::EvaluateEffector.
[[nodiscard]] float EvaluateEffectorWeight(
    const ParticleShapeUniformBlock& shape,
    const std::array<float, 3>& worldPosition,
    float timeSeconds) noexcept;

/// Bit-accurate CPU mirror of ParticleShapeAnchor.slang::EvaluateDerez.
[[nodiscard]] DerezEvaluationReading EvaluateDerezFront(
    const ParticleShapeUniformBlock& shape,
    const std::array<float, 3>& restHome,
    float timeSeconds) noexcept;

/// Bit-accurate CPU mirror of ParticleUpdateCompute.slang::ComputeEntry for all particles in a cloud.
void StepParticleCloudCpu(
    ParticleCloudStateRecord& record,
    const ParticleUpdateUniformBlock& uniforms,
    std::span<const SimulationSpace::ForceFields::ForceModelGpuRecord> forceRecords = {},
    const GasVelocityQueryFn& gasVelocityQuery = {});

/// Bit-accurate CPU mirror of ParticleTrailUpdateCompute.slang::ComputeEntry (ring-buffer history up to 96 samples).
void StepTrailHistoryCpu(
    ParticleCloudStateRecord& record,
    const ParticleTrailUpdateUniformBlock& uniforms);

//------------------------------------------------------------------------------------------------------------------------
//                                                PARTICLEGPUSIMULATION
//------------------------------------------------------------------------------------------------------------------------

/// Manages per-cloud GPU/CPU state records, anchor texture refreshes, and simulation stepping.
class ParticleGpuSimulation
{
public:
    ParticleGpuSimulation() noexcept = default;
    ~ParticleGpuSimulation() noexcept = default;

    void ClearAll() noexcept;
    void ResetAll() noexcept;
    void ResetCloud(std::string_view nodeId) noexcept;
    void PruneDeadClouds(std::span<const ParticleCloudNode> liveNodes);

    [[nodiscard]] ParticleCloudStateRecord& EnsureCloudRecord(
        const ParticleCloudNode& node,
        const CarrierSolidRegistry& solidRegistry,
        float simulationTimeSeconds);

    [[nodiscard]] const ParticleCloudStateRecord* FindCloudRecord(std::string_view nodeId) const noexcept;
    [[nodiscard]] ParticleSimulationStatistics QueryStatistics() const noexcept { return Statistics; }

    /// Steps all enabled non-halted clouds using the bit-accurate CPU compute mirror & builds GPU uniform blocks.
    void StepCloudsCpu(
        std::span<const ParticleCloudNode> clouds,
        const CarrierSolidRegistry& solidRegistry,
        const ParticleSimulationRegion& region,
        float simulationTimeSeconds,
        float deltaTimeSeconds,
        std::span<const SimulationSpace::ForceFields::ForceModelGpuRecord> forceRecords = {},
        const GasVelocityQueryFn& gasVelocityQuery = {});

private:
    void RefreshCloudAnchors(
        ParticleCloudStateRecord& record,
        const ParticleCloudNode& node,
        const CarrierSolidRegistry& solidRegistry,
        float simulationTimeSeconds);

    std::vector<ParticleCloudStateRecord> Records    {};
    ParticleSimulationStatistics          Statistics {};
};

} // namespace Frontier::ParticleSpace
