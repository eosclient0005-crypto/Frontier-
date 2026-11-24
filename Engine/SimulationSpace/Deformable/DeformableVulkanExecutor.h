/*============================================================================================================================================
                                                    DEFORMABLEVULKANEXECUTOR.H
============================================================================================================================================*/
// 🧩 Phase D14 Triple-buffered Vulkan 1.3 SSBO / RGBA32F VAT image & lock-free compute dispatch executor with deterministic headless mirror
// Reference: Documentation/Plans/DeformableFemVatArchitecturePlan.md §4 (D14)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/GeometrySpace/Deformation/VertexMotionAtlasTypes.h"
#include "Engine/SimulationSpace/Deformable/DeformableGpuDispatch.h"
#include "Engine/SimulationSpace/Deformable/DeformablePhysicsCoupling.h"
#include "Engine/SimulationSpace/Deformable/DeformableTypes.h"

#if __has_include(<volk.h>)
#define FRONTIER_DEFORMABLE_HAS_VULKAN 1
#include "Engine/GeometricRaster/VulkanInterchange.h"
#else
#define FRONTIER_DEFORMABLE_HAS_VULKAN 0
#endif

#include <array>
#include <cstdint>

namespace Frontier::SimulationSpace::Deformable
{

//------------------------------------------------------------------------------------------------------------------------
//                                                 EXECUTOR TELEMETRY
//------------------------------------------------------------------------------------------------------------------------

enum class DeformableExecutionRoute : uint8_t
{
    HeadlessCpuMirror = 0u, // Deterministic CPU parity executor + triple-buffered staging ring
    VulkanComputeGpu  = 1u  // Native Vulkan 1.3 compute SSBO / RGBA32F VAT image executor
};

struct DeformableExecutorTelemetry
{
    DeformableExecutionRoute Route                  = DeformableExecutionRoute::HeadlessCpuMirror;
    uint32_t                 ActiveRingSlot         = 0u;
    uint64_t                 CompletedTicks         = 0ull;
    uint64_t                 TotalDispatches        = 0ull;
    uint64_t                 TotalMemoryBarriers    = 0ull;
    uint64_t                 AllocatedSsboBytes     = 0ull;
    uint64_t                 AllocatedVatImageBytes = 0ull;
    uint32_t                 LastSubstepCount       = 0u;
    uint32_t                 LastDispatchCount      = 0u;
};

//------------------------------------------------------------------------------------------------------------------------
//                                                 DEFORMABLE VULKAN EXECUTOR
//------------------------------------------------------------------------------------------------------------------------

class DeformableVulkanExecutor
{
public:
    static constexpr uint32_t kTripleBufferSlots = 3u;

    DeformableVulkanExecutor() = default;
    ~DeformableVulkanExecutor();

    DeformableVulkanExecutor(const DeformableVulkanExecutor&)            = delete;
    DeformableVulkanExecutor& operator=(const DeformableVulkanExecutor&) = delete;

    /// Initializes triple-buffered SSBO staging rings for `body` (headless CPU mirror or native Vulkan device).
    [[nodiscard]] bool InitializeForBody(
        const DeformableBody& body,
        const DeformableSimulationConfiguration& configuration);

    /// Uploads a baked RGBA32F Vertex Motion Atlas into the executor's VAT texture resource slot.
    [[nodiscard]] bool UploadVertexMotionAtlas(
        const GeometrySpace::Deformation::VertexMotionAtlas& atlas);

    /// Advances one simulation tick: builds the lock-free conflict-colored dispatch plan, executes via the active route,
    /// rotates the triple-buffered ring slot, and updates packed GPU buffers & telemetry.
    [[nodiscard]] DeformableSubstepDispatchPlan ExecuteTick(
        DeformableBody& body,
        float dtSeconds,
        const DeformableSimulationConfiguration& configuration,
        float simTimeSeconds,
        float windPulse = 0.0f,
        const DeformableExternalCoupling* externalCoupling = nullptr,
        bool appendVatBakeStep = false,
        uint32_t highPolyVertexCount = 0u);

    void Teardown() noexcept;

    [[nodiscard]] bool Ready() const noexcept { return Initialized_; }
    [[nodiscard]] const DeformableExecutorTelemetry& InspectTelemetry() const noexcept { return Telemetry_; }
    [[nodiscard]] const DeformableGpuBufferPack& InspectActiveGpuPack() const noexcept
    {
        return RingPacks_[Telemetry_.ActiveRingSlot];
    }

#if FRONTIER_DEFORMABLE_HAS_VULKAN
    [[nodiscard]] bool AttachVulkanInterchange(GeometricRaster::VulkanInterchange* interchange) noexcept;
#endif

private:
    std::array<DeformableGpuBufferPack, kTripleBufferSlots> RingPacks_   {};
    DeformableStepUniforms                                  LastUniforms_ {};
    DeformableExecutorTelemetry                             Telemetry_   {};
    bool                                                    Initialized_ = false;
    bool                                                    HasVatImage_ = false;

#if FRONTIER_DEFORMABLE_HAS_VULKAN
    GeometricRaster::VulkanInterchange*                     Interchange_ = nullptr;
    std::array<GeometricRaster::DeviceBufferResource, kTripleBufferSlots> UniformBuffers_ {};
    GeometricRaster::DeviceImageResource                    VatPosImage_ {};
    GeometricRaster::DeviceImageResource                    VatNrmImage_ {};
#endif
};

} // namespace Frontier::SimulationSpace::Deformable
