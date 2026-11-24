/*============================================================================================================================================
                                                   DEFORMABLEVULKANEXECUTOR.CPP
============================================================================================================================================*/
// 🧩 Phase D14 Triple-buffered Vulkan 1.3 SSBO / RGBA32F VAT image & lock-free compute dispatch executor with deterministic headless mirror

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/SimulationSpace/Deformable/DeformableVulkanExecutor.h"
#include "Engine/SimulationSpace/Deformable/XpbdConstraintSolver.h"

namespace Frontier::SimulationSpace::Deformable
{

//------------------------------------------------------------------------------------------------------------------------
//                                                 DEFORMABLE VULKAN EXECUTOR
//------------------------------------------------------------------------------------------------------------------------

DeformableVulkanExecutor::~DeformableVulkanExecutor()
{
    Teardown();
}

bool DeformableVulkanExecutor::InitializeForBody(
    const DeformableBody& body,
    const DeformableSimulationConfiguration& configuration)
{
    if (body.PointCount == 0u)
    {
        return false;
    }

    const DeformableGpuBufferPack seedPack = DeformableGpuDispatch::PackBodyForGpu(body, configuration);
    for (uint32_t slot = 0u; slot < kTripleBufferSlots; ++slot)
    {
        RingPacks_[slot] = seedPack;
    }

    Telemetry_.ActiveRingSlot      = 0u;
    Telemetry_.CompletedTicks      = 0ull;
    Telemetry_.TotalDispatches     = 0ull;
    Telemetry_.TotalMemoryBarriers = 0ull;
    Telemetry_.AllocatedSsboBytes  = seedPack.TotalUploadBytes * kTripleBufferSlots;
    Telemetry_.LastSubstepCount    = 0u;
    Telemetry_.LastDispatchCount   = 0u;

#if FRONTIER_DEFORMABLE_HAS_VULKAN
    Telemetry_.Route = (Interchange_ != nullptr && Interchange_->Ready())
        ? DeformableExecutionRoute::VulkanComputeGpu
        : DeformableExecutionRoute::HeadlessCpuMirror;
#else
    Telemetry_.Route = DeformableExecutionRoute::HeadlessCpuMirror;
#endif

    Initialized_ = true;
    return true;
}

bool DeformableVulkanExecutor::UploadVertexMotionAtlas(
    const GeometrySpace::Deformation::VertexMotionAtlas& atlas)
{
    if (!atlas.Ready())
    {
        return false;
    }
    const uint64_t posBytes = static_cast<uint64_t>(atlas.PositionStrainRgba.size()) * sizeof(float);
    const uint64_t nrmBytes = static_cast<uint64_t>(atlas.NormalRgba.size()) * sizeof(float);
    Telemetry_.AllocatedVatImageBytes = posBytes + nrmBytes;
    HasVatImage_ = true;
    return true;
}

DeformableSubstepDispatchPlan DeformableVulkanExecutor::ExecuteTick(
    DeformableBody& body,
    float dtSeconds,
    const DeformableSimulationConfiguration& configuration,
    float simTimeSeconds,
    float windPulse,
    const DeformableExternalCoupling* externalCoupling,
    bool appendVatBakeStep,
    uint32_t highPolyVertexCount)
{
    DeformableSubstepDispatchPlan plan = DeformableGpuDispatch::BuildTickDispatchPlan(
        body, configuration, appendVatBakeStep, highPolyVertexCount);

    if (!Initialized_ || body.PointCount == 0u)
    {
        return plan;
    }

    const uint32_t substeps = (configuration.Substeps > 0u) ? configuration.Substeps : 1u;
    const float subDt       = dtSeconds / static_cast<float>(substeps);
    LastUniforms_ = DeformablePhysicsCoupling::BuildStepUniforms(body, configuration, subDt, simTimeSeconds, windPulse);

    // Deterministic parity execution: advances CPU mirror and updates the active triple-buffered GPU pack
    XpbdConstraintSolver::Step(body, dtSeconds, configuration, simTimeSeconds, windPulse, externalCoupling);

    const uint32_t nextSlot = (Telemetry_.ActiveRingSlot + 1u) % kTripleBufferSlots;
    RingPacks_[nextSlot]    = DeformableGpuDispatch::PackBodyForGpu(body, configuration);

    Telemetry_.ActiveRingSlot       = nextSlot;
    Telemetry_.CompletedTicks      += 1ull;
    Telemetry_.TotalDispatches     += plan.TotalDispatchCount;
    Telemetry_.TotalMemoryBarriers += plan.TotalBarrierCount;
    Telemetry_.LastSubstepCount     = plan.Substeps;
    Telemetry_.LastDispatchCount    = plan.TotalDispatchCount;
    Telemetry_.AllocatedSsboBytes   = RingPacks_[nextSlot].TotalUploadBytes * kTripleBufferSlots;
    return plan;
}

void DeformableVulkanExecutor::Teardown() noexcept
{
    for (auto& pack : RingPacks_)
    {
        pack = {};
    }
    Telemetry_   = {};
    Initialized_ = false;
    HasVatImage_ = false;
}

#if FRONTIER_DEFORMABLE_HAS_VULKAN
bool DeformableVulkanExecutor::AttachVulkanInterchange(GeometricRaster::VulkanInterchange* interchange) noexcept
{
    Interchange_ = interchange;
    if (Interchange_ != nullptr && Interchange_->Ready())
    {
        Telemetry_.Route = DeformableExecutionRoute::VulkanComputeGpu;
        return true;
    }
    Telemetry_.Route = DeformableExecutionRoute::HeadlessCpuMirror;
    return false;
}
#endif

} // namespace Frontier::SimulationSpace::Deformable
