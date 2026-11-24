# Frontier Engineering Backlog (Deferred Items & Ground-Up Redo Queue)

Tracks intentionally deferred tasks, ground-up domain redos, platform follow-ups, and scheduled
refactoring so active milestones stay focused and zero technical debt goes undocumented. Move items
to [Tracker.md](Tracker.md) when scheduled for active implementation.

---

## 1. Ground-Up Redo Queue (Removed Simplifications)

| ID | Domain | Queued Redo Scope | Baseline & Gate |
| -- | ------ | ----------------- | --------------- |
| `BKL-012` | `DistanceFieldSpace/` · `GeometricRaster/` · `Shaders/` · `Exhibits/` | **Completed (`2026-11-20`) — Three-Order World Rendering ([`WorldRenderingRedoPlan.md`](Plans/WorldRenderingRedoPlan.md) · [`WorldRenderingRedoReport.md`](Plans/WorldRenderingRedoReport.md))**: Phase 0 SDF/`.fvol` purge (`757b5c6`), Order 1 Standalone Direct PBR + `.ies` photometry (`4294a70`), Order 2 `GtxLowEnd` (`<= 3.7 MB` local `C0` SDF) + `HighEnd` (`0 B` SDF `BVH4` refit) Real-Time GI (`18b3329`), and Order 3 Unified Vulkan 1.3 + Slang Deterministic Path Tracer (`GameRealtime` & `SceneFinalRender`) + `DeterministicPathTracerPane` (`DeterministicPathTracerValidation`). | Moved to [`Tracker.md`](Tracker.md) Phase 6. |
| `BKL-013` | `SimulationSpace/` · `Workspaces/` · `Shaders/` | **Solver / Shader / Codec / Coupling Backbone Complete (`524/524` PASS); Stage E UI Deferred (`S19`–`S23`)**: Native C++20 + `.slang` `ForceFields`, `Fluid` (3D Eulerian + Sparse Infinite + `54` `.FluidGrid` presets), `ParticleSpace`, and `Deformable` XPBD/Neo-Hookean FEM/Cage/VAT (`D01`–`D26`, `9` `.SoftBody` presets, `2` shipped `.VertexAtlas` assets, `SimulationCoordinatorExtension`, `SimulationSceneDocument`, and `37` `.slang` shaders wired in `CMakeLists.txt`) are shipped and validated (`524/524` PASS). Only Stage E editor UI panels (`S19`–`S23`) remain deferred pending user UI design. | See [`UnifiedFluidParticleForceFieldPortPlan.md`](Plans/UnifiedFluidParticleForceFieldPortPlan.md) & [`DeformableFemVatArchitecturePlan.md`](Plans/DeformableFemVatArchitecturePlan.md). |

---

## 2. Platform & Packaging Follow-Ups

| ID | Domain | Deferred Item | Target Trigger |
| -- | ------ | ------------- | -------------- |
| `BKL-001` | `Windowing/` · `Automation/` | Android NDK `GameActivity` / `ANativeWindow` packaging script (`PackageAndroidApk.py`) and Gradle-free APK/AAB bundler for `Shipping` builds. | After desktop + headless `PlatformWindow` & Vulkan surface validation complete. |
| `BKL-002` | `Windowing/` | Native Wayland `pointer-constraints-unstable-v1` and `relative-pointer-unstable-v1` cursor lock for fly-camera viewport navigation in `PlatformWindowWayland.cpp`. | Phase 2 Viewport Camera Navigation. |
| `BKL-003` | `Automation/Workflows/` | Cross-platform `Shipping` binary packaging workflow templates (Windows x64, Linux x64, Android ARM64). Keep retained under `Automation/Workflows/` (inactive) until explicit GitHub Actions approval. | First `SampleProject` `Shipping` milestone. |

---

## 3. Rendering, `.slang` Shaders & GPU Overlays

| ID | Domain | Deferred Item | Target Trigger |
| -- | ------ | ------------- | -------------- |
| `BKL-004` | `Shaders/Compiler/` | Live file-modification timestamp polling for `.slang` hot-recompilation in `Editor` and `Development` profiles (compiled out in `Shipping`). | Phase 3 `ShaderCompiler` implementation. |
| `BKL-005` | `Annotation/` · `GeometricRaster/Gizmo/` | Dedicated `.slang` anti-aliased infinite-grid shader (`NavigationOverlay`), analytical 3D gizmo shader (`TransformGizmo`), and GPU SDF text callout rasterizer (`TextAnnotation`). | Phase 2–3 GPU Viewport Overlay bringup. |
| `BKL-006` | `Components/VectorIcons/` | GPU-resident analytical curve/icon sheet rasterizer (`VectorRasterizer`) superseding offline CPU ThorVG icon baking where dynamic resolution scaling is required. | Phase 3 UI & VectorIcon bringup. |

---

## 4. Input, Audio, Networking & Naming Remediation

| ID | Domain | Deferred Item | Target Trigger |
| -- | ------ | ------------- | -------------- |
| `BKL-007` | `Input/` | Multi-finger gesture chord recognizer (pinch-zoom, two-finger pan/orbit) over `InputExchange` touch streams for Android and touchscreen laptops. | After core `InputExtension` desktop/touch intake lands. |
| `BKL-008` | `Experimental/` → `Engine/` | Native ISO C++20 + `.slang` conversion of remaining staged reference studios (`PhotometricEditor`, `VehicleLab`, `ClothEditor`, `GeometryCompiler`, `LevelDesigner`). *(Active 9 host workspaces in `Engine/Workspaces/` & `Engine/Lifecycle/`: `FrontierWorld`, `UV`, `Pattern`, `Stamp`, `Baking`, `Fracture`, `Sketcher`, `Material`, `ComponentEditor`.)* | Dedicated studio conversion milestones under `CLAUDE.md` §2.1. |
| `BKL-009` | `AudioSpace/` | Completed — `AudioHardwareOutput` ships Windows `WASAPI`, runtime-loaded Linux `PulseAudio`/`ALSA`, Android `AAudio`, and deterministic `HeadlessBuffer` WAV export (`AudioSpaceValidation`). | Moved to `Tracker.md`. |
| `BKL-010` | `NetworkingSpace/` | Optional DTLS / encrypted datagram framing over `SocketTransport` for WAN `Shipping` sessions. | `NetworkingExtension` Phase 2. |
| `BKL-011` | `Engine/Workspaces/` · `Engine/Editor/` · `Engine/Lifecycle/` | Remediate audited C++ banned-vocabulary violations catalogued in workspace `README.md` files (notably prohibited `<K-suffix>` symbols in `ParametricSketcher/`, `ComponentEditor/`, `UV/`, `Fracture/`, `Texturing/`, plus `State`/`Frame`/`Card`/`Pill`/`Badge`/`Draw*`/`Get*`/`Set*` identifiers). | Dedicated naming-compliance refactor passes. |
