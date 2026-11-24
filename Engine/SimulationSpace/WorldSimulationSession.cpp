/*============================================================================================================================================
                                                   WORLDSIMULATIONSESSION.CPP
============================================================================================================================================*/
// 🧩 Phase D34 Implementation of WorldSimulationSession

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/SimulationSpace/WorldSimulationSession.h"

#include <algorithm>
#include <unordered_map>

namespace Frontier::SimulationSpace
{

namespace
{

uint64_t PackClassKey(uint32_t slotIndex, uint32_t generation) noexcept
{
    return (static_cast<uint64_t>(generation) << 32u) | static_cast<uint64_t>(slotIndex);
}

} // namespace

WorldSimulationSession::~WorldSimulationSession()
{
    Stop();
}

bool WorldSimulationSession::MountPreparedScene(const SimulationSceneDocument& sceneDoc, std::string* outError)
{
    Coordinator_ = std::make_unique<SimulationCoordinatorExtension>(
        SimulationSceneCodec::BuildCoordinatorParameters(sceneDoc));
    Coordinator_->SetShouldUpdate(ShouldUpdate_);
#if FRONTIER_FLUID_HAS_VULKAN
    if (Interchange_ != nullptr)
    {
        (void)Coordinator_->AttachVulkanInterchange(Interchange_);
    }
#endif

    Sequence_ = std::make_unique<Lifecycle::LifecycleSequence>();
    (void)Sequence_->RegisterParticipant("FrontierWorldSimulation", *Coordinator_);
    if (!Sequence_->Initialize())
    {
        if (outError != nullptr) *outError = "Failed to initialize SimulationCoordinatorExtension in WorldSimulationSession.";
        Stop();
        return false;
    }

    if (!SimulationSceneCodec::MountIntoCoordinator(sceneDoc, *Coordinator_, outError))
    {
        Stop();
        return false;
    }

    ActiveScene_ = sceneDoc;
    RunStatus_   = PreviewRunStatus::Running;
    return true;
}

bool WorldSimulationSession::BeginPlayFromWorldPlacements(
    std::span<const ComponentClass::ComponentClassDefinition> classDefinitions,
    const ComponentClass::ComponentWorldPlacementRegister&    worldPlacements,
    std::string_view                                          sessionTitle,
    std::string*                                              outError)
{
    Stop();

    std::unordered_map<uint64_t, const ComponentClass::ComponentClassDefinition*> catalog;
    catalog.reserve(classDefinitions.size());
    for (std::size_t classOrdinal = 0u; classOrdinal < classDefinitions.size(); ++classOrdinal)
    {
        catalog.emplace(PackClassKey(static_cast<uint32_t>(classOrdinal + 1u), 1u), &classDefinitions[classOrdinal]);
    }

    AssemblyReport_ = {};
    const SimulationSceneDocument sceneDoc = ComponentClassSimulationAssembler::BuildSceneFromPlacements(
        catalog, worldPlacements, sessionTitle, &AssemblyReport_);

    const uint32_t totalSimEntities = AssemblyReport_.ForcesAssembled + AssemblyReport_.FluidSourcesAssembled +
                                      AssemblyReport_.CloudsAssembled + AssemblyReport_.SoftBodiesAssembled;
    if (totalSimEntities == 0u)
    {
        if (outError != nullptr) *outError = "World placements contain no active simulation components.";
        return false;
    }

    return MountPreparedScene(sceneDoc, outError);
}

bool WorldSimulationSession::BeginPlayFromSceneFile(
    const std::filesystem::path& sceneFilePath,
    std::string*                 outError)
{
    Stop();

    SimulationSceneDocument decoded {};
    if (!SimulationSceneCodec::DecodeFromFile(sceneFilePath, decoded, outError))
    {
        return false;
    }

    AssemblyReport_ = {};
    AssemblyReport_.PlacementsProcessed   = 1u;
    AssemblyReport_.ForcesAssembled       = static_cast<uint32_t>(decoded.Forces.size());
    AssemblyReport_.FluidSourcesAssembled = static_cast<uint32_t>(decoded.FluidSources.size());
    AssemblyReport_.CloudsAssembled       = static_cast<uint32_t>(decoded.ParticleClouds.size());
    AssemblyReport_.SoftBodiesAssembled   = static_cast<uint32_t>(decoded.SoftBodies.size());
    AssemblyReport_.CollidersAssembled    = static_cast<uint32_t>(decoded.RigidColliders.size());

    return MountPreparedScene(decoded, outError);
}

void WorldSimulationSession::Tick(double deltaSeconds, const std::array<float, 3>& cameraWorldPos) noexcept
{
    if (RunStatus_ != PreviewRunStatus::Running || !ShouldUpdate_ || !Coordinator_ || !Sequence_)
    {
        return;
    }
    Coordinator_->AssignCameraWorldPosition(cameraWorldPos);
    const double clampedDt = std::clamp(deltaSeconds, 0.0005, 0.05);
    Sequence_->Tick(clampedDt);
}

void WorldSimulationSession::SetShouldUpdate(bool shouldUpdate) noexcept
{
    ShouldUpdate_ = shouldUpdate;
    if (Coordinator_)
    {
        Coordinator_->SetShouldUpdate(shouldUpdate);
    }
}

#if FRONTIER_FLUID_HAS_VULKAN
bool WorldSimulationSession::AttachVulkanInterchange(GeometricRaster::VulkanInterchange* interchange) noexcept
{
    Interchange_ = interchange;
    return Coordinator_ ? Coordinator_->AttachVulkanInterchange(interchange) : (interchange != nullptr && interchange->Ready());
}
#endif

bool WorldSimulationSession::Pause() noexcept
{
    if (RunStatus_ != PreviewRunStatus::Running || !Sequence_)
    {
        return false;
    }
    if (!Sequence_->Suspend())
    {
        return false;
    }
    RunStatus_ = PreviewRunStatus::Paused;
    return true;
}

bool WorldSimulationSession::Resume() noexcept
{
    if (RunStatus_ != PreviewRunStatus::Paused || !Sequence_)
    {
        return false;
    }
    if (!Sequence_->Resume())
    {
        return false;
    }
    RunStatus_ = PreviewRunStatus::Running;
    return true;
}

void WorldSimulationSession::Stop() noexcept
{
    if (Sequence_)
    {
        if (Coordinator_)
        {
            Sequence_->Finalize();
        }
        Sequence_.reset();
    }
    Coordinator_.reset();
    AssemblyReport_ = {};
    ActiveScene_    = {};
    RunStatus_      = PreviewRunStatus::Idle;
}

SimulationCoordinatorTelemetry WorldSimulationSession::InspectTelemetry() const noexcept
{
    if (!Coordinator_)
    {
        return {};
    }
    return Coordinator_->InspectTelemetry();
}

SimulationExecutionRouteSummary WorldSimulationSession::InspectExecutionRoutes() const noexcept
{
    if (!Coordinator_)
    {
        return {};
    }
    return Coordinator_->InspectExecutionRoutes();
}

std::vector<PreviewSurfaceSnapshot> WorldSimulationSession::ExtractDeformableRenderSurfaces() const
{
    std::vector<PreviewSurfaceSnapshot> out;
    if (!Coordinator_)
    {
        return out;
    }

    const auto& worldReg = Coordinator_->InspectDeformables().InspectWorldRegistry();
    for (const auto& entry : worldReg.EnumerateInstances())
    {
        if (!entry.Enabled) continue;
        PreviewSurfaceSnapshot snap {};
        snap.BodyInstanceId = entry.InstanceId;
        snap.Label          = entry.InstanceName;
        GeometrySpace::Deformation::DeformableSurfaceBuilder::AppendBodyAttachments(entry.Body, snap.Surface, true, true);
        out.push_back(std::move(snap));
    }
    return out;
}

std::string WorldSimulationSession::ExportActiveSceneToToml() const
{
    return SimulationSceneCodec::EncodeToTomlText(ActiveScene_);
}

} // namespace Frontier::SimulationSpace
