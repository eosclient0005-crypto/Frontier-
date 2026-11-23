# Frontier Engine Format Set: Naming, Folder Structure, and Embedding Plan

> **Status:** Proposal for review. No code has been renamed yet.
> **Scope:** Every native file format the engine reads or writes: textures, raster pixels, vector paths, fonts, geometry, UV layouts, materials, colour, lighting, lenses, atmosphere, volumes, force fields, fluids, particles, fracture, animation, logic, input, audio, projects, history, and networking.
> **Language:** British spelling throughout (`Colour`, `Behaviour`, `Fibre`, `Licence`), matching the existing `ColourSpace`.

---

## 0. Feedback Mapped to Decisions

| Feedback | Previous name (from earlier plans) | New name |
| :--- | :--- | :--- |
| No `Dag` | `WorldSerializationDagPlan`, `DependencyDAG` | `WorldSerializationOrderPlan`, `DependencyNetwork` |
| No `Tree` | `FractureTreeCodec`, `.FractureTree` | `FractureNetworkCodec`, `.FractureNetwork` |
| No `Provenance` | `Provenance`, `ProvenanceInspector`, `Provenance.json` | `OriginLedger`, `OriginInspector` |
| No `Recipe` | `DynamicMaterialRecipe`, `WaveformSynthesisRecipe` | `DynamicMaterialFormula`, `WaveformSynthesisFormula` |
| No `Script` | `ScriptingSpace` | `BehaviourSpace` |
| `PixelSpace`, not `PixelSheet` | `PixelSheetCodec`, `.PixelSheet` | `PixelSpace` (raster). Packed sprite pages become `SpriteSheet` |
| Atmosphere | `SkyAtmosphereCodec`, `.SkyAtmosphere` | `AtmosphereSpace` / `AtmosphereCodec` |
| Lens covers the whole lens | `LensApertureCodec`, `.LensAperture` | `LensSpace` (whole lens). Aperture becomes one section inside it |
| Dislikes `VectorField` | `.VectorFieldSpace` | Split into `ForceSpace` (forces) and `VelocitySpace` (fluid motion) |
| Dislikes `VectorGraphics` | `VectorGraphics`, `VectorGraphicsWorkspacePlan` | `PathSpace`, `PathWorkspacePlan` |
| Dislikes `CurveSpace.Environment` | nested "Environment" under a Curve | `CurveSpace` (1-D curves only). Environment becomes `ClimateSpace` |
| Dislikes `EditorConfiguration` | `EditorConfiguration` | `EditorPreferences` |
| Dislikes `Projectspace` | `.ProjectSpace` | `ProjectDescriptor` |

**Source note:** `CurveSpace.Environment`, `EditorConfiguration`, `ScriptingSpace`, and the `VehicleDefault.Instance` example are not in the current branch. They come from the earlier session and are treated as stated. Every other old name above was found in the linked branch `egoistinbox-hue/Frontier @ arena/2809d087-frontier`.

---

## 1. Naming Rules

| Rule | Statement |
| :--- | :--- |
| **R1 Banned vocabulary** | Apply `Documentation/BannedWordsVocabulary.md` in full, plus these user additions: `Dag`, `Tree`, `Provenance`, `Recipe`, `Script`, `Scripting`, `Hook`, `Environment`, `VectorField`, `Config`. |
| **R2 Name the thing, not the container** | `Force` is the physical quantity. `VectorField` is a storage shape. A file is named for what it holds, not how it is stored. |
| **R3 `Space` marks a top-level domain only** | Sub-parts use plain nouns. Never nest a second `Space` (`CurveSpace.Environment` is forbidden). |
| **R4 Linked structures are `Network`** | Any structure linked by index (parent/child-style links, dependency links, closure links) is a `Network`. Never `Tree`, `Graph`, `DAG`, `Hierarchy`, or `Node`. |
| **R5 One name per concept** | No parallel names for the same thing (`ImageSpace` and `PixelSpace` must not both mean "raster"). |
| **R6 Codec naming** | Codec class = container name without `Space`, plus `Codec`: `AtmosphereCodec`, `PixelCodec`, `LensCodec`, `FractureNetworkCodec`. **Decision D1 pending.** |
| **R7 No conjunctions** | Never `And` in a name. Split into two units. |
| **R8 Optics vocabulary** | Lens parts are `GlassGroup` and `OpticalSurface`, never `Element` (banned). |
| **R9 Overlays** | Viewport overlays of any Space are `…Rasterization`, per `SKILL-Naming.md`. |
| **R10 Categories** | Enumerations use `SpaceCategory`, never `Kind` (banned). |
| **R11 No SDF sections** | The branch purged general-purpose SDF. No format may add a signed-distance section. |
| **R12 No `Field` suffix** | Drop `Field` from names. Describe the quantity (`Force`, `Velocity`, `Density`). |

---

## 2. Architecture: Three File Tiers and One Runtime Role

| Tier | Suffix | What it is | Example |
| :--- | :--- | :--- | :--- |
| **A. Space file** | `.<Space>`, one per area | Single-area document, one codec | `Brushed.MaterialSpace`, `Rain.ForceSpace`, `Sky.AtmosphereSpace` |
| **B. Instance** | `.Instance` (**D2 pending**) | Multi-section container. Embeds several Spaces in one file | `VehicleDefault.Instance` |
| **C. Interchange** | none (translated on import) | External formats converted into A or B | OBJ, glTF, SVG, EXR, PNG, KTX2, IES, MaterialX |
| *Runtime* | none (memory only) | Live transfer between workspaces or OS boundaries. Existing term: `Exchange` | Clipboard, drag-and-drop, live UV-to-texture sync |

### 2.1 Instance Layout (Tier B)

```text
+--------------------------------------------------------------------------+
| HEADER            TOML, UTF-8. Identity, schema version, units, colour    |
|                   primaries. Human-readable. Written first.              |
+--------------------------------------------------------------------------+
| SECTION DIRECTORY One entry per section:                                 |
|                   SpaceCategory, CodecVersion, Name, Placement           |
|                   (Embedded | Linked), Offset, ByteLength, Hash64,        |
|                   DependencyIndices (edges of a DependencyNetwork)        |
+--------------------------------------------------------------------------+
| 64-BYTE BOUNDARY  Explicit padding                                       |
+--------------------------------------------------------------------------+
| SECTION PAYLOADS  Each section is a full Space payload with its own      |
|                   codec, so it can be read on its own.                   |
+--------------------------------------------------------------------------+
```

**Rules for embedding**

- **Embedded:** the bytes live inside the Instance.
- **Linked:** the Instance stores a relative path plus `Hash64`. Use this for shared libraries such as a headlamp IES file used by many vehicles.
- **Flat, no nesting:** an Instance never contains another Instance. It links to one through a Linked section. This avoids nesting.
- **Dependencies:** a section that uses another section lists it in `DependencyIndices`. Example: `MaterialSpace` depends on `TextureSpace`, which depends on `PixelSpace`.
- **Versioning:** each section carries its own `CodecVersion`. The Instance header carries only the container version.

### 2.2 Section Vocabulary

| Term | Meaning | Alternatives (if D-series rejects) |
| :--- | :--- | :--- |
| **Section** | Embedded payload inside a container | Part · Block · Chunk · Segment · Slot |
| **Linked** | Reference to an external file | Reference · Import · Pointer · Tether · Attachment |
| **SectionDirectory** | Contents list | Index · Roster · Register · Inventory · Listing |
| **DependencyNetwork** | Edges between sections | Linkage · Order · Dependency Chain · Build Order · Requirements |
| **Header** | TOML identity block | TomlHeader · DocumentHeader · HeaderSection · IndexHeader |

---

## 3. Category Index (Format Names)

Each row has one **recommended** name and five **alternatives**. Key sections are the parts a format contains. Sections are the main reuse unit, so they are also named here.

### 3.1 Containers, Project, and Workspace

| # | Category | Recommended | Alternatives (5) | Holds (key sections) |
| :--- | :--- | :--- | :--- | :--- |
| 1 | Multi-section container | **Instance** | Bundle · Document · Prefab · Package · Blueprint | Header, SectionDirectory, sections |
| 2 | Project root | **ProjectDescriptor** | ProjectSettings · ProjectRegister · ProjectRoot · ProjectIndex · WorkspaceDescriptor | Name, units, default colour, layout reference |
| 3 | Editor settings | **EditorPreferences** | EditorSettings · WorkbenchLayout · InterfaceSettings · EditorProfile · SessionPreferences | Themes, UI scale, panel splits, shortcuts |
| 4 | Edit history (undo) | **RevisionSpace** | HistorySpace · JournalSpace · OperationLog · ChangeLog · EditLedger | Operation, Branch, Compaction |
| 5 | Origin (where content came from) | **OriginLedger** | OriginLog · SourceTrail · DerivationLog · SourceRegister · OriginSpace | Source, Tool, Licence, Timestamp |
| 6 | Procedural parameter set | **Formula** | Schematic · Procedure · ParameterSet · Derivation · Specification | Inputs, steps, seed (replaces the old `Recipe` name) |
| 7 | World / placement | **WorldSpace** | SceneSpace · LevelSpace · PlacementSpace · ZoneSpace · RegionSpace | Placement, Region, references to ClimateSpace |
| 8 | Render and view settings | **RenderSpace** | ViewSpace · ViewportSpace · CaptureSpace · CameraSpace · ExposureSpace | Viewport, Capture, exposure defaults |
| 9 | Input bindings | **InputSpace** | ControlSpace · ShortcutSpace · ChordSpace · GestureSpace · DeviceSpace | Chord, Device, Axis |
| 10 | External translators | **Interchange** | Translation · Importers · Converters · Transcoders · Adapters | Per-format translators (OBJ, glTF, SVG, EXR, …) |
| 11 | Codec class suffix | **Codec** | Serializer · Transcoder · Translator · Writer · Packer | `Decode` / `Encode` pair per Space |
| 12 | Replication (multiplayer) | **ReplicationSpace** | NetworkingSpace · SyncSpace · TransportSpace · LinkSpace · BroadcastSpace | Packet, Transport, Authority |

### 3.2 Shape, Surface, and Raster

| # | Category | Recommended | Alternatives (5) | Holds (key sections) |
| :--- | :--- | :--- | :--- | :--- |
| 13 | Solid geometry | **GeometrySpace** | ShapeSpace · SolidSpace · PolygonSpace · FormSpace · BoundarySpace | Primitives, Points, Corners, Faces, Edges, Normals, Tangents, UV0–UV7, Creases, Revision |
| 14 | UV layout | **UVSpace** | ChartSpace · UnwrapSpace · ParameterSpace · SeamSpace · TileSpace | Seams, UVTile, UDIM, TexelDensity |
| 15 | Raster pixels (photo, paint, picture) | **PixelSpace** | RasterSpace · BitmapSpace · ImageSpace · PixelGridSpace · PictureSpace | Channel, Tile, Exposure, Tone, HDR, Layer, Revision |
| 16 | Packed sprite pages | **SpriteSheet** | SpritePack · TileSheet · PagePack · PackedSprites · CellSheet | Frames, Cell, Pivot |
| 17 | GPU channel set | **TextureSpace** | ChannelSpace · TextureSetSpace · ChannelPackSpace · SurfaceChannelSpace · MipChainSpace | ChannelPack, MipChain, BlockEncoding, UDIM, UVTile |
| 18 | Vector paths (SVG-like) | **PathSpace** | VectorSpace · ContourSpace · OutlineSpace · SplineSpace · StrokeSpace | Contour, Stroke, Fill, Clip, Winding |
| 19 | 1-D curves (falloff, easing, response) | **CurveSpace** | ResponseSpace · EasingSpace · FalloffSpace · TransferSpace · ShapingSpace | Curve, Knot, Tangent |
| 20 | Colour and gradients | **ColourSpace** | SpectrumSpace · ChromaSpace · GamutSpace · TintSpace · GradientSpace | Primaries, WhitePoint, TransferCurve, Gamut, Spectral, Ramp |
| 21 | Fonts and text | **FontSpace** | TypeSpace · TypefaceSpace · LetterformSpace · TextSpace · CharacterSpace | Typeface, Weight, Metrics, Hinting |
| 22 | Procedural noise and patterns | **NoiseSpace** | PatternSpace · ProceduralSpace · WeaveSpace · GeneratorSpace · StochasticSpace | Octave, Seed, Extent, Gain |

### 3.3 Appearance, Paint, Optics, and Light

| # | Category | Recommended | Alternatives (5) | Holds (key sections) |
| :--- | :--- | :--- | :--- | :--- |
| 23 | Surface appearance | **MaterialSpace** | FinishSpace · ShadingSpace · AppearanceSpace · ReflectanceSpace · LookSpace | BSDF, Microfacet, Fresnel, Clearcoat, Sheen, Transmission, Dispersion, Absorption, Subsurface, ClosureNetwork, LayerStack, Formula |
| 24 | Natural-media paint | **PigmentSpace** | BrushSpace · DabSpace · BristleSpace · StylusSpace · PaintSpace | Brush, Dab, Bristle, Stylus, Stroke |
| 25 | Lamps (IES photometry) | **LuminaireSpace** | PhotometrySpace · LightSpace · IlluminationSpace · RadianceSpace · LampSpace | Photometric (IES), Area, Spectrum, Solar |
| 26 | Whole lens system | **LensSpace** | OpticsSpace · LensSystemSpace · OpticalStackSpace · LensDesignSpace · CameraOpticsSpace | Aperture, GlassGroup, Coating, Distortion, Vignette, Exposure, Flare |

### 3.4 Climate, Volumes, and Forces

| # | Category | Recommended | Alternatives (5) | Holds (key sections) |
| :--- | :--- | :--- | :--- | :--- |
| 27 | Sky, sun, and scattering | **AtmosphereSpace** | SkySpace · SkyDomeSpace · AirSpace · ScatteringSpace · CelestialSpace | Scattering (Rayleigh, Mie, Ozone), Transmittance, AerialPerspective, Celestial |
| 28 | Umbrella: sky + weather + fog | **ClimateSpace** | WeatherSpace · AmbienceSpace · BackdropSpace · SettingSpace · SurroundSpace | References to AtmosphereSpace, VoxelSpace, ForceSpace |
| 29 | Density volumes (smoke, fog, cloud) | **VoxelSpace** | DensitySpace · MediumSpace · VolumeSpace · CloudSpace · VolumetricSpace | Density, Absorption, Scatter, Phase, Cloud |
| 30 | Force fields (gravity, wind, push) | **ForceSpace** | ForceFieldSpace · DriftSpace · ForcingSpace · ActuatorSpace · GustSpace | Grid, Falloff, Turbulence, Timing |
| 31 | Velocity (fluid motion) | **VelocitySpace** | CurrentSpace · StreamSpace · AdvectionSpace · VelocityFieldSpace · DirectionSpace | Grid, Boundary, Advection |

### 3.5 Motion, Dynamics, and Behaviour

| # | Category | Recommended | Alternatives (5) | Holds (key sections) |
| :--- | :--- | :--- | :--- | :--- |
| 32 | Fluid simulation | **FluidSpace** | LiquidSpace · SphSpace · MultiphaseSpace · FluidVolumeSpace · SolverSpace | Solver, Viscosity, Surface, Source, Seed |
| 33 | Particles | **ParticleSpace** | GrainSpace · SpraySpace · SwarmSpace · PlumeSpace · DebrisSpace | Source, Lifetime, Integration, Appearance, Forces, Trail, Fibre |
| 34 | Rigid bodies and constraints | **DynamicsSpace** | SimulationSpace · PhysicsSpace · KinematicsSpace · ConstraintSpace · BodySpace | Body, Constraint, Collision, Solver |
| 35 | Fracture | **FractureNetwork** | ShatterNetwork · FragmentNetwork · ContainmentNetwork · FractureLinkSpace · InterfaceNetwork | Fragment, Interface, Threshold, Containment, Seed |
| 36 | Animation | **AnimationSpace** | MotionSpace · KeyframeSpace · ClipSpace · PoseSpace · TimelineSpace | Keyframe, Pose, Rig, Blend, Clip |
| 37 | Logic (replaces the old `Script` name) | **BehaviourSpace** | LogicSpace · RuleSpace · ProgramSpace · ReactionSpace · TriggerSpace | Trigger, Condition, Reaction, Parameter, Timer |

### 3.6 Sound

| # | Category | Recommended | Alternatives (5) | Holds (key sections) |
| :--- | :--- | :--- | :--- | :--- |
| 38 | Sound clips and synthesis | **SoundSpace** | AudioSpace · WaveformSpace · SampleSpace · SynthSpace · VoiceSpace | Sample, Envelope, Loop, Synthesis |
| 39 | Room acoustics | **AcousticSpace** | ReverbSpace · RoomSpace · EchoSpace · PropagationSpace · ResonanceSpace | Room, Absorption, Reverb, Propagation |

### 3.7 Folder Groups (Engine and Content Grouping)

| Group | Recommended | Alternatives (5) |
| :--- | :--- | :--- |
| Shape | **Shape** | Solids · Forms · Boundary · Parametric · Surfaces |
| Raster | **Raster** | Pixels · Bitmap · Picture · Grid · Sprite |
| Vector | **Vector** | Path · Outline · Contour · Spline · Linework |
| Appearance | **Appearance** | Finish · Shading · Look · Reflectance · Pigment |
| Optics | **Optics** | Lens · Light · Lamp · Radiance · Photometry |
| Climate | **Climate** | Weather · Sky · Ambience · Backdrop · Setting |
| Motion | **Motion** | Dynamics · Kinetics · Simulation · Movement · Animation |
| Behaviour | **Behaviour** | Logic · Rule · Reaction · Trigger · Control |
| Sound | **Sound** | Audio · Acoustic · Waveform · Voice · Resonance |
| Project | **Project** | Workspace · Setting · Preference · Session · Document |

---

## 4. Embedded Examples

### 4.1 `VehicleDefault.Instance`

```text
VehicleDefault.Instance
├── Header               TOML identity, units, colour primaries
├── SectionDirectory     14 entries, each with SpaceCategory and Hash64
├── GeometrySpace        Embedded    chassis, 4 rims, 4 tyres (UV0, UVTile)
├── UVSpace              Embedded    body-panel seams
├── PixelSpace           Embedded    livery decal (RGBA16 raster)
├── TextureSpace         Embedded    packed albedo, roughness, normal   ← PixelSpace
├── MaterialSpace        Embedded    paint, glass, rubber, chrome           ← TextureSpace
├── LuminaireSpace       Linked      headlamp IES (shared library file)
├── ForceSpace           Embedded    downforce over body
├── VelocitySpace        Embedded    wake airflow for the smoke trail
├── VoxelSpace           Embedded    exhaust smoke density
├── FractureNetwork      Embedded    crash breakup network
├── AnimationSpace       Embedded    suspension rig
├── SoundSpace           Embedded    engine loop
├── BehaviourSpace       Embedded    brake lights follow brake input; door opens on press
└── RevisionSpace        Embedded    edit operations
```

### 4.2 `SparkShower.ParticleSpace` (your "ParticleSpace contains vector field and colours")

```text
SparkShower.ParticleSpace
├── ParticleSpace        Source, Lifetime, Integration
├── ForceSpace           Embedded    gravity + wind                ← your "vector field"
├── ColourSpace          Embedded    Ramp section: white → orange → transparent
├── SpriteSheet          Embedded    4×4 spark sprites
└── BehaviourSpace       Embedded    optional trigger on impact
```

### 4.3 `Sky.AtmosphereSpace` and `Lens.LensSpace`

```text
Sky.AtmosphereSpace
├── Scattering           Rayleigh, Mie, Ozone curves
├── Transmittance        LUT
├── AerialPerspective    fog LUT
└── Celestial            sun, moon, stars

Lens.LensSpace           (LensSpace is the whole lens; Aperture is one section)
├── Aperture             blade count, bokeh shape
├── GlassGroup           per optical surface: radius, index, Abbe number
├── Coating              reflectance per wavelength
├── Distortion           radial coefficients
├── Vignette             falloff
├── Exposure             f-stop range
└── Flare                ghost reflections
```

---

## 5. Repository Folder Structure

```text
Frontier/
├── Engine/
│   ├── Spaces/                      one folder per Space: codec, types, README
│   │   ├── Shape/
│   │   │   ├── GeometrySpace/
│   │   │   └── UVSpace/
│   │   ├── Raster/
│   │   │   ├── PixelSpace/
│   │   │   ├── TextureSpace/
│   │   │   ├── SpriteSheet/
│   │   │   └── NoiseSpace/
│   │   ├── Vector/
│   │   │   ├── PathSpace/
│   │   │   ├── CurveSpace/
│   │   │   └── FontSpace/
│   │   ├── Appearance/
│   │   │   ├── ColourSpace/
│   │   │   ├── MaterialSpace/
│   │   │   └── PigmentSpace/
│   │   ├── Optics/
│   │   │   ├── LensSpace/
│   │   │   └── LuminaireSpace/
│   │   ├── Climate/
│   │   │   ├── ClimateSpace/
│   │   │   ├── AtmosphereSpace/
│   │   │   ├── VoxelSpace/
│   │   │   └── ForceSpace/ + VelocitySpace/
│   │   ├── Motion/
│   │   │   ├── ParticleSpace/
│   │   │   ├── FluidSpace/
│   │   │   ├── DynamicsSpace/
│   │   │   ├── FractureNetwork/
│   │   │   └── AnimationSpace/
│   │   ├── Behaviour/
│   │   │   ├── BehaviourSpace/
│   │   │   └── InputSpace/
│   │   ├── Sound/
│   │   │   ├── SoundSpace/
│   │   │   └── AcousticSpace/
│   │   └── Project/
│   │       ├── ProjectDescriptor/
│   │       ├── EditorPreferences/
│   │       ├── WorldSpace/
│   │       ├── RevisionSpace/
│   │       ├── OriginLedger/
│   │       └── ReplicationSpace/
│   ├── Instances/                   Instance container: header, directory, linking
│   ├── Interchange/                 OBJ, glTF, SVG, EXR, PNG, KTX2, IES, MaterialX
│   ├── Shaders/                     .slang, grouped by the same groups
│   └── Workspaces/                  editor UIs, one folder per Space group
├── Content/                         default content, mirrors the groups
│   ├── Shape/   ├── Raster/   ├── Vector/   ├── Appearance/
│   ├── Optics/  ├── Climate/  ├── Motion/   ├── Behaviour/
│   └── Sound/   └── Project/
└── Documentation/
    ├── Formats/                     one page per Space, replaces scattered plans
    └── EngineFormatNamingPlan.md    this file
```

**Rules**

- Each Space folder contains exactly one codec pair (`<Name>Codec.h/.cpp`), its section types, a `README.md`, and a `<Name>Validation` executable.
- No flat `Codecs/` folder. Codecs live beside their types.
- `Content/` folders use the group names, not the old `Photometry` or `SurfaceParameters` names.

---

## 6. Rename Ledger (From the Linked Branch)

| Old name | Occurrences (approx.) | New name |
| :--- | :---: | :--- |
| `.VectorFieldSpace`, `VectorFieldCodec`, `VectorFieldWorkspace`, `VectorFieldVisualizer`, `VectorFieldOverlaySequence` | ~130 | `ForceSpace` / `VelocitySpace` (decide per use); overlays become `…Rasterization` |
| `VectorGraphics`, `VectorGraphicsGPUValidation`, `VectorGraphicsWorkspacePlan.md` | ~21 | `PathSpace`, `PathRasterizationValidation`, `PathWorkspacePlan.md` |
| `.ProjectSpace`, `ProjectSpaceRepository`, `ProjectSpaceDocument` | ~130 | `ProjectDescriptor`, `ProjectDescriptorRepository` |
| `PixelSheet`, `PixelSheetCodec`, `.PixelSheet` | ~12 | `PixelSpace` for raster; `SpriteSheet` for packed pages |
| `PixelSpace` (existing component class in `Engine/Components/`) | ~638 | **Collision, see §7** |
| `SkyAtmosphereCodec`, `.SkyAtmosphere` | ~22 | `AtmosphereSpace`, `AtmosphereCodec` |
| `LensApertureCodec`, `.LensAperture` | ~8 | `LensSpace`, `LensCodec`; old name becomes the `Aperture` section |
| `FractureTreeCodec`, `.FractureTree` | ~10 | `FractureNetworkCodec`, `.FractureNetwork` |
| `WorldSerializationDagPlan.md`, `DAG`, `DependencyDAG` | ~74 | `WorldSerializationOrderPlan.md`, `DependencyNetwork` |
| `Provenance`, `ProvenanceInspector`, `Provenance.json` | ~41 | `OriginLedger`, `OriginInspector`, `Origin.json` |
| `DynamicMaterialRecipe`, `WaveformSynthesisRecipe`, `Recipe.*` | ~140 | `DynamicMaterialFormula`, `WaveformSynthesisFormula`, `Formula.*` |
| `ScriptingSpace` (if present) | — | `BehaviourSpace` |
| `EditorConfiguration` (if present) | — | `EditorPreferences` |
| `CurveSpace.Environment` (if present) | — | `CurveSpace` + `ClimateSpace` |

**Read-only aliases during migration:** the reader accepts the old suffixes (case-insensitive) and maps them to the new Space, so existing `.VolumetricSpace` and `.ProjectSpace` files still open. Writers always emit the new names.

---

## 7. Collisions and Risks

| # | Risk | Mitigation |
| :--- | :--- | :--- |
| C1 | `PixelSpace` is already a class in `Engine/Components/PixelSpace.h` and is referenced ~638 times. | Rename the existing component to a different concrete name, or keep it and use another name for the raster Space. **Decision D3.** |
| C2 | `ImageSpace` and `PixelSpace` both mean raster today. | Merge `ImageSpace` into `PixelSpace` (R5). `TextureSpace` stays as the GPU packing step. |
| C3 | `NetworkingSpace` and the new `Network` word overlap. | Rename `NetworkingSpace` to `ReplicationSpace`, so `Network` only means linked structure. |
| C4 | `Instance` also means a runtime object in `DocumentFormatsInstancePlan`. | Keep `Instance` for the file, and use `PlacedInstance` for runtime objects. |
| C5 | `Trigger` is an existing instance category. | Keep it. `BehaviourSpace.Trigger` is a section, so there is no clash. |
| C6 | `Layer` is used by `LayerStack`. | Use `Part` or `Segment` for section names where a clash appears. |
| C7 | `Formula` may read as maths. | Acceptable. Its doc comment states it holds generator parameters. |
| C8 | Existing binary files use old extensions. | Read aliases (§6). Re-save on edit. |

---

## 8. Migration Phases

| Phase | Work | Exit gate |
| :--- | :--- | :--- |
| **0. Decisions** | Settle D1–D5 (§9). Update `BannedWordsVocabulary.md` with the user additions from R1. | Decisions recorded |
| **1. Docs** | Rename the plan files in §6. Write one `Documentation/Formats/<Space>.md` per Space. | Zero banned words in `Documentation/` (grep gate) |
| **2. Codecs** | Rename codec classes and `<Name>Codec.h/.cpp`. Add read aliases. | Old and new files both open; round-trip tests pass |
| **3. Folders** | Move codecs into `Engine/Spaces/<Group>/<Space>/`. Update CMake. | Full build on all three profiles |
| **4. Content** | Rename default `Content/` folders and files. | Scene load tests pass |
| **5. Instances** | Implement the Instance container and section directory. Migrate `VehicleDefault`. | Round-trip of a multi-section file |

---

## 9. Decisions Needed

| ID | Question | Recommendation |
| :--- | :--- | :--- |
| **D1** | Codec class naming: `AtmosphereCodec` (drop `Space`) or `AtmosphereSpaceCodec` (keep)? | Drop `Space` (R6). Matches your `AtmosphereCodec` request |
| **D2** | Container suffix: `.Instance` or `.Document`? | `.Instance` (your example). `.Document` is the other option |
| **D3** | Keep `PixelSpace` for the raster Space and rename the existing component class, or keep the component and name the raster Space differently? | Keep `PixelSpace` for raster. Rename the component |
| **D4** | Split `VectorField` into `ForceSpace` and `VelocitySpace`, or keep one `ForceSpace`? | Split. Forces and fluid motion have different units and layouts |
| **D5** | Drop `Space` from folder-level group names (`Motion` instead of `MotionSpace`)? | Yes. Groups are folders. Spaces are files |
