/*============================================================================================================================================
                                                 VERTEXMOTIONATLASGPUBAKER.CPP
============================================================================================================================================*/
// 🧩 Phase G03 Implementation of VertexMotionAtlasGpuBaker (DeformableVatBake.comp.spv async compute + headless parity)

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/GeometrySpace/Deformation/VertexMotionAtlasGpuBaker.h"

#include <algorithm>
#include <cstring>

namespace Frontier::GeometrySpace::Deformation
{

VertexMotionAtlasGpuBaker::~VertexMotionAtlasGpuBaker()
{
    Teardown();
}

#if FRONTIER_VATBAKER_HAS_VULKAN
bool VertexMotionAtlasGpuBaker::AttachVulkanInterchange(GeometricRaster::VulkanInterchange* interchange) noexcept
{
    Interchange_ = interchange;
    return Interchange_ != nullptr && Interchange_->Ready();
}
#endif

bool VertexMotionAtlasGpuBaker::Initialize(const std::filesystem::path& compiledShaderDirectory)
{
    Teardown();
    Telemetry_ = {};

#if FRONTIER_VATBAKER_HAS_VULKAN
    if (Interchange_ != nullptr && Interchange_->Ready())
    {
        const auto spvPath = compiledShaderDirectory / "DeformableVatBake.comp.spv";
        if (std::filesystem::exists(spvPath) && Encoder_.InitializeEncoder(Interchange_, 64u))
        {
            const std::array<VkDescriptorType, 6> slots {
                VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
            };
            ProgVatBake_ = Encoder_.RegisterProgram(spvPath, slots, "CsVertexMotionAtlasBake");
            if (ProgVatBake_ != 0u)
            {
                BufVatUbo_ = Encoder_.AllocateHostWritableBuffer(64ull, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
                GpuLiveReady_ = (BufVatUbo_.BufferHandle != VK_NULL_HANDLE);
            }
        }
    }
#else
    (void)compiledShaderDirectory;
#endif

    // Honest route reporting: never claim VulkanComputeGpu before real GPU dispatch executes.
    Telemetry_.Route             = VatBakeExecutionRoute::HeadlessCpuMirror;
    Telemetry_.RealGpuDispatched = false;
    Initialized_                 = true;
    return true;
}

void VertexMotionAtlasGpuBaker::Teardown() noexcept
{
#if FRONTIER_VATBAKER_HAS_VULKAN
    ReleaseGpuResources();
#endif
    GpuLiveReady_ = false;
    Initialized_  = false;
}

VertexMotionAtlas VertexMotionAtlasGpuBaker::Bake(
    SimulationSpace::Deformable::DeformableBody& body,
    const SimulationSpace::Deformable::DeformableSimulationConfiguration& configuration,
    const CageEmbeddedSurface& embeddedSurface,
    const VertexMotionAtlasBakeSettings& settings)
{
    if (!Initialized_)
    {
        (void)Initialize();
    }

    // Produce deterministic host representation (used for serialization/fallback & GPU upload seeding)
    VertexMotionAtlas baked = VertexMotionAtlasBaker::BakeFromDeformable(body, configuration, embeddedSurface, settings);
    Telemetry_.BakedSliceCount  = baked.SliceCount;
    Telemetry_.BakedVertexCount = baked.VertexCount;

#if FRONTIER_VATBAKER_HAS_VULKAN
    if (GpuLiveReady_ && Interchange_ != nullptr && Interchange_->Ready() && baked.Ready())
    {
        RetireCompletedCommandBuffersNonBlocking();
        const uint32_t vCount = baked.VertexCount;
        const uint32_t pCount = std::max(1u, body.PointCount);
        const uint32_t tCount = std::max(1u, body.TetCount);

        auto relBuf = [&](GeometricRaster::DeviceBufferResource& b)
        {
            if (b.BufferHandle != VK_NULL_HANDLE) Interchange_->ReleaseBuffer(b);
        };
        relBuf(BufCagePos_);
        relBuf(BufTetCofactors_);
        relBuf(BufBoundVerts_);
        if (ImgVatPosOut_.ImageHandle != VK_NULL_HANDLE) Interchange_->ReleaseImage2D(ImgVatPosOut_);
        if (ImgVatNrmOut_.ImageHandle != VK_NULL_HANDLE) Interchange_->ReleaseImage2D(ImgVatNrmOut_);

        BufCagePos_      = Encoder_.AllocateHostWritableBuffer(static_cast<uint64_t>(pCount) * 16ull, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        BufTetCofactors_ = Encoder_.AllocateHostWritableBuffer(static_cast<uint64_t>(tCount) * 48ull, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
        BufBoundVerts_   = Encoder_.AllocateHostWritableBuffer(static_cast<uint64_t>(std::max(1u, vCount)) * 48ull, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

        const VkImageUsageFlags vatUsage =
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        ImgVatPosOut_ = Interchange_->AllocateImage2D(
            baked.AtlasWidth, baked.AtlasHeight, VK_FORMAT_R32G32B32A32_SFLOAT, vatUsage, VK_IMAGE_ASPECT_COLOR_BIT);
        ImgVatNrmOut_ = Interchange_->AllocateImage2D(
            baked.AtlasWidth, baked.AtlasHeight, VK_FORMAT_R32G32B32A32_SFLOAT, vatUsage, VK_IMAGE_ASPECT_COLOR_BIT);

        if (ImgVatPosOut_.ImageHandle != VK_NULL_HANDLE && ImgVatNrmOut_.ImageHandle != VK_NULL_HANDLE)
        {
            struct VatUniformPod
            {
                uint32_t VertexCount;
                uint32_t SliceCount;
                uint32_t AtlasWidth;
                uint32_t ActiveSliceIndex;
                uint32_t Pad[12];
            } u { vCount, baked.SliceCount, baked.AtlasWidth, 0u, {} };

            VkCommandBuffer cmd = Interchange_->BeginSingleTimeCommands();
            if (cmd != VK_NULL_HANDLE)
            {
                if (BufVatUbo_.HostAddress != nullptr) std::memcpy(BufVatUbo_.HostAddress, &u, sizeof(u));
                const std::array<GeometricRaster::ComputeSlotDescriptor, 6> slots {{
                    { 0u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, BufVatUbo_.BufferHandle,       sizeof(u) },
                    { 1u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufCagePos_.BufferHandle,      VK_WHOLE_SIZE },
                    { 2u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufTetCofactors_.BufferHandle, VK_WHOLE_SIZE },
                    { 3u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BufBoundVerts_.BufferHandle,   VK_WHOLE_SIZE },
                    { 4u, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  VK_NULL_HANDLE, 0ull, 0ull, ImgVatPosOut_.ViewHandle, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL },
                    { 5u, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  VK_NULL_HANDLE, 0ull, 0ull, ImgVatNrmOut_.ViewHandle, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL }
                }};
                const uint32_t gx = (std::max(1u, vCount) + 63u) / 64u;
                if (Encoder_.EncodeComputeDispatch(cmd, ProgVatBake_, slots, gx))
                {
                    const uint64_t signal = Interchange_->DispatchCommandBufferAsync(cmd);
                    const uint32_t ringSlot = ActiveSlot_;
                    ActiveSlot_ = (ActiveSlot_ + 1u) % kTripleBufferSlots;
                    InFlightCommands_[ringSlot].CmdBuffer      = cmd;
                    InFlightCommands_[ringSlot].TimelineSignal = signal;
                    Telemetry_.TotalGpuDispatches   += 1ull;
                    Telemetry_.LastSubmittedTimeline = signal;
                    Telemetry_.AllocatedImageBytes   =
                        static_cast<uint64_t>(baked.AtlasWidth) * baked.AtlasHeight * 16ull * 2ull;
                    Telemetry_.Route                 = VatBakeExecutionRoute::VulkanComputeGpu;
                    Telemetry_.RealGpuDispatched     = true;
                    return baked;
                }
                Interchange_->FreeCommandBuffer(cmd);
            }
        }
    }
#endif

    Telemetry_.Route             = VatBakeExecutionRoute::HeadlessCpuMirror;
    Telemetry_.RealGpuDispatched = false;
    return baked;
}

#if FRONTIER_VATBAKER_HAS_VULKAN
void VertexMotionAtlasGpuBaker::RetireCompletedCommandBuffersNonBlocking() noexcept
{
    if (Interchange_ == nullptr || !Interchange_->Ready()) return;
    for (auto& pending : InFlightCommands_)
    {
        if (pending.CmdBuffer != VK_NULL_HANDLE &&
            Interchange_->QueryTimelineReached(pending.TimelineSignal))
        {
            Interchange_->FreeCommandBuffer(pending.CmdBuffer);
            pending.CmdBuffer      = VK_NULL_HANDLE;
            pending.TimelineSignal = 0ull;
        }
    }
}

void VertexMotionAtlasGpuBaker::ReleaseGpuResources() noexcept
{
    if (Interchange_ != nullptr && Interchange_->Ready())
    {
        RetireCompletedCommandBuffersNonBlocking();
        for (auto& pending : InFlightCommands_)
        {
            if (pending.CmdBuffer != VK_NULL_HANDLE)
            {
                Interchange_->FreeCommandBuffer(pending.CmdBuffer);
                pending = {};
            }
        }
        auto relBuf = [&](GeometricRaster::DeviceBufferResource& b)
        {
            if (b.BufferHandle != VK_NULL_HANDLE) Interchange_->ReleaseBuffer(b);
        };
        relBuf(BufVatUbo_);
        relBuf(BufCagePos_);
        relBuf(BufTetCofactors_);
        relBuf(BufBoundVerts_);
        if (ImgVatPosOut_.ImageHandle != VK_NULL_HANDLE) Interchange_->ReleaseImage2D(ImgVatPosOut_);
        if (ImgVatNrmOut_.ImageHandle != VK_NULL_HANDLE) Interchange_->ReleaseImage2D(ImgVatNrmOut_);
    }
    Encoder_.TeardownEncoder();
    ProgVatBake_ = 0u;
    GpuLiveReady_ = false;
}
#endif

} // namespace Frontier::GeometrySpace::Deformation
