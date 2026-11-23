# Frontier Format Registry & Naming Plan — Design Draft for Review

> **Status:** Proposal only. Nothing in `egoistinbox-hue/Frontier` has been modified. This document is the "show me first" deliverable requested on 2026-11-23 before any rename is applied.
>
> **Scope:** Every persisted format in Frontier (native `.Space` containers, embedded sub-formats, composite `.Instance` containers, configuration/session files, interchange), the codec naming law, the embedded-section palette, and a 5+ alternative shortlist per category.
>
> **Grounding:** Derived from `Documentation/Plans/DocumentFormatsInstancePlan.md`, `DocumentSpacesRevisionPlan.md`, `FolderStructure.md`, `BannedWordsVocabulary.md`, `ImportantNotes.md`, `Engine/Codecs/` (current codecs), `Content/ContentRepositoryIndex.ProjectSpace` (current on-disk extensions) and a full-path sweep of branch `arena/2809d087-frontier`.

---

## 0. Audit — What Is Wrong With the Names Claude Produced

### 0.1 Borrowed / overloaded container words (fix or retire)

| Current | Problem | Verdict |
| :--- | :--- | :--- |
| `.VectorGraphics` / `VectorGraphics…` | Wordier than the mechanism it names; the established container is `.VectorSpace` with `VectorCodec` | **Keep `.VectorSpace`; never introduce `.VectorGraphics`** |
| `.CurveSpace` | A curve is a section inside a container, not a self-contained runtime universe | **Never a standalone container — embedded `.PathCurve` / `.Curve` sections only** |
| `Environment` (as a format/category word) | Concrete enough for a content folder, meaningless as a format noun | **Files stay `.VolumetricSpace`; folder becomes `Content/Atmosphere/`** |
| `.EditorConfiguration` | Wrong decomposition; editor session prefs and engine startup are different beasts | **Retired → `.RuntimeConfiguration` + `.EngineSettings`** |
| `.ProjectSpace` (as exposed to users) | One mega-file trying to be everything | **Split into the configuration family (§6)** |
| `PixelSheetCodec` | "Sheet" carries zero mechanism; collides with nothing and explains nothing | **`.PixelSpace` (§4.4)** |
| `SkyAtmosphereCodec` | Double-barreled; "Sky" is a content bookmark, "Atmosphere" is the mechanism | **`.Atmosphere` + `AtmosphereCodec`** (§4.7) |
| `LensApertureCodec` | An aperture stop is one element of a lens; the container holds the **entire** lens | **`.Lens` + `LensCodec`** (§4.8) |
| `FractureTreeCodec` | "Tree" misnames a branching break-pattern network | **`.FractureNetwork` + `FractureNetworkCodec`** (§4.9) |
| `.VectorField` vs `.ForceField` | Both were floated; the storage mechanism and the applied consumer were conflated | **Storage = `.VectorField`; application label = `.ForceField` section** (§6.4) |

### 0.2 Banned words found in the tree (must be renamed)

| Path | Banned word | Replacement |
| :--- | :--- | :--- |
| `Engine/MaterialSpace/Dynamic/DynamicMaterialRecipe.{h,cpp}` | `Recipe` (banned) | `DynamicMaterialProcedure.{h,cpp}` |
| `Exhibits/DynamicMaterialRecipeValidation.cpp` | `Recipe` | `DynamicMaterialProcedureValidation.cpp` |
| `Experimental/FrontierWorldMirror/InspectorDepot/Provenance.json` | `Provenance` (banned) | `Origin.json` |
| `Experimental/ProjectZeroEditor/InspectorDepot/Provenance.json` | `Provenance` | `Origin.json` |
| `Documentation/Plans/WorldSerializationDagPlan.md` (tracked plan file) | `Dag` (banned) | `SerializationArchitecturePlan.md` / `WorldSerializationNetworkPlan.md` |
| `Engine/Workspaces/Revision/README.md` — "Revision DAG architecture" | `Dag` | "Revision Network" |
| `Engine/Workspaces/Revision/NonLinearHistory.{h,cpp}` — "directed acyclic revision history tree" | `Dag` / `Tree` | Concept renamed **Revision Network**; optional file rename to `RevisionBranchNetwork.{h,cpp}` |
| `Documentation/Plans/VectorFieldArchitecturePlan.md` — "19 Force Laws", header "Force Laws" | `Law` (banned since 2026-11-14) | "19 Force Models" |
| `Experimental/VehicleProvingGrounds/Archive/` + `FacetArchiveCodec.py`, `Source/FacetCodec.js`, `*.facet` | `Archive` (banned), `Facet` (banned) | `Repository/`, `FaceStreamCodec.{py,js}`, `.face` |
| `EngineContent/EnvironmentArchives/` | `Archive` (banned — only `EngineContent/FontsArchives/` is exempt) | `EngineContent/AtmosphereRepository/` |
| `Engine/GeometrySpace/Parametric/Feature/FeatureTree.{h,cpp}` | `Tree` | `FeatureStack.{h,cpp}` (matches `Stack` vocabulary) or `FeatureNetwork.{h,cpp}` |
| `Content/Environment/` | `Environment` (disliked) | `Content/Atmosphere/{Skies,Fog,Clouds}/` |
| `Content/Optics/StudioShadingSphereSheet.ImageSpace` | `Studio` (banned by user ruling) | `ShadingSphereSheet.ImageSpace` |
| `EngineContent/MatCaps/MatCapStudioAtlas.png` / `.rgba` / `MatCapStudioCatalog.json` | `Studio` | `MatCapAtlas.png` / `.rgba` / `MatCapCatalog.json` |
| `EngineContent/Input/FrontierStudio.InputSpace` | `Studio` | `FrontierDefault.InputSpace` |
| `Engine/Components/Panes/StudioPaneAppearance.h` | `Studio` | `WorkspacePaneAppearance.h` |
| `Engine/Shaders/MaterialSpace/StudioEnvironmentRadiance.slang` | `Studio` / `Environment` | `ImageBasedRadiance.slang` |
| `Engine/Components/PixelSpace.{h,cpp}` | Collides with proposed `.PixelSpace` format | Route rename to **`PixelCanvas.{h,cpp}`** (it *is* a 2D drawing canvas) — frees `.PixelSpace` for the raster format |
| `Automation/ServeMarkdownStudio.py`, `Documentation/Plans/MarkdownStudioViewer.html`, `Experimental/*/` `Studio*` (~60 paths) | `Studio` | Viewer/Workspace equivalents; full sweep listed in §10 |
| `Documentation/Plans/WorldMirrorInspectorCarouselReport.md` | `Carousel` (banned) | Rename report file |
| `Engine/NetworkingSpace/NetworkEndpointRegister.cpp` — "Peer telemetry" comment | `Peer` (banned by user ruling) | "endpoint telemetry" |
| `.Pigment` · `.Sketch` · `.Assembly` (proposed formats) | `Assembly` banned (use `PolygonNetwork`/`ComponentSpace`) | `.Assembly` **retired**; `.Pigment`, `.Sketch` retained (no banned word) |
| `.facet` source files (Experimental/Shared/Models, UVEditor, DeterministicPathTracer) | `Facet` | `.face` |

### 0.3 Structural shortcomings

1. **Everything ends in `Space`.** Camera optics, luminaires, project configuration and render jobs are not self-contained runtime universes — `Space` must be earned (§2).
2. **No composite container.** "Embedded" was hand-waved as "TOML sections" with no rule for when bytes are inline vs referenced, and no first-class "a vehicle is one document" answer (`VehicleDefault.Instance`).
3. **One layer conflated storage with policy** (`VectorField` = storage mechanism; `ForceField` = physics application). Now separated (§6.4).
4. **No provenance answer.** Something was needed for "who/what generated this file" — the answer is `.Origin`, not `Provenance`.
5. **Config sprawl.** One `.ProjectSpace` mega-file tried to hold theme, layout, shortcuts and dependencies. Now split into the five-file configuration family the owner specified.

---

## 1. The Naming Laws (superseding all prior drafts)

**Law 1 — The Space Suffix Is Earned, Not Default.**
`.Space` is reserved for a **first-class runtime document category**: a self-contained binary document the engine loads, executes and may edit in its own workspace, with nothing higher than it. Undisputed examples — Geometry, Particles, Physics, Audio. Camera optics, project settings, lens definitions, luminaires, origin records: **no `Space`**.

**Law 2 — Two-Tier Hybrid Framing (unchanged).**
UTF-8 PascalCase TOML `.DocumentHeader` on top; 64-byte-aligned binary sections below with FNV-1a checksums and a byte-offset directory. Case-insensitive load, canonical PascalCase write.

**Law 3 — Codecs, Interchange, Exchange (unchanged).**
`*Codec` = native container read+write (`*Encoder` + `*Decoder`). `*Interchange` = external industry format or GPU API boundary. `*Exchange` = live in-memory/OS bi-directional transfer. Codec name = container name minus the leading dot, full length (`AtmosphereCodec`, `LensCodec`, `PhysicsSpaceCodec`); legacy short names (`GeometryCodec`) grandfathered, normalized opportunistically.

**Law 4 — Embedding Is a Contract, Not a vibe.**
Any container may embed another container as (a) **inline payload** (small, self-contained: parameters, rules, assignments) or (b) **`.ContentReference`** (path + FNV-1a hash + section map) for large payloads (images, geometry, audio). Same logical document either way; the manifest says which. Never copy megabytes into a TOML header, never reference what you cannot hash.

**Law 5 — Instance Containers.**
A placed, reusable, self-describing composition (e.g. `VehicleDefault.Instance`) is a first-class container of category **`Instance`**. It references or inlines Tier A containers and carries its own `.InstanceHeader`, identity token, placement and `.Origin`.

**Law 6 — Banned Words (amended 2026-11-23).**
The existing `BannedWordsVocabulary.md` stands, plus the user rulings recorded on 2026-11-23: `Dag`, `Provenance`, `Recipe`, `Studio`, `Peer`, `Tree` (in graph/fracture/feature contexts — use `Network` / `Stack` / `Timeline`), and `Script` (in format/container names for logic — use `.Behavior`). `VectorGraphics`-style phrasing is rejected in favor of `.VectorSpace`. See §9 for the amendment text to be appended to the banned-words ledger.

**Law 7 — Mechanism First, Always.**
Name the layout, the transform, the grid, the record. Never the vibe. (`PixelSheet` → rejected; `PixelSpace` → correct.)

---

## 2. The Suffix & Tier System

| Tier | Rule | Examples |
| :--- | :--- | :--- |
| **A — Core Container** | Standalone document; owns a runtime interpretation; `.Space` suffix | `.GeometrySpace`, `.ParticleSpace`, `.PhysicsSpace`, `.AudioSpace` |
| **B — Domain Container** | Standalone document; supporting role; plain domain noun, **no Space** | `.Atmosphere`, `.Lens`, `.Photometry`, `.Terrain`, `.Motion`, `.Pigment`, `.Sketch`, `.Instance`, `.Behavior`, `.FractureNetwork` |
| **C — Configuration** | Project/engine/session configuration family; plain noun, no Space | `.ProjectIndex`, `.EngineSettings`, `.RuntimeConfiguration`, `.PlatformSettings`, `.WorkspaceLayout` |
| **D — Embedded Section** | Never a standalone file; only a dot-section inside Tier A/B/C | `.Revision`, `.Origin`, `.PathCurve`, `.VectorField`, `.ForceField`, `.Fluid`, `.LayerStack`, `.ContentReference` |
| **E — External / Runtime Boundary** | Not a file format at all | `*Interchange` (OBJ/glTF/EXR/SVG…), `*Exchange` (live OS↔UE sync) |

`Space` earning test (all three required): **(1)** loaded and executed/simulated/editable at runtime or in a dedicated workspace, **(2)** meaningful alone on disk, **(3)** nothing higher than it in the containment order. Camera optics fails (3) — a lens is *part of* a camera rig. Project configuration fails (2). Luminaires fail (3) — they serve lights.

---

## 3. Tier A — Core `.Space` Containers (Recommended Registry)

Legend for each row: **Current → Proposed** · (S) = status quo kept, (R) = renamed, (P) = promoted from embedded/child.

| # | Extension | Codec | Contents (dot-sections) | Verdict |
|:---:| :--- | :--- | :--- | :--- |
| 1 | `.GeometrySpace` | `GeometrySpaceCodec` | `.Primitives` `.Points` `.Corners` `.Faces` `.Edges` `.Normals` `.Tangents` `.UV0..UV7` `.Creases` `.UDIM` `.UVTile` `.PathCurve` `.Parameterization` `.Revision` `.Origin` | Keep (S) |
| 2 | `.UVSpace` | `UVCodec` | `.UDIM` `.UVTile` `.TexelDensity` `.Seams` `.Revision` `.Origin` | Keep (S) |
| 3 | `.TextureSpace` | `TextureCodec` | `.ColourSpace` `.UDIM` `.UVTile` `.MipChain` `.ChannelPack` `.BlockEncoding` `.Revision` `.Origin` | Keep (S) |
| 4 | `.MaterialSpace` | `MaterialCodec` | `.ColourSpace` `.BSDF` `.BSSRDF` `.Microfacet` `.Fresnel` `.Anisotropy` `.ThinFilm` `.Clearcoat` `.Sheen` `.Transmission` `.Dispersion` `.Absorption` `.Subsurface` `.ClosureNetwork` `.LayerStack` *(ref `.TextureSpace` / `.ImageSpace`)* `.Revision` `.Origin` | Keep (S) |
| 5 | `.PixelSpace` | `PixelSpaceCodec` | `.ColourSpace` `.Exposure` `.Tone` `.HDRI` `.LayerStack` `.Tiles` `.BlockEncoding` `.Revision` `.Origin` | **(R) from `.ImageSpace`** — see §4.4 |
| 6 | `.VectorSpace` | `VectorCodec` | `.PathCurve` `.StrokeAppearance` `.Stencil` `.LayerStack` `.ColourSpace` `.Revision` `.Origin` (`.SvgXml` import payload only) | Keep (S); `.VectorGraphics` rejected |
| 7 | `.ColourSpace` | `ColourCodec` | `.Primaries` `.WhitePoint` `.TransferCurve` `.Gamut` `.Spectral` `.Origin` | Keep (S) |
| 8 | `.VolumetricSpace` | `VolumetricSpaceCodec` | `.SignedDistance` `.DensityGrid` `.Absorption` `.Scatter` `.Phase` `.Cloud` `.Revision` `.Origin` | Keep (S) |
| 9 | `.ParticleSpace` | `ParticleSpaceCodec` | `.SpawnSource` `.VectorField` `.ColourRamp` `.Integration` `.Trail` `.Fibre` `.Revision` `.Origin` | Keep (S) — user-specified `VectorField + colours` core set |
| 10 | `.PhysicsSpace` | `PhysicsSpaceCodec` | `.RigidDynamics` `.Constraints` `.Collision` `.Fluid` `.ForceField` `.Motion` `.Revision` `.Origin` | **(R) from `.SimulationSpace`** (§4.10) |
| 11 | `.AudioSpace` | `AudioSpaceCodec` | `.Waveform` `.BusLayout` `.AcousticProfile` `.RoomImpulse` `.Revision` `.Origin` | **(R) from `.Acoustic`** (§4.11) |
| 12 | `.FontSpace` | `FontCodec` | `.TypefaceGroup` `.WeightList` `.Metrics` `.Origin` | Keep (S) |
| 13 | `.WorldSpace` | `WorldSpaceCodec` | `.InstancePlacement` `.Environment` *(ref `.Atmosphere`)* `.Revision` `.Origin` | Keep (S) — see note below |
| 14 | `.ComponentSpace` | `ComponentSpaceCodec` | `.ComponentClass` `.SlotRegister` `.PhysicsBinding` `.Thumbnail` `.Revision` `.Origin` | Keep (S) — replaces retired `.Assembly` |

**Borderline — `Space` retained by default, owner may veto to Tier B:** `.WorldSpace`, `.ComponentSpace`, `.InputSpace`, `.NetworkSpace`, `.RenderSpace`. None of these are load-bearing for the current debate; each passes all three Law-1 tests (runtime-executed, standalone, nothing higher), so they stay Tier A unless you rule otherwise.

**Retired names:** `.SimulationSpace` (→ `.PhysicsSpace`), `.Acoustic` (→ `.AudioSpace`), `.ImageSpace` (→ `.PixelSpace`, pending §4.4), `.Assembly` (banned word; capability absorbed by `.ComponentSpace`), `.SimulationSpace` codec `SimulationCodec` (→ `PhysicsSpaceCodec`).

---

## 4. Per-Category Alternatives & Rulings (5+ each)

### 4.1 Geometry container
Current: `.GeometrySpace`. Problem: none. Alternatives auditioned:

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.GeometrySpace`** 🏆 | Matches repo, codecs, content, docs. |
| 2 | `.PolygonSpace` | Emphasizes half-edge interior; hides primitives. |
| 3 | `.ShapeSpace` | Vague ("shape" covers 2D contours). |
| 4 | `.SolidSpace` | Excludes open meshes/curves. |
| 5 | `.PrimitiveSpace` | Excludes authored n-gons. |
| 6 | `.FormSpace` | Circular vibe, mechanism-free. |

**Ruling: `.GeometrySpace` (status quo).**

### 4.2 Raster pixel image container — *the PixelSpace question*
Current: `.ImageSpace` (+ `ImageCodec`). Owner asked: wasn't it supposed to be `PixelSpace`?

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.PixelSpace`** 🏆 | Says exactly what it is: a pixel surface. Frees `Photometry`. |
| 2 | `.ImageSpace` | Status quo; content index uses 5 files; fine but blander. |
| 3 | `.RasterSpace` | Accurate (raster grid); less friendly. |
| 4 | `.BitmapSpace` | Dated; implies 1-bit lineage. |
| 5 | `.CanvasSpace` | Collides conceptually with `PixelCanvas` (UI surface). |
| 6 | `.PixelSheet` | Rejected — owner-flagged "Sheet". |
| 7 | `.ChannelSpace` | Implies packed channel semantics, not pixels. |
| 8 | `.TexelSpace` | Implies texture (lives in `.TextureSpace`). |

**Ruling: `.PixelSpace`.** Requirement: `Engine/Components/PixelSpace.{h,cpp}` (the UI 2D drawing canvas) renames to **`PixelCanvas.{h,cpp}`** first — canvas is the honest word for it, and it frees `PixelSpace` for the file category. References to update: `Components/README` mentions, ImGui panes that include `PixelSpace.h`, and exhibit text. `.ImageSpace` → `.PixelSpace` in `Content/`, `EngineContent/`, `ContentRepositoryIndex.ProjectSpace`, `ImageCodec` → `PixelSpaceCodec`. **If you veto the UI rename, keep `.ImageSpace` and `ImageCodec`** — do not accept the name collision.

### 4.3 Vector graphics container
Current: `.VectorSpace`. Owner flagged `.VectorGraphics` (wordy) and `VectorField` (wrong layer).

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.VectorSpace`** 🏆 | Status quo; `VectorCodec`; ThorVG path. |
| 2 | `.Contour` | Precise (outer/inner contours); loses stroke semantics. |
| 3 | `.Pathwork` | Coinage; mechanism = paths, but odd. |
| 4 | `.Drawing` | Familiar (SVG = drawing); okay but plain. |
| 5 | `.Figure` | Fine; weaker mechanism signal. |
| 6 | `.Outline` | Only strokes, not fills. |
| 7 | `.Schematic` | Implies diagrams, not artwork. |
| 8 | `.VectorGraphics` | Explicitly rejected (owner). |

**Ruling: `.VectorSpace`.** `.VectorGraphics` formally added to rejected-name ledger.

### 4.4 Curve container (the `.CurveSpace` rejection)
Owner: curves were given their own `Space` — wrong.

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.PathCurve`** (section) 🏆 | Already used by `VectorCodec`; Bézier/Hermite/B-spline payload. |
| 2 | `.Curve` (section) | Generic section for Geometry/Sketch. |
| 3 | `.Spline` | Mechanism-true; but only one curve family (excludes arcs). |
| 4 | `.PolynomialCurve` | Precisest; verbose for a section tag. |
| 5 | `.KnotCurve` | CAD-flavored; narrow audience. |
| 6 | `.StrokePath` | Paint-biased. |

**Ruling: no `.CurveSpace`, ever.** Curves live as embedded `.PathCurve` / `.Curve` sections inside `.VectorSpace`, `.GeometrySpace`, `.Sketch`. Applies to `.Curve` inside `.Pigment` strokes too.

### 4.5 Material container
| # | Candidate | Note |
| 1 | **`.MaterialSpace`** 🏆 | Status quo; 128 B registers, 10-lobe OpenPBR. |
| 2 | `.SurfaceMaterialSpace` | Redundant breadth. |
| 3 | `.ShadingSpace` | Conflates shading (runtime) with material (document). |
| 4 | `.ShaderMaterialSpace` | Shader is the program, not the document. |
| 5 | `.SurfaceSpace` | "Surface" is an overloaded catch-all; rejected. |
| 6 | `.OpenPbrSpace` | Implementation-locked; materials outlive any one model. |

**Ruling: `.MaterialSpace` (status quo).**

### 4.6 Colour science container
| # | Candidate | Note |
| 1 | **`.ColourSpace`** 🏆 | Status quo; CIE XYZ / ACEScg / Rec.2020 / Oklab. |
| 2 | `.ColorSpace` | US spelling; repo standardized on British `Colour`. |
| 3 | `.PrimariesSpace` | Only primaries; white points/transfer curves homeless. |
| 4 | `.GamutSpace` | Gamut ≠ white point ≠ transfer curve. |
| 5 | `.SpectralSpace` | Only spectra; excludes tristimulus. |
| 6 | `.ChromaticSpace` | Conflates chromaticity with colour spaces. |

**Ruling: `.ColourSpace` (status quo).**

### 4.7 Atmosphere container — *SkyAtmosphereCodec is dead*
New container. Holds a **sky + fog + cloud environment set**: analytic atmosphere parameters, LUT factory settings, solar optics profile, wind coupling. (Baked LUTs and density grids themselves stay in `.VolumetricSpace`.)

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.Atmosphere`** 🏆 (owner-directed) | Exact mechanism word; `AtmosphereCodec`; content folder aligned. |
| 2 | `.Sky` | Sky ≠ fog ≠ clouds; too narrow. |
| 3 | `.ClearSky` | Only the clear-day case. |
| 4 | `.SkyDome` | A rendering primitive, not a document. |
| 5 | `.AtmosphereProfile` | "Profile" implies lookup, not document. |
| 6 | `.AerialVolume` | Cinematic; conflates air with fog. |
| 7 | `.PlanetaryAtmosphere` | Precise but pedantic for Earth defaults. |

**Ruling: `.Atmosphere` + `AtmosphereCodec`** (encoder/decoder pair `AtmosphereEncoder`/`AtmosphereDecoder`). Sub-sections: `.TransmittanceLUT` `.MultiScatterLUT` `.SkyViewLUT` `.SolarOptics` `.PhaseFunction` `.WindCoupling` `.CloudCondensate` `.Origin`. Retire: `SkyAtmosphereCodec` (never shipped, now formally dead). Content reorg: `Content/Environment/{Skies,Fog,Clouds}/*.VolumetricSpace` → `Content/Atmosphere/{Skies,Fog,Clouds}/`.

### 4.8 Lens / camera optics container — *LensApertureCodec is wrong*
Owner: "it's supposed to contain info about the entire lens." Aperture stop is **one** element. New container, **no Space** (a lens is part of a camera rig; nothing higher test fails).

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.Lens`** 🏆 | Shortest exact word; `LensCodec`; camera-first audience. |
| 2 | `.Optic` | Broader (mirrors, photometric optics); slightly abstract. |
| 3 | `.CameraLens` | Explicit but redundant for an engine with one camera class of lens. |
| 4 | `.LensStack` | "Stack" = the element list only; not the whole document. |
| 5 | `.OpticalBench` | Laboratory tone; "Bench" = rig, not glass. |
| 6 | `.OpticTrain` | Real optics term (a train of elements); obscure. |
| 7 | `.GlareProfile` | Only the flare side-effect baked by `LensFlareSequence` — scope error. |
| 8 | `.Aperture` | The rejected failure mode; never. |

**Ruling: `.Lens` + `LensCodec`.** Sub-sections (whole-lens document): `.ElementStack` (per-element radius/thickness/material/index) `.ElementSpacing` `.ApertureStop` (blade count, iris curve, min/max f-number) `.CoatingSet` `.ChromaticProfile` (Abbe dispersion, focus shift) `.GlareProfile` (streak/ghost baking recipe) `.FocusScale` `.SensorGate` `.Origin`. Baked optical transfer LUTs referenced via `.ContentReference` only. Camera **bodies** (sensor size, mount, stabilization) remain in the camera component, not `.Lens`.

### 4.9 Fracture network container — *never "Tree"*
New container for pre-fractured break patterns: convex cells + fracture descriptor payloads produced offline, consumed at runtime by `FastRuntimeFracture`.

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.FractureNetwork`** 🏆 (owner-directed) | Matches `PolygonNetwork` precedent; cells + joints. |
| 2 | `.FracturePattern` | Pattern ≈ the look, not the data structure. |
| 3 | `.FractureSet` | Set = unordered; joints/adjacency ARE the value. |
| 4 | `.FractureLattice` | Lattice implies regular spacing; breaks are irregular. |
| 5 | `.FractureField` | Field = continuous sampling; wrong model class. |
| 6 | `.FailureNetwork` | Structural-engineering flavor; fine but cold. |
| 7 | `.BreakNetwork` | Blunt; "Break" as noun acceptable, but Fracture is established. |
| 8 | `.FractureCells` | Cells only; misses bond/joint parameters. |

**Ruling: `.FractureNetwork` + `FractureNetworkCodec`.** Sub-sections: `.ConvexCells` `.BondRegister` `.WeibullDistribution` `.HertzCone` `.Revision` `.Origin`. `FractureTreeCodec` never ships; `FeatureTree.{h,cpp}` → `FeatureStack.{h,cpp}`; "Revision DAG" → **Revision Network** (§5.2).

### 4.10 Physics / simulation container
Owner: physics is core; `.SimulationSpace` proposed split/promotion.

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.PhysicsSpace`** 🏆 | Owner's own taxonomy; Jolt v5.6.0 rigid mechanics + constraints + fluids + fields. |
| 2 | `.SimulationSpace` | Status quo; but "simulation" is everything (UI, audio, networks) — too wide. |
| 3 | `.DynamicsSpace` | Excludes collision/fluid solvers. |
| 4 | `.MechanicsSpace` | Same narrowing; rigid-only flavor. |
| 5 | `.KineticsSpace` | Chemistry-flavored; excludes statics. |
| 6 | `.MotionSpace` | `.Motion` is already the keyframe container (Tier B). |
| 7 | `.RigidSpace` | Excludes fluids and deformables. |

**Ruling: `.PhysicsSpace`.** `Engine/SimulationSpace/PhysicsSpace/` (child) is promoted to top-level `Engine/PhysicsSpace/`; `.Fluid`, `.ForceField`, `.Motion` sections migrate accordingly. `Engine/Codecs/SimulationCodec.{h,cpp}` → `PhysicsSpaceCodec.{h,cpp}`. `Content/*.SimulationSpace` → `.PhysicsSpace`. `Motion` keyframes remain `.Motion` (Tier B, separate container).

### 4.11 Audio container
Owner: audio is core → `Space`.

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.AudioSpace`** 🏆 | Matches `Engine/AudioSpace/`, `AudioThread`, `AudioBusAccumulator`. |
| 2 | `.Acoustic` | Status quo; "acoustic" = sound treated as physics; fine but second-class next to Audio*. |
| 3 | `.AcousticSpace` | Space + Acoustic; waxes pedantic. |
| 4 | `.SoundSpace` | Consumer flavor; "Sound" vague vs engine's acoustic precision. |
| 5 | `.WaveformSpace` | Waveforms are payloads, not the universe. |
| 6 | `.Audio` | Too terse; loses tier signal. |

**Ruling: `.AudioSpace` + `AudioSpaceCodec`.** `.Acoustic` retired. `AcousticCodec.{h,cpp}` → `AudioSpaceCodec.{h,cpp}`; `.AcousticFile` section vocabulary (`.AcousticProfile`) retained as sub-sections since "acoustic profile" is the correct term inside the audio category.

### 4.12 UV parameterization container
| # | Candidate | Note |
| 1 | **`.UVSpace`** 🏆 | Status quo; `UVCodec`; UDIM/1001 addressing. |
| 2 | `.UnwrapSpace` | Process name, not artifact. |
| 3 | `.ParameterSpace` | Loses UV meaning; too generic. |
| 4 | `.TexelSpace` | Texel density is one section. |
| 5 | `.TileSpace` | Reads as mip tiles. |
| 6 | `.ChartSpace` | `.UVChart` was explicitly banned in favor of `.UVTile`. |

**Ruling: `.UVSpace` (status quo).**

### 4.13 Volumetric container
| # | Candidate | Note |
| 1 | **`.VolumetricSpace`** 🏆 | Status quo; SDF / density / participating media. |
| 2 | `.VolumeSpace` | Shorter; fine, but loses adjective. |
| 3 | `.DensitySpace` | Density is one payload class; excludes SDF semantics. |
| 4 | `.SignedDistanceSpace` | SDF-only; excludes fog/clouds. |
| 5 | `.GridSpace` | Bare grid = nothing. |
| 6 | `.FieldSpace` | See §4.16 — field is a vector mechanism, not a volume. |

**Ruling: `.VolumetricSpace` (status quo).**

### 4.14 Texture container
| # | Candidate | Note |
| 1 | **`.TextureSpace`** 🏆 | Status quo; channel-packed UDIM mip chains. |
| 2 | `.SurfaceMapSpace` | Map(s) = banned word (`map`). |
| 3 | `.ChannelSpace` | Channels are sections, not documents. |
| 4 | `.MipSpace` | Mip chains are sections. |
| 5 | `.TexelSpace` | See 4.12 — rejected. |
| 6 | `.SamplerSpace` | Sampler is a GPU unit, not a document. |

**Ruling: `.TextureSpace` (status quo).**

### 4.15 Font / typography container
| # | Candidate | Note |
| 1 | **`.FontSpace`** 🏆 | Status quo; sfnt outlines + metrics. |
| 2 | `.TypefaceSpace` | More precise but verbose; breaks short-extension convention. |
| 3 | `.TypographySpace` | Typography = discipline, not document. |
| 4 | `.GlyphSpace` | `glyph` banned. |
| 5 | `.Font` | Terse; works but loses tier signal next to Pass/Fail audits. |
| 6 | `.OutlineSpace` | Typeface-only; rejects bitmap strikes. |

**Ruling: `.FontSpace` (status quo).**

### 4.16 Force & fluid fields — *VectorField or ForceField?*
Owner asked the direct question. Answer: **both, at different layers, never conflated.**

- **`.VectorField` (Tier D, and optionally Tier B standalone)** = the *storage mechanism*: a sampled or analytic vector-valued field (curl noise, wind, velocity, magnetic). Lives as an embedded section inside `.ParticleSpace`, `.PhysicsSpace`, `.VolumetricSpace`; reusable field libraries may ship standalone as `.VectorField`.
- **`.ForceField` (Tier D section of `.PhysicsSpace`)** = the *application*: a `ForceModel` (LinearWind, CurlNoise, Lorentz…) bound to a `.VectorField` with magnitude, falloff and coupling registers. "Field that pushes things."
- **`.Fluid` (Tier D section of `.PhysicsSpace`)** = solver configuration (grid, phase coupling, viscosity, surface tension). Baked fluid state = `.VolumetricSpace`. There is no `.FluidSpace` — a fluid is a regime of physics, not a document category.
- Never write `.FlowField` (**`flow` banned**). Never `.Field` bare (says nothing).

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.VectorField`** (mechanism) + **`.ForceField`** (section) 🏆 | Separation of storage vs application. |
| 2 | `.VectorFieldSpace` | Space inflation; owner's suffix law forbids. |
| 3 | `.ForceField` alone | Narrower than reality (velocity ≠ force). |
| 4 | `.Field` bare | Mechanism-free. |
| 5 | `.VelocityField` | One quantity; diverges for magnetic/curl fields. |
| 6 | `.FlowField` | **`flow` banned.** |
| 7 | `.ForceLattice` | Lattice ≠ arbitrary sampling. |
| 8 | `.StreamField` | Particle-specific; excludes analytic models. |

**Ruling: `.VectorField` storage sections + `.ForceField` application sections; standalone `.VectorField` allowed for reusable baked field volumes; never `Space`.**

### 4.17 Particles container
| # | Candidate | Note |
| 1 | **`.ParticleSpace`** 🏆 | Status quo; user's anchor example. |
| 2 | `.ParticulateSpace` | Pedantic synonym. |
| 3 | `.EmitterSpace` | `emitter` banned. |
| 4 | `.MoteSpace` | Poetic; not mechanism. |
| 5 | `.SprinklerSpace` | Hardware impression; wrong scope. |
| 6 | `.PointCloudSpace` | Point clouds ≠ simulated particles. |

**Ruling: `.ParticleSpace` (status quo).** Embedded core (owner-specified): `.SpawnSource` `.VectorField` `.ColourRamp` *(the "colours")* `.Integration` `.Trail` `.Fibre` `.Revision` `.Origin`.

### 4.18 Natural-media paint container
| # | Candidate | Note |
| 1 | **`.Pigment`** 🏆 | Status quo; bristle/dab dynamics, stylus curves. |
| 2 | `.PaintSpace` | Space it doesn't earn; word is also bland. |
| 3 | `.NaturalMedia` | Two words; not a mechanism. |
| 4 | `.StrokeSpace` | Strokes are operations, not documents. |
| 5 | `.Bristle` | One subsystem of the document. |
| 6 | `.MediumSpace` | Collides with artistic "medium"; vague. |

**Ruling: `.Pigment` (status quo).**

### 4.19 CAD sketch container
| # | Candidate | Note |
| 1 | **`.Sketch`** 🏆 | Status quo; parametric curves + constraints. |
| 2 | `.CurveSet` | Curves without constraints are not CAD. |
| 3 | `.ParametricSketch` | Redundant modifier. |
| 4 | `.DrawingSheet` | `sheet` rejected; Drawing implied 2D-only. |
| 5 | `.ProfileSpace` | Profile ⊂ sketch; misses constraints. |
| 6 | `.Boundary` | Bare noun; say nothing. |

**Ruling: `.Sketch` (status quo).**

---

## 5. Tier B — Standalone Domain Containers (Plain Noun, No `Space`)

All are first-class on-disk documents that **fail** the Law-1 "nothing higher" test or serve supporting roles. Codecs follow Law 3.

| # | Extension | Codec | Role | Sub-sections |
|:---:| :--- | :--- | :--- | :--- |
| 1 | `.Atmosphere` | `AtmosphereCodec` | Sky/fog/cloud environment set (§4.7) | `.TransmittanceLUT` `.MultiScatterLUT` `.SkyViewLUT` `.SolarOptics` `.PhaseFunction` `.WindCoupling` `.CloudCondensate` `.Origin` |
| 2 | `.Lens` | `LensCodec` | **Entire** camera lens document (§4.8) | `.ElementStack` `.ElementSpacing` `.ApertureStop` `.CoatingSet` `.ChromaticProfile` `.GlareProfile` `.FocusScale` `.SensorGate` `.Origin` |
| 3 | `.FractureNetwork` | `FractureNetworkCodec` | Pre-fractured break patterns (§4.9) | `.ConvexCells` `.BondRegister` `.WeibullDistribution` `.HertzCone` `.Revision` `.Origin` |
| 4 | `.Instance` | `InstanceCodec` | Composite placed composition — `VehicleDefault.Instance` (§6.1) | `.InstanceHeader` `.ContentReference`* `.TransformPlacement` `.MaterialAssignment` `.BehaviorBinding` `.Origin` |
| 5 | `.Behavior` | `BehaviorCodec` | Logic/hook rule sets bound to Instances (§5.1) | `.RuleSet` `.Directive` `.InputBinding` `.TimerRegister` `.OutputBinding` `.Origin` |
| 6 | `.Photometry` | `PhotometryCodec` | Luminaire intensity distribution (LM-63 `.ies` successor) | `.CandelaGrid` `.SymmetryFolder` `.LuminaireHeader` `.Origin` |
| 7 | `.Motion` | `MotionCodec` | Keyframe/FCurve tracks | `.Track` `.Keyframe` `.Easing` `.LoopRegion` `.Revision` `.Origin` |
| 8 | `.Terrain` | `TerrainCodec` | Heightfield + biome scatter | `.Heightfield` `.SplatWeights` `.BiomeScatter` `.WaterLevel` `.Revision` `.Origin` |
| 9 | `.Pigment` | `PigmentCodec` | Natural-media paint libraries (§4.18) | `.Stroke` `.Brush` `.Dab` `.PigmentRamp` `.Stylus` `.ColourSpace` `.Revision` `.Origin` |
| 10 | `.Sketch` | `SketchCodec` | Parametric CAD sketches (§4.19) | `.PathCurve` `.ConstraintSet` `.Dimension` `.Revision` `.Origin` |
| 11 | `.Parameterization` | *(embedded or standalone)* | Surface UV/parameterization sets — retires `.SurfaceParameterSpace` | `.ParameterGrid` `.SeamProjection` `.DistributionLaw` `.Origin` |
| 12 | `.VectorField` | `VectorFieldCodec` | Reusable spatial field volumes (§4.16) — optional standalone | `.VectorGrid` `.AnalyticModel` `.CurlComponent` `.SamplingKernel` `.Origin` |
| 13 | `.InputProfile` | `InputProfileCodec` *(optional)* — see note | Chord/axis/device profiles — retires `.InputSpace` **if** owner rejects Space for input | `.DeviceProfile` `.AxisCalibration` `.ChordRegister` `.Origin` |

**Note on `.InputProfile` vs `.InputSpace`:** `.InputSpace` works today (`InputCodec`, 3 content files). It fails Law 1 only on a strict reading (nothing higher than it — actually true for input). Keep `.InputSpace` (Tier A borderline) **unless** you want the strictest suffix diet — then `.InputProfile`. Default: **keep `.InputSpace`.**

**Note on `.RenderSpace` / `.NetworkSpace` / `.WorldSpace` / `.ComponentSpace`:** all retained Tier A borderline (§3). None are part of the current dispute; none gain by renaming now.

### 5.1 Behavior container — *the "don't say Script" ruling*
Owner: logic hooks for behavior control, but `Script` is disliked/banned-for-containers.

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.Behavior`** 🏆 | Owner's own intent ("control behavior"); reads naturally in `.BehaviorBinding`. |
| 2 | `.Logic` | Terse and exact; slight mystique ("logic" of what?). |
| 3 | `.Directive` | Mechanism-flavored; implies imperatives, excludes reactive rules. |
| 4 | `.Routine` | Implies scheduled code paths only. |
| 5 | `.Expression` | Expression = value formula, not procedure. |
| 6 | `.Formula` | Math-only flavor. |
| 7 | `.Rules` / `.RuleSet` | Rules are the meat; `.Rules` as container reads odd. Use `.RuleSet` **inside** `.Behavior`. |
| 8 | `.Automation` | CI/CD overtones. |
| 9 | `.Intent` | Goal-flavored; excludes actuation detail. |
| 10 | `.Script` | Explicitly rejected — owner. |
| 11 | `.Automation` | Tooling overtones; rejected as container. |

**Ruling: `.Behavior` + `BehaviorCodec`.** A `.Behavior` document is a deterministic rule/hook set: input bindings (throttle, brake, steer…), state registers (gear, rpm, speed), time-based emitters (shift lights), and output bindings (actuators, HMI channels). Compiled to the same zero-allocation dispatch tables Frontier already uses — no interpreter, no text evaluation at tick time. `.Behavior` embeds as `.BehaviorBinding` inside `.Instance`, or ships standalone for shared vehicle AI logic.

### 5.2 Revision network — *DAG is retired*
| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **Revision Network** 🏆 | Matches `PolygonNetwork` / `FractureNetwork`; branchy, acyclic, merge-free. |
| 2 | Revision Lattice | Lattice = regular tiling; history isn't regular. |
| 3 | Revision Timeline | Hides branching. |
| 4 | Revision Chain | Linear-only connotation. |
| 5 | Revision Weave | Poetic; weave implies interlacing merges. |
| 6 | Revision Branch Register | Wordy; fine internally for a member (`BranchRegister`). |
| 7 | Revision Span | Loses branch semantics entirely. |

**Ruling: "Revision Network".** Every doc/comment/README mentioning `DAG`/`tree` for revision history is updated. `NonLinearHistory.{h,cpp}` optionally renames to `RevisionBranchNetwork.{h,cpp}` (owner call; filename contains no banned word as-is, but the concept name must change). `WorldSerializationDagPlan.md` → `WorldSerializationNetworkPlan.md` (or `SerializationArchitecturePlan.md` — both exist; dedupe at rename time).

### 5.3 Origin record — *Provenance is dead*
| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.Origin`** 🏆 | Shortest mechanism-true answer to "where did this come from"; embedded everywhere. |
| 2 | `.Source` | "Source" also means source code; dual meaning. |
| 3 | `.Derivation` | Precise for computed outputs; blander for authored files. |
| 4 | `.Attribution` | Legal-flavored (licenses), not technical lineage. |
| 5 | `.Authorship` | Person-flavored; misses tool/version. |
| 6 | `.Trace` | Hardware-trace connotation; fine but narrow. |
| 7 | `.SourceStamp` | Hybrid; fine as internal type, awkward section. |

**Ruling: `.Origin`** — embedded section (`.Origin`) in **every** container + standalone `Origin.json` for Experimental mirrors. Contents: generating tool + version, source path(s), content hashes of inputs, authoring workspace, timestamp, schema generation. Replaces both `Provenance.json` files.

### 5.4 Photometry container
Current: `.PhotometrySpace` (7 content files: `.ies` luminaire profiles re-authored). Fails Law 1 (serves lights; nothing higher).

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.Photometry`** 🏆 | Matches Tier B role; `PhotometryCodec` already half-exists (`PhotometrySpaceCodec` → `PhotometryCodec`). |
| 2 | `.PhotometrySpace` | Status quo; Space it doesn't earn. |
| 3 | `.PhotometricProfile` | Honest (it's the profile); longer extension. |
| 4 | `.Luminaire` | Fixture ≠ its intensity distribution; conflates object with data. |
| 5 | `.CandelaDistribution` | Precise grid term; verbose. |
| 6 | `.Goniometry` | True measurement term; obscure to most readers. |
| 7 | `.IesProfile` | Leans on an external spec name; fine as alias, not identity. |

**Ruling: `.Photometry` + `PhotometryCodec`** (rename from `PhotometrySpaceCodec`). Content: 7 files renamed. `.PhotometrySpace` retired. **Veto option:** if minimal-churn wins for content, `.PhotometrySpace` may stay Tier A-borderline — but that breaks Law 1 and is not recommended.

### 5.5 Instance composite container — *how VehicleDefault.Instance works*
| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.Instance`** 🏆 | Owner's anchor term; matches `GeometryInstance` & the 32 B identity tokens. |
| 2 | `.Composition` | Compose is **banned** (verb AND noun). |
| 3 | `.Package` | Shipping/tarball connotation. |
| 4 | `.Bundle` | Similar freight flavor. |
| 5 | `.Prefab` | Unity-ism; not mechanism-first. |
| 6 | `.Archetype` | Implies class-of-thing; close but abstract. |
| 7 | `.Edition` | Publishing term. |
| 8 | `.Specimen` | Laboratory tone. |
| 9 | `.Kit` | Parts-bag connotation; misses placement. |

**Ruling: `.Instance` + `InstanceCodec`.** `.Instance` = a placed, reusable composition document. Two embedding modes (Law 4): **inline** (`.MaterialAssignment`, `.BehaviorBinding`, `.TransformPlacement`) or **`.ContentReference`** (path + FNV-1a + byte length + section map) for bulk payloads (`.PixelSpace`, `.GeometrySpace`, `.TextureSpace`). Never duplicate bulk bytes; always hash references.

### 5.6 Origin / Revision / codec-stated invariants (no alternatives needed — mechanism is fixed)
- `.Revision` — embedded operation stream; `RevisionCodec`; never standalone.
- `.Baseline` — folded prefix operations; embedded; `RevisionCodec`.
- `.DocumentHeader` / `.BinarySectionDirectory` — universal Two-Tier framing sections.
- `.ContentReference` — `{ RelativePath, FNV1a64, ByteLength, SectionMap }`.
- `.InstanceHeader` — `{ IdentityToken(32B), DocumentTitle, SchemaVersion, CategoryOrdinal }`.

---

## 6. Tier C — The Configuration Family (replaces `.ProjectSpace` + `EditorConfiguration`)

Owner directive: `StudioLayout → WorkspaceLayout`; configuration splits into **`EngineSettings` + `RuntimeConfiguration` + `PlatformSettings` + `ProjectIndex`**. These five files (none carries `Space`) fully replace the old monolithic `.ProjectSpace` and the disliked `.EditorConfiguration`.

### 6.1 `.ProjectIndex` — the project spine
The identity card: what this project is, what it depends on, what opens at launch. Read at startup; tiny; TOML-only (no binary tier required).

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.ProjectIndex`** 🏆 (owner-sanctioned) | "Index of the project" — dependencies, startup, ordering. |
| 2 | `.ProjectSpace` | Retired per owner; also fails decomposition. |
| 3 | `.ProjectCharter` | Governance tone; charter ≠ machine-read index. |
| 4 | `.ProjectDocument` | Vague; "document" is the universal suffix. |
| 5 | `.ProjectDossier` | Paperwork tone. |
| 6 | `.ProjectLedger` | Ledger ≈ append-only log; index is not a log. |
| 7 | `.ProjectRegister` | Register ≈ table of contents of components; close, but ProjectIndex is owner's word. |

**Sections:** `.DocumentHeader` `.ProjectIdentity` `.StartupTarget` `.DependencyLock`* `.DependencyLock` entries carry `RelativePath + FNV1a64 + PinnedVersion` (content-addressed, git-diffable) `.DefaultInstance` (optional — e.g. `VehicleDefault.Instance`) `.SchemaGeneration`. **Retire:** `.ProjectSpace` / `.projectspace` (loader keeps a case-insensitive read alias for one deprecation window, then hard error with migration message). `ProjectCodec` → `ProjectIndexCodec`.

### 6.2 `.EngineSettings` — engine-wide physical defaults
| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.EngineSettings`** 🏆 (owner-sanctioned) | Settings = scalar defaults the engine owns. |
| 2 | `.EngineConfiguration` | "Configuration" reserved for per-profile run files (§6.3). |
| 3 | `.EngineDefaults` | "Defaults" undersells enforcement. |
| 4 | `.EnginePolicy` | Policy = rules; settings = values. |
| 5 | `.EngineConstants` | Constants are compile-time headers, not files. |
| 6 | `.EngineProfile` | Profile reserved for run profiles. |
| 7 | `.EngineConfigurationFile` | Redundant suffix. |

**Sections:** `.TickBudget` (fixed-step rate, slice budgets) `.MemoryCeiling` (215 MiB / 750 MiB revision caps, arena capacities) `.Threading` (ExecutionChannel allocation, watchdog thresholds) `.GpuPolicy` (heap budgets, timeline semaphore rings) `.BuildProfile` (Editor/Development/Shipping defaults).

### 6.3 `.RuntimeConfiguration` — how a run behaves (replaces `EditorConfiguration`)
| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.RuntimeConfiguration`** 🏆 (owner-sanctioned) | "Configuration" in full (abbreviation `Config` banned). Per-run profile. |
| 2 | `.RunConfiguration` | Shorter; fine; loses "runtime" precision. |
| 3 | `.SessionConfiguration` | Session = editor session; scope creep. |
| 4 | `.LaunchConfiguration` | Launch ≈ process spawn only. |
| 5 | `.EditorConfiguration` | Explicitly retired (owner). |
| 6 | `.ExecutionProfile` | Close; "profile" already means run profile — redundant. |
| 7 | `.RuntimeProfile` | Acceptable twin; owner named RuntimeConfiguration — keep it. |

**Sections:** `.WindowPolicy` (start mode, resizability) `.Diagnostics` (assert level, log channels, watchdog arming) `.Profiling` (capture scopes, telemetry sinks) `.StartupWorkspace` (which workspace opens) `.FeatureToggle`* (editor-only gates; stripped in Shipping).

### 6.4 `.PlatformSettings` — hardware/OS peripheral reality
| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.PlatformSettings`** 🏆 (owner-sanctioned) | GPU, displays, audio devices, input hardware. |
| 2 | `.HardwareSettings` | Hardware ≠ OS surface; too broad/narrow at once. |
| 3 | `.DeviceSettings` | Device-agnostic read; USB connotation. |
| 4 | `.TargetingSettings` | Build-target flavor; not peripherals. |
| 5 | `.MachineSettings` | Implies one machine; these are per-machine but the word is informal. |
| 6 | `.PlatformProfile` | Close twin; owner named PlatformSettings — keep it. |
| 7 | `.HardwareConfiguration` | Verbose twin of §6.2. |

**Sections:** `.GpuDevice` (adapter choice, heap budget, upscaler) `.DisplayTopology` (monitor layout, DPI scale) `.AudioHardware` (buffer size, device) `.InputDevices` (gamepad maps, stylus calibration) `.FileSystemRoots` (content roots, cache roots). **Developer-only?** No — ships with Editor andDevelopment profiles; Shipping reads it read-only.

### 6.5 `.WorkspaceLayout` — editor UI arrangement (replaces `StudioLayout`)
| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`.WorkspaceLayout`** 🏆 (owner-sanctioned) | Geometry of the editor itself. |
| 2 | `.StudioLayout` | `studio` banned (owner ruling). |
| 3 | `.EditorLayout` | Fine twin; but "editor" narrows to one workspace family. |
| 4 | `.PaneLayout` | Panes are half the story; docking/tabs/notch missing. |
| 5 | `.DockLayout` | Docking only; hides theme/scale prefs. |
| 6 | `.UiLayout` | Terse; okay but generic. |
| 7 | `.InterfaceLayout` | Verbose; "interface" also means public API surface. |

**Sections:** `.PanelArrangement` (dock cells, sizes, z-order) `.TabStrip` `.NotchGeometry` `.ThemeSelection` (theme id + accent) `.UiScale` `.FontSelection` (references `.FontSpace`) `.ShortcutOverrides` (references `.InputSettings`). **Retire:** `StudioLayout` everywhere (note `.VisualAppearance` + `.FontSpace` + `.InputSettings` previously inside `.ProjectSpace` fold here or into `.ProjectIndex.StartupTarget` — rule: anything *per-user/per-machine UI* → `.WorkspaceLayout`; anything *per-project* → `.ProjectIndex`).

### 6.6 Configuration family — at a glance
```text
Frontier Workspace Root
├── ProjectIndex            // What this project is (identity, deps, startup)         ← project-scoped, git-committed
├── EngineSettings          // What the engine enforces everywhere                    ← engine-scoped, git-committed
├── RuntimeConfiguration    // How one run behaves (diagnostics, window, profiling)   ← per-profile, git-committed
├── PlatformSettings        // Reality of this machine (GPU, audio, displays, input)  ← per-machine, git-ignored
└── WorkspaceLayout         // How the editor looks on this desk (theme, docks, scale)← per-user, git-ignored
```
Ownership rule: committed vs machine-local is decided by section, not by filename — but the defaults above hold. `.ProjectSpace` and `.projectspace` read-aliases survive one deprecation window mapping 1:1 onto these five.

### 6.7 `DynamicMaterialRecipe` → `DynamicMaterialProcedure` — *Recipe is dead*
Not a file format, but a banned word in the tree; same class of fix.

| # | Candidate | Note |
| :---: | :--- | :--- |
| 1 | **`DynamicMaterialProcedure`** 🏆 | Mechanism = ordered procedural steps. |
| 2 | `MaterialFormula` | Formula = closed-form math; misses sequencing. |
| 3 | `DynamicMaterialProgram` | Program implies compile/run split; we have one pass. |
| 4 | `DynamicMaterialBlueprint` | Blueprint = layout drawing; wrong instrument. |
| 5 | `MaterialStack` | Pass-order stack is a *part* of it (as is `MaterialLayerStack`). |
| 6 | `ProceduralMaterialSequence` | Sequence is our approved GPU-unit noun; but type is not a GPU unit. |
| 7 | `DynamicMaterialProtocol` | Protocol = wire behavior; reserved for networking. |

**Ruling:** rename type, file, tests (`Exhibits/DynamicMaterialRecipeValidation.cpp` → `DynamicMaterialProcedureValidation.cpp`). Sweep for `recipe` case-insensitively across comments.

---

## 7. The Embedded Section Palette (Tier D) & the Reference Matrix

### 7.1 Universal sections (every Tier A/B container)
`.DocumentHeader` · `.BinarySectionDirectory` · `.Revision` · `.Origin`

### 7.2 Domain sections by container
| Container | Embeds (inline) | References (`.ContentReference`) |
| :--- | :--- | :--- |
| `.GeometrySpace` | `.Primitives` `.Points` `.Corners` `.Faces` `.Edges` `.Normals` `.Tangents` `.Creases` `.PathCurve` `.Parameterization` | — |
| `.TextureSpace` | `.ColourSpace` `.ChannelPack` `.MipChain` `.BlockEncoding` `.TexelDensity` | `.UDIM` tile sets |
| `.PixelSpace` | `.ColourSpace` `.Exposure` `.Tone` `.HDRI` `.Tiles` `.LayerStack` | mip pyramids > 64 MiB |
| `.VectorSpace` | `.PathCurve` `.StrokeAppearance` `.Stencil` `.LayerStack` `.ColourSpace` | — |
| `.MaterialSpace` | `.BSDF` `.Microfacet` `.ThinFilm` `.Clearcoat` `.Sheen` `.Transmission` `.Dispersion` `.LayerStack` `.ClosureNetwork` `.ColourSpace` | `.TextureSpace` channels, `.PixelSpace` bake targets |
| `.VolumetricSpace` | `.SignedDistance` `.DensityGrid` `.Absorption` `.Scatter` `.Phase` `.Cloud` | LUT blobs > 16 MiB |
| `.ParticleSpace` | `.SpawnSource` `.VectorField` `.ColourRamp` `.Integration` `.Trail` `.Fibre` | — |
| `.PhysicsSpace` | `.RigidDynamics` `.Constraints` `.Collision` `.Fluid` `.ForceField` `.Motion` | — |
| `.AudioSpace` | `.Waveform` `.BusLayout` `.AcousticProfile` `.RoomImpulse` | PCM streams > 32 MiB |
| `.FontSpace` | `.TypefaceGroup` `.WeightList` `.Metrics` | sfnt binaries |
| `.WorldSpace` | `.InstancePlacement`* | `.Instance` paths, `.Atmosphere`, `.Terrain` |
| `.ComponentSpace` | `.ComponentClass` `.SlotRegister` `.PhysicsBinding` | `.PixelSpace` thumbnails |
| `.Atmosphere` | `.TransmittanceLUT` `.MultiScatterLUT` `.SolarOptics` `.PhaseFunction` `.WindCoupling` `.CloudCondensate` | baked LUT grids |
| `.Lens` | `.ElementStack` `.ElementSpacing` `.ApertureStop` `.CoatingSet` `.ChromaticProfile` `.GlareProfile` `.FocusScale` `.SensorGate` | — |
| `.FractureNetwork` | `.ConvexCells` `.BondRegister` `.WeibullDistribution` `.HertzCone` | — |
| `.Instance` | `.InstanceHeader` `.TransformPlacement` `.MaterialAssignment` `.BehaviorBinding` | `.GeometrySpace` `.PixelSpace` `.TextureSpace` `.MaterialSpace` `.PhysicsSpace` `.VectorField` payloads |
| `.Behavior` | `.RuleSet` `.InputBinding` `.OutputBinding` `.TimerRegister` | — |
| `.ProjectIndex` | `.ProjectIdentity` `.StartupTarget` `.DependencyLock`* | default `.Instance` |
| `.WorkspaceLayout` | `.PanelArrangement` `.TabStrip` `.NotchGeometry` `.ThemeSelection` `.UiScale` | `.FontSpace`, `.InputSettings` |

### 7.3 Reference matrix — who may embed what
- **`.Instance` may embed everything.** That is its job.
- **Tier A containers may reference Tier A/B documents**, embed their own sections, and embed `.ColourSpace`/`.PathCurve`/`.VectorField`-class payloads — but never embed a whole `.Instance` (no `.Instance`-in-`.Instance`).
- **Tier B/C containers** embed only own-sections plus `.Origin` and `.ContentReference`.
- **Tier D sections never stand alone.** Loaders reject `*.Revision` as a root file.

---

## 8. Worked Examples

### 8.1 `VehicleDefault.Instance` (the owner's example, completed)
```toml
# VehicleDefault.Instance — Two-Tier Hybrid, PascalCase sections
['.DocumentHeader']
SchemaVersion = 1
ContainerExtension = '.Instance'
DocumentTitle = 'VehicleDefault'
InstanceCategory = 'Vehicle'
BinaryAlignmentBytes = 64

['.InstanceHeader']
IdentityToken = '0x9E37…'            # 32 B generation-checked token
DisplayTitle = 'Frontier Control Vehicle'
ThumbnailReference = 'EngineContent/Vehicle/DefaultThumbnail.PixelSpace'

[['.ContentReference']]              # large payloads stay by-reference
ResidencePath = 'Geometry/VehicleDefault.GeometrySpace'
FNV1a64 = '0xC0FFEE11'
ByteLength = 48123456
SectionMap = ['.Faces', '.Normals', '.Tangents', '.UDIM']
ResidenceGeometry = true

[['.ContentReference']]
ResidencePath = 'Textures/ControlVehicleBody.TextureSpace'
FNV1a64 = '0xDEADBEEF'
ByteLength = 209715200

[['.ContentReference']]
ResidencePath = 'Materials/ControlVehicle.MaterialSpace'   # 128 B registers; inline-able, still by reference for reuse
FNV1a64 = '0xBADC0DE5'
ByteLength = 16384

['.TransformPlacement']              # inline
PlacementToken = '0x1234'
TargetSocket = 'VehicleBay.Default'

['.BehaviorBinding']                 # inline — the logic hooks (NOT .Script…)
BehaviorPath = 'Behavior/ControlVehicle.Behavior'
RuleSetOrdinal = 0                   # throttle/brake/steer/shift/headlamp rules

['.Origin']                          # §5.3 — replaces any notion of provenance
GeneratingTool = 'Frontier.VehicleLab/1.4.0'
AuthoredWorkspace = 'VehicleLab'
InputHashes = ['Geometry/ControlVehicleBody.GeometrySpace@0xC0FFEE11']
AuthoredUtc = '2026-11-20T09:00:00Z'
```
Runtime split of the same document at load: `GeometryInstance` tokens → render stream; `.MaterialSpace` → shading registers; `.BehaviorBinding` → `BehaviorExtension` ticks; suspension powertrain (Jolt `.PhysicsSpace` reference) → physics thread. The `.Instance` never duplicates bulk bytes — it owns *identity, assignment and placement*.

**What the vehicle literally contains** (by reference unless noted): `.GeometrySpace` (body/rims/tyres) · `.PixelSpace` (thumbnail/paint bakes) · `.TextureSpace` (channel-packed PBR set) · `.MaterialSpace` (metallic paint, obsidian canopy, tyre rubber, rim alloy) · `.VectorField` (downforce/drag/lift coupling for the aero pass; embedded in `.PhysicsSpace` once resolved) · `.VolumetricSpace` (dust/spray volumes it generates at speed) · `.Behavior` (throttle curve, 6-speed shift logic, headlamp toggle).

### 8.2 `ParticleSpace` (owner's second example)
```toml
['.DocumentHeader']
ContainerExtension = '.ParticleSpace'
SchemaVersion = 1

['.SpawnSource']
RateHz = 240.0
Basis = 'SphereSurface'              # SphereSurface | Point | Cone | AreaGrid
RadiusMetres = 0.35

['.VectorField']                     # the field it advects by (§4.16)
AnalyticModel = 'CurlNoise'          # LinearWind | CurlNoise | RadialBurst | Lorentz …
Amplitude = 1.8
CorrelationLength = 0.6
Seed = 88172645463325252

['.ColourRamp']                      # the "colours" (owner's words)
GradientStops = [ [0.0,'#FFFFFFFF'], [0.35,'#FFB36BFF'], [1.0,'#00000000'] ]
ColourSpace = '.ColourSpace/Accepted.LinearRec2020'

['.Integration']
Solver = 'RK4'
Substeps = 2
LifetimeSeconds = 1.5

['.Trail']
EnableTrail = true
LengthMetres = 0.8
```

### 8.3 `PhysicsSpace` with `.Fluid` + `.ForceField` (the "forces and fluid it generates" case)
```toml
['.DocumentHeader']
ContainerExtension = '.PhysicsSpace'
SchemaVersion = 1

['.RigidDynamics']
MassKilograms = 1_480.0
InertiaTensor = 'AutoFromConvexCells'

['.Fluid']                           # regime configuration, not a standalone container
Solver = 'LatticeBoltzmann'          # Grid-based; baked state exports to .VolumetricSpace
Viscosity = 1.0e-6
PhaseCoupling = ['Body', 'Spray']

['.ForceField']                      # application of a .VectorField (§4.16)
Model = 'LinearWind'                 # Ignores banned Law… Exposed as ForceModel ordinals
VectorFieldReference = 'VehicleAerodynamics.VectorField'
CouplingRegister = 'DownforceCoefficient'
```

### 8.4 `Atmosphere` (ex-SkyAtmosphereCodec)
```toml
['.DocumentHeader']
ContainerExtension = '.Atmosphere'
SchemaVersion = 1

['.TransmittanceLUT']
Resolution = [256, 64]
SolarElevationRange = [-0.05, 1.0]

['.SolarOptics']
SunAngularRadiusMicroRadians = 4652  # Neckel & Labs limb darkening
KelvinTemperature = 5780

['.WindCoupling']                    # feeds PlanetaryClouds + VolumetricFog advection
BaseVelocity = [2.5, 0.0, 0.5]
GustAmplitude = 1.4

['.Origin']
GeneratingTool = 'Frontier.AtmosphereBaker/2.1.0'
```

### 8.5 `.Lens` (entire lens, not just aperture)
```toml
['.DocumentHeader']
ContainerExtension = '.Lens'
SchemaVersion = 1

['.ElementStack']
Elements = [ { Index = 1, RadiusMm = 62.4, ThicknessMm = 12.0, Ior = 1.5168, Coating = 'MgF2' }, … ]

['.ElementSpacing']
AirGapsMm = [ 4.2, 28.7 ]            # whole-train air gaps — the entire lens is here

['.ApertureStop']                    # one element among equals — not the document
BladeCount = 9
IrisCurve = 'Circular'
MaxApertureFNumber = 1.4
IridescenceProfile = 'Butterfly'

['.ChromaticProfile']
AbbeNumber = 59.5                    # 5.95×10²; longitudinal color shift per stop
ChromaticShiftMillimetresAtF8 = 0.18

['.GlareProfile']
StreakCount = 6
StreakRotationDegrees = 30.0
GhostIntensity = 0.08

['.SensorGate']
Mount = 'E-Mount'
FlangeFocusDistanceMillimetres = 18.0
ImageCircleMillimetres = 43.3
```

---

## 9. Banned-Word Ledger Amendments (to append to `BannedWordsVocabulary.md` §4, dated 2026-11-23)

- **Barred `Dag`** everywhere (filenames, identifiers, comments, plan titles). Use **Revision Network** (`PolygonNetwork`-class) or **Serialization Network** for world-serialization ordering.
- **Barred `Provenance`** everywhere. Use **`.Origin`** (embedded section), `Origin.json` (external sidecars), or `OriginRecord` (C++ type).
- **Barred `Recipe`** everywhere. Use **`Procedure`** (`DynamicMaterialProcedure`), **`Formula`** (closed-form), or **`Sequence`** (GPU render units only).
- **Barred `Studio`** everywhere (filenames, folders, identifiers, prose). Use **`Workspace`** for authoring environments, **`Viewer`** for static presentation, or the concrete tool name (`TextureWorkspace`, `MarkdownViewer`).
- **Barred `Peer`** everywhere (identifiers, comments, prose). Use **`Endpoint`** (`NetworkEndpointRegister`), **`Remote`** (`RemoteDirectiveRouter`), or **`Socket`**.
- **Barred `Tree`** in graph/ownership/fracture/feature contexts (identifiers, filenames, comments). Use **`Network`** (`PolygonNetwork`, `FractureNetwork`, `FeatureStack`), **`Stack`** (ordered compile stacks), or **`Timeline`** (ordered playback). *(Scope: design-time structural names; botany and genuine binary-search trees in algorithms may note an exemption request.)*
- **Barred `Script`** in container/format names for logic. Use **`.Behavior`** / `BehaviorCodec` / `.BehaviorBinding`.
- **Rejected `VectorGraphics`** as a container name (superseded by `.VectorSpace`). Recorded in §2.2 of the ledger as "rejected name", same class as `.UVChart`.
- **Rejected `PixelSheet`** as a container name (mechanism-free). Recorded as rejected name.
- **Re-affirmed `Facet`/`Archive`/`Law`** with concrete rename paths (§0.2 of this plan).
- **Discouraged (avoid) `Sheet`** as a format/container noun (`ThumbnailSheet` excepted — it is a pre-baked atlas, and "ThumbnailSheet" is indexed as approved vocabulary in `FolderStructure.md`; revisit on next governance pass).

---

## 10. Rename Ledger & Verification Gates

### 10.1 Mechanical renames (safe, sed-able once owner approves)
| From | To |
| :--- | :--- |
| `Engine/SimulationSpace/` | `Engine/PhysicsSpace/` |
| `Engine/Codecs/SimulationCodec.{h,cpp}` | `Engine/Codecs/PhysicsSpaceCodec.{h,cpp}` |
| `Engine/Codecs/AcousticCodec.{h,cpp}` | `Engine/Codecs/AudioSpaceCodec.{h,cpp}` |
| `Engine/Codecs/ImageCodec.{h,cpp}` | `Engine/Codecs/PixelSpaceCodec.{h,cpp}` |
| `Engine/Codecs/PhotometrySpaceCodec.{h,cpp}` | `Engine/Codecs/PhotometryCodec.{h,cpp}` |
| `Engine/Codecs/ProjectCodec.{h,cpp}` | `Engine/Codecs/ProjectIndexCodec.{h,cpp}` |
| `Engine/MaterialSpace/Dynamic/DynamicMaterialRecipe.{h,cpp}` | `Engine/MaterialSpace/Dynamic/DynamicMaterialProcedure.{h,cpp}` |
| `Exhibits/DynamicMaterialRecipeValidation.cpp` | `Exhibits/DynamicMaterialProcedureValidation.cpp` |
| `Engine/Components/PixelSpace.{h,cpp}` | `Engine/Components/PixelCanvas.{h,cpp}` |
| `Engine/Components/Panes/StudioPaneAppearance.h` | `Engine/Components/Panes/WorkspacePaneAppearance.h` |
| `Engine/Shaders/MaterialSpace/StudioEnvironmentRadiance.slang` | `Engine/Shaders/MaterialSpace/ImageBasedRadiance.slang` |
| `EngineContent/EnvironmentArchives/` | `EngineContent/AtmosphereRepository/` |
| `Content/Environment/` | `Content/Atmosphere/` |
| `Content/Optics/StudioShadingSphereSheet.ImageSpace` | `Content/Optics/ShadingSphereSheet.PixelSpace` |
| `EngineContent/MatCaps/MatCapStudioAtlas.png` / `.rgba` / `MatCapStudioCatalog.json` | `MatCapAtlas.png` / `.rgba` / `MatCapCatalog.json` |
| `EngineContent/Input/FrontierStudio.InputSpace` | `EngineContent/Input/FrontierDefault.InputSpace` |
| `Experimental/FrontierWorldMirror/InspectorDepot/Provenance.json` · `ProjectZeroEditor/InspectorDepot/Provenance.json` | `Origin.json` (schema fields renamed `GeneratingTool/InputHashes/AuthoredUtc`) |
| `Documentation/Plans/WorldSerializationDagPlan.md` (if present) | `Documentation/Plans/WorldSerializationNetworkPlan.md` |
| `Documentation/Plans/VectorFieldArchitecturePlan.md` "Force Laws" | "Force Models" (3 occurrences) |
| `Engine/Workspaces/Revision/README.md` "Revision DAG" | "Revision Network" |
| `Engine/Workspaces/ComponentEditor/ComponentTreePanel.{h,cpp}` | *(exempt — widget tree panel; or rename `ComponentOutlinerPanel` to match `OutlinerPanel` convention — owner call)* |
| `Engine/GeometrySpace/Parametric/Feature/FeatureTree.{h,cpp}` | `FeatureStack.{h,cpp}` *(pairs with `Stack` vocabulary)* |
| `Experimental/VehicleProvingGrounds/Archive/FacetArchiveCodec.py` + `Source/FacetCodec.js` + `*.facet` | `Repository/FaceStreamCodec.py` + `Source/FaceStreamCodec.js` + `*.face` |
| `Automation/ServeMarkdownStudio.py` | `Automation/ServeMarkdownViewer.py` |
| `.PhotometrySpace` content files ×7 | `.Photometry` |
| `.SimulationSpace` content/codecs | `.PhysicsSpace` |
| `.Acoustic` content/codecs | `.AudioSpace` |
| `.ImageSpace` content/codecs | `.PixelSpace` |
| `.SurfaceParameterSpace` content ×7 | `.Parameterization` (or embedded `.Parameterization` inside `.GeometrySpace` — owner call) |
| `"PixelSpace.h"` includes | `"PixelCanvas.h"` |

### 10.2 Experimental sweep (`Studio*` names, informational)
`Experimental/` holds ~60 `Studio*` paths (`Shared/Studio/`, `TextureStudio/`, `PhotometricEditor/` refs, `StudioControls.js`, `Studio.css`…). These are reference prototypes; the HTML/JSX should migrate to `Workspace`/`Viewer` vocabulary on the next UI pass, and `TextureStudio/` → `TextureWorkspace/` when folded into native C++. **No file is renamed until the owner approves the list**, because cross-references (`index.html`, `CheckEnvironmentGraphs.mjs`) must move atomically.

### 10.3 Governance documents to update (in the same change)
1. `Documentation/FolderStructure.md` — Tier registry table, Engine/Codecs list, Configuration file list, `Content/Atmosphere`, `AtmosphereRepository`.
2. `Documentation/BannedWordsVocabulary.md` — §4 dated amendment (exact text in §9 above) + §5 A–Z index additions (`Dag`, `Provenance`, `Recipe`, `Studio`, `Peer`, `Tree`, `Script`).
3. `Documentation/Plans/DocumentFormatsInstancePlan.md` — replaced by this document (or fold §3–§8 of this plan into it).
4. `Documentation/Plans/DocumentSpacesRevisionPlan.md` — codec rename table + `.PhysicsSpace` promotion.
5. `Content/ContentRepositoryIndex.ProjectSpace` — dependency locks re-pointed to new extensions (content hashes recomputed).
6. `Engine/Codecs/README.md` — file table regenerated.
7. `CLAUDE.md` / `AGENTS.md` — banned-word pointer updated (no body changes needed; they reference the ledger).
8. New `.Instance` + `.Behavior` + `.Atmosphere` + `.Lens` + `.FractureNetwork` + `.PixelSpace` codec sources + `Exhibits/` validation targets when implementation begins.

### 10.4 Verification gates (definition of done for the rename PR)
- `grep -riE 'recipe|provenance|\bdag\b|studio|\bpeer\b' --exclude-dir=.git` → only governance-ledger hits describing the ban itself.
- `grep -riE '\b(facet|archive|\.assembly)\b'` → zero hits outside approved exemptions (`FontsArchives`, this plan).
- Loader round-trip tests: `.PhysicsSpace`, `.AudioSpace`, `.PixelSpace`, `.Atmosphere`, `.Lens`, `.Instance`, `.Behavior` open case-insensitively and write PascalCase; legacy aliases (`.projectspace`, `.imagespace`, `.simulationspace`, `.acoustic`) read with deprecation warnings for one release.
- `Exhibits/` builds; `Exhibits/Images/` snapshots re-rendered where file names embed them.
- Two-Tier framing self-check: every fixture container has `.DocumentHeader` + `.BinarySectionDirectory` and 64 B section alignment.
- `git grep -E '\.Space\b'` reviewed by owner: every surviving `Space` file is Tier A or explicitly grandfathered.

---

## 11. Open Questions for the Owner (answer inline or by reply)

1. **`.PixelSpace` vs keep `.ImageSpace`:** I recommend the rename (§4.2) with the `PixelCanvas` UI rename. Keep or rename?
2. **`.PhysicsSpace` promotion** (§4.10): approve promotion + `.SimulationSpace` retirement, or keep `.SimulationSpace` and leave physics as an inner folder?
3. **`.AudioSpace`** (§4.11): approve `.Acoustic` retirement?
4. **`.ProjectSpace` retirement** (§6.1): split fully into the five configuration files — or keep `.ProjectSpace` as the umbrella for `.ProjectIndex` + sections and only split the UI/machine-local parts?
5. **`.PhotometrySpace` → `.Photometry`** (§5.4): approve, or grandfather `.PhotometrySpace`?
6. **`.SurfaceParameterSpace`** (7 files): fold into embedded `.Parameterization` sections inside `.GeometrySpace`, or keep standalone as `.Parameterization`?
7. **Standalone `.VectorField` container** (§4.16): needed as a reusable baked-field asset, or embedded-only (inside `.ParticleSpace`/`.PhysicsSpace`/`.Instance`)?
8. **`.Behavior` vs `.Logic`** for the logic container (§5.1)? (Recommendation: `.Behavior`.)
9. **`FeatureTree` → `FeatureStack`** and `ComponentTreePanel` rename — approve both, or exempt the UI widget case?
10. **Apply the renames** to branch `arena/2809d087-frontier` in `egoistinbox-hue/Frontier` (as a PR), or keep this document as the agreed naming law and let you/Claude implement? I have **not** touched that repository.
11. **`ThumbnailSheet`** discouraged-word status — keep as-is, or rename now (e.g. `ThumbnailGrid`)?
12. **Space retention veto window** (§3 borderline): confirm `.WorldSpace`, `.ComponentSpace`, `.InputSpace`, `.NetworkSpace`, `.RenderSpace` keep the suffix (default: yes).

---

## 12. Executive Summary (one screen)

- **`Space` is earned** by self-contained runtime categories: Geometry, UV, Texture, **Pixel** (was Image), Vector, Colour, Volumetric, Particle, **Physics** (was Simulation), **Audio** (was Acoustic), Material, Font (+ retained World/Component/Input/Network/Render pending your veto).
- **New Tier B containers:** `.Atmosphere` (ex-SkyAtmosphereCodec), `.Lens` (whole lens — ex-LensApertureCodec), `.FractureNetwork` (ex-FractureTreeCodec), `.Instance` (VehicleDefault.Instance), `.Behavior` (logic hooks — never `.Script`), `.Photometry`, `.Parameterization`.
- **Configuration family (your five):** `.ProjectIndex` · `.EngineSettings` · `.RuntimeConfiguration` · `.PlatformSettings` · `.WorkspaceLayout` (ex-StudioLayout) — `.ProjectSpace` retired.
- **Embedded palette:** `.Revision` `.Origin` (ex-Provenance) `.VectorField` (storage) + `.ForceField` (application) `.Fluid` `.PathCurve` `.ContentReference` `.InstanceHeader` `.BehaviorBinding` — DAG is now **Revision Network**.
- **Banned-word sweep** shipped as a ledger amendment (§9) with concrete rename paths (§10).
- **Everything above is a proposal.** No repository has been modified; `egoistinbox-hue/Frontier` untouched pending your answers to §11.
