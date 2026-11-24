/*============================================================================================================================================
                                                    WORLDSIMULATIONSESSION.H
============================================================================================================================================*/
// 🧩 Phase D34 Unified Play-in-Editor & FrontierWorld 5-domain runtime session launching live coupled simulation directly
//    from EditorLifecycle ComponentDefinitions + WorldPlacements OR from a .VolumetricSpace scene file on disk.
// Reference: Documentation/Plans/DeformableFemVatArchitecturePlan.md §4 (D34-D35)

#pragma once

//------------------------------------------------------------------------------------------------------------------------
//                                                        INCLUDES
//------------------------------------------------------------------------------------------------------------------------

#include "Engine/ComponentClass/ComponentWorldPlacement.h"
#include "Engine/Lifecycle/LifecycleSequence.h"
#include "Engine/SimulationSpace/ComponentClassSimulationAssembler.h"
#include "Engine/SimulationSpace/ComponentSimulationPreviewDriver.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Frontier::SimulationSpace
{

//------------------------------------------------------------------------------------------------------------------------
//                                                WORLD SIMULATION SESSION
//------------------------------------------------------------------------------------------------------------------------

class WorldSimulationSession
{
public:
    WorldSimulationSession() = default;
    ~WorldSimulationSession();

    WorldSimulationSession(const WorldSimulationSession&) = delete;
    WorldSimulationSession& operator=(const WorldSimulationSession&) = delete;

    /// Launches a live 5-domain coupled world simulation from EditorLifecycle's ComponentDefinitions (1-based SlotIndex
    /// matching ComponentClassToken{ classOrdinal + 1u, 1u }) and WorldPlacements (including per-instance overrides).
    [[nodiscard]] bool BeginPlayFromWorldPlacements(
        std::span<const ComponentClass::ComponentClassDefinition> classDefinitions,
        const ComponentClass::ComponentWorldPlacementRegister&    worldPlacements,
        std::string_view                                          sessionTitle = "FrontierWorldPlay",
        std::string*                                              outError = nullptr);

    /// Launches a live 5-domain coupled world simulation directly from a pure-TOML .VolumetricSpace scene file.
    [[nodiscard]] bool BeginPlayFromSceneFile(
        const std::filesystem::path& sceneFilePath,
        std::string*                 outError = nullptr);

    /// Advances the active world simulation by `deltaSeconds` with the viewer's live world camera position.
    void Tick(double deltaSeconds, const std::array<float, 3>& cameraWorldPos = { 4.10f, 2.35f, 4.95f }) noexcept;

    [[nodiscard]] bool Pause() noexcept;
    [[nodiscard]] bool Resume() noexcept;
    void               Stop() noexcept;

    /// Edit-mode / runtime fast-skip toggle (when false, Tick() returns immediately in 0.00 ms).
    void               SetShouldUpdate(bool shouldUpdate) noexcept;
    [[nodiscard]] bool QueryShouldUpdate() const noexcept { return ShouldUpdate_; }

#if FRONTIER_FLUID_HAS_VULKAN
    /// Attaches a live Vulkan 1.2 interchange for Deformable, Fluid, and Particle async GPU execution.
    [[nodiscard]] bool AttachVulkanInterchange(GeometricRaster::VulkanInterchange* interchange) noexcept;
#endif

    [[nodiscard]] PreviewRunStatus QueryStatus() const noexcept { return RunStatus_; }
    [[nodiscard]] bool             QueryPlaying() const noexcept { return RunStatus_ == PreviewRunStatus::Running; }
    [[nodiscard]] const PrefabAssemblyReport&    InspectAssemblyReport() const noexcept { return AssemblyReport_; }
    [[nodiscard]] const SimulationSceneDocument& InspectActiveScene() const noexcept { return ActiveScene_; }
    [[nodiscard]] SimulationCoordinatorTelemetry InspectTelemetry() const noexcept;
    [[nodiscard]] SimulationExecutionRouteSummary InspectExecutionRoutes() const noexcept;

    /// Extracts current deformed world-space triangle surfaces for all active SoftBody instances.
    [[nodiscard]] std::vector<PreviewSurfaceSnapshot> ExtractDeformableRenderSurfaces() const;

    /// Serializes the active world scene document to pure-TOML text.
    [[nodiscard]] std::string ExportActiveSceneToToml() const;

    [[nodiscard]] const SimulationCoordinatorExtension* InspectCoordinator() const noexcept { return Coordinator_.get(); }

private:
    [[nodiscard]] bool MountPreparedScene(const SimulationSceneDocument& sceneDoc, std::string* outError);

    PreviewRunStatus                                RunStatus_      = PreviewRunStatus::Idle;
    bool                                            ShouldUpdate_   = true;
#if FRONTIER_FLUID_HAS_VULKAN
    GeometricRaster::VulkanInterchange*             Interchange_    = nullptr;
#endif
    PrefabAssemblyReport                            AssemblyReport_ {};
    SimulationSceneDocument                         ActiveScene_    {};
    std::unique_ptr<SimulationCoordinatorExtension> Coordinator_    {};
    std::unique_ptr<Lifecycle::LifecycleSequence>   Sequence_       {};
};

} // namespace Frontier::SimulationSpace
