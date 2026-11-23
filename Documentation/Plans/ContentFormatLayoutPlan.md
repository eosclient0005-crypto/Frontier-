# Frontier Content Format / Folder Layout Plan

## 1. Decision in one page

This plan uses a small, readable native document vocabulary. A suffix describes the payload that a file owns. A compound file uses `.Instance`, exactly as in `VehicleDefault.Instance`. It can carry compact owned payloads inline or point at reusable content files.

The selected vocabulary is:

```text
Instance                    GeometrySpace              SurfaceParameterSpace
PixelSpace                  TextureSpace               MaterialSpace
ContourSpace                ColourSpace                TypefaceSpace
MotionSpace                 BehaviourSpace             DynamicsSpace
ForceSpace                  FluidSpace                 VolumeSpace
ParticleSpace               FractureSpace              RigSpace
AtmosphereSpace             TerrainSpace               LightSpace
LensSpace                   CameraSpace                AudioSpace
InputSpace                  NetworkSpace               WorldSpace
WorkspaceSettings           ContentIndex
```

Four decisions remove the ambiguity in the prior direction:

1. **`PixelSpace` is the raster-image document.** Its reader/writer is `PixelSheetCodec`.
2. **`TextureSpace` is sampling-ready surface imagery.** It can point at, or inline, one or more `PixelSpace` payloads. A pixel image is not automatically a texture.
3. **A vector quantity never names the outer document by itself.** Force, fluid, particle, or volume documents describe what the quantity means. Each can contain a velocity, direction, pressure, density, or force section.
4. **`Instance` is the cross-category envelope.** It is the only format intended to own geometry, pixels, textures, materials, physics, behaviour, camera, light, audio, plus similar payloads in one portable unit.

`AtmopherCodec`, `LensCodec`, `FractureNetworkCodec`, plus `PixelSheetCodec` are retained as the selected codec names requested for those four cases.

---

## 2. Naming grammar

### 2.1 File grammar

```text
<DescriptiveNoun>.<PayloadSuffix>
```

Examples:

```text
VehicleDefault.Instance
VehicleChassis.GeometrySpace
VehiclePaint.PixelSpace
VehiclePaint.TextureSpace
VehiclePaint.MaterialSpace
DashboardContour.ContourSpace
StormCell.FluidSpace
SummerDay.AtmosphereSpace
WidePrime.LensSpace
ConcreteWall.FractureSpace
```

Rules:

- Use one native suffix per file.
- Use singular PascalCase suffixes; use plural folder names.
- Put ownership or subject matter in the filename stem, not in the suffix.
- Use a physical or authoring meaning for the suffix. Do not use a location, editor screen, catch-all world grouping, or broad product label as a payload suffix.
- A codec name describes the complete document it reads or writes. It must not name one small control when the document covers a larger whole.
- A native document can contain `EmbeddedSection` blocks plus `ReferenceSection` blocks. Inline payloads retain their own selected payload suffix in `SectionFormat`.

### 2.2 Codec grammar

The normal form is:

```text
<PayloadName>Codec
```

Examples: `GeometryCodec`, `MaterialCodec`, `FluidCodec`, `BehaviourCodec`.

There are four deliberate exceptions:

| Native suffix | Selected codec | Reason |
| --- | --- | --- |
| `PixelSpace` | `PixelSheetCodec` | A pixel document can carry tiled, layered, multi-plane 2-D sheets. |
| `AtmosphereSpace` | `AtmopherCodec` | Covers the complete aerial medium, not merely a sky preset. |
| `LensSpace` | `LensCodec` | Covers glass, aperture, focus, distortion, coating, plus imaging details. |
| `FractureSpace` | `FractureNetworkCodec` | Break relations form a concrete fracture network. |

### 2.3 Stable section names

All native documents use the same small outer vocabulary:

```text
DocumentHeader
SectionDirectory
EmbeddedSection
ReferenceSection
OriginSection
BuildMethod
RevisionSection
ReferenceNetwork
BinaryBlock
ContentFingerprint
```

`OriginSection` names where a payload came from. `BuildMethod` names how a derived payload was produced. `ReferenceNetwork` names cross-file relations. These names avoid vague historical or procedural labels while remaining useful during import, rebuild, review, or interchange.

---

## 3. Document envelope

Every native document has three physical regions:

```text
DocumentHeader       readable identity, units, schema revision, authoring details
SectionDirectory     each section name, format, byte range, fingerprint, placement
BinaryBlock          compact numeric payloads, pixel planes, samples, geometry streams
```

The header stays readable. Heavy payloads sit in binary blocks with aligned offsets. Each section can be one of two placements:

| Placement | Use it when | Example |
| --- | --- | --- |
| `Embedded` | The payload is small, private to one owner, or needed for a single-file handoff. | A vehicle-specific colour ramp within `VehicleDefault.Instance`. |
| `Reference` | The payload is large, reusable, independently revised, or shared by many instances. | `VehiclePaint.MaterialSpace` referenced by several vehicle instances. |

A reference carries a relative file name plus `ContentFingerprint`. A portable handoff may add an embedded fallback, but the normal working copy keeps reusable payloads external.

### 3.1 What belongs inline

| Owner format | Good inline sections | Usually external |
| --- | --- | --- |
| `Instance` | transforms, placement readings, small colour sets, one-off behaviour parameters, lightweight force settings | detailed geometry, large pixel sheets, reusable materials, long audio, dense volumes |
| `MaterialSpace` | scalar readings, colour readings, sampler settings, a tiny procedural texture | reusable pixel sheets, large texture groups |
| `ParticleSpace` | particle birth readings, colour ramp, local force definition, behaviour parameters | reusable material, pixel sheet, large collision geometry |
| `FluidSpace` | solver readings, local force definition, small boundary description | high-resolution volume samples, collision geometry, reusable pixel sheet |
| `AtmosphereSpace` | sun, air, cloud, fog, weather readings | dense cloud volumes, reusable star pixel sheet, terrain |
| `LensSpace` | glass sequence, aperture, focus, distortion, coating readings | large calibration pixel sheets |
| `FractureSpace` | break thresholds, fracture network, material links | source geometry, high-resolution baked debris geometry |

### 3.2 `VehicleDefault.Instance` example

```toml
[DocumentHeader]
DocumentName = "VehicleDefault"
DocumentFormat = "Instance"
FormatRevision = 1

[[EmbeddedSection]]
SectionName = "Chassis"
SectionFormat = "GeometrySpace"
SectionPurpose = "VehicleBody"
Placement = "Embedded"

[[ReferenceSection]]
SectionName = "Paint"
SectionFormat = "MaterialSpace"
SectionPurpose = "OuterFinish"
Placement = "Reference"
FileName = "../../Library/Materials/Vehicles/VehiclePaint.MaterialSpace"
ContentFingerprint = "..."

[[ReferenceSection]]
SectionName = "PaintPixels"
SectionFormat = "PixelSpace"
SectionPurpose = "AlbedoSheet"
Placement = "Reference"
FileName = "../../Library/Pixels/Vehicles/VehiclePaint.PixelSpace"
ContentFingerprint = "..."

[[EmbeddedSection]]
SectionName = "WindResponse"
SectionFormat = "ForceSpace"
SectionPurpose = "VehicleForceResponse"
Placement = "Embedded"

[[EmbeddedSection]]
SectionName = "DrivingBehaviour"
SectionFormat = "BehaviourSpace"
SectionPurpose = "VehicleControl"
Placement = "Embedded"
```

The instance owns a complete vehicle description without flattening every reusable file into one large binary. Geometry, pixel imagery, texture sampling, materials, force behaviour, particle effects, light, audio, camera, plus similar payloads use the same embedding rule.

---

## 4. Selection catalogue

Every row has one selected name plus five further viable choices. The first bold entry is the recommended canonical choice. Alternatives are useful only before the first public writer ships; after that, select one name and keep it stable.

### 4.1 Compound / placement documents

| Payload purpose | Six candidate suffixes | Selected use |
| --- | --- | --- |
| Compound portable unit | **`Instance`**, `Bundle`, `Package`, `Compound`, `Unit`, `Preset` | A vehicle, prop, character, tool, effect, or reusable authored unit. |
| World placement | **`WorldSpace`**, `SceneSpace`, `LevelSpace`, `LocationSpace`, `WorldLayoutSpace`, `SettingSpace` | A loadable world containing instance placement plus world-level readings. |
| Content index | **`ContentIndex`**, `LibraryIndex`, `FileIndex`, `ReferenceIndex`, `DocumentIndex`, `ContentRegister` | Generated lookup file; not a broad authoring container. |
| Workspace-only preferences | **`WorkspaceSettings`**, `StudioSettings`, `AuthoringSettings`, `EditorSettings`, `ToolSettings`, `SessionSettings` | Local authoring preferences; keep this outside shipped content. |
| Cross-file relation section | **`ReferenceNetwork`**, `LinkNetwork`, `RelationNetwork`, `ConnectionNetwork`, `DependencyNetwork`, `CrossLinkNetwork` | A section within an instance, world, fracture, or material document. |

### 4.2 Shape / surface / imagery documents

| Payload purpose | Six candidate suffixes | Selected use |
| --- | --- | --- |
| 3-D shape | **`GeometrySpace`**, `ShapeSpace`, `SolidSpace`, `PolygonSpace`, `FaceSpace`, `FormSpace` | Points, faces, normals, creases, surface links, compact acceleration readings. |
| Surface coordinates | **`SurfaceParameterSpace`**, `TextureCoordinateSpace`, `CoordinateSpace`, `ChartSpace`, `UvSpace`, `SurfaceLayoutSpace` | Surface coordinates, seams, pins, density readings, chart placement. |
| Raster imagery | **`PixelSpace`**, `PixelSheet`, `RasterSpace`, `CanvasSpace`, `ImageSpace`, `PixelPlane` | Colour-managed pixel planes, HDR imagery, masks, decals, baked sheets. |
| Sampling-ready imagery | **`TextureSpace`**, `TexelSpace`, `SamplingSpace`, `SurfaceTextureSpace`, `ChannelTextureSpace`, `TextureSheet` | Sampler readings, resolution chain, channel packing, surface coordinate link. |
| Surface response | **`MaterialSpace`**, `SurfaceMaterialSpace`, `FinishSpace`, `ShadingSpace`, `OpticalMaterialSpace`, `SurfaceResponseSpace` | Reflection, transmission, roughness, coating, plus texture links. |
| SVG / editable contours | **`ContourSpace`**, `PathSpace`, `StrokeSpace`, `ShapeOutlineSpace`, `SvgSpace`, `OutlineSpace` | Contours, fills, strokes, winding, typography outlines; SVG exchange belongs here. |
| Colour definition | **`ColourSpace`**, `ChromaticSpace`, `SpectrumSpace`, `SwatchSpace`, `ColourProfileSpace`, `ToneSpace` | Primaries, transfer curve, white point, spectral readings, colour sets. |
| Typeface definition | **`TypefaceSpace`**, `FontSpace`, `LetterformSpace`, `TextFaceSpace`, `TypeSpace`, `FontMetricSpace` | Outline glyphs, shaping, metrics, kerning, optical size readings. |

### 4.3 Motion / behaviour / physical documents

| Payload purpose | Six candidate suffixes | Selected use |
| --- | --- | --- |
| Timed movement | **`MotionSpace`**, `AnimationSpace`, `KinematicSpace`, `PoseSpace`, `TrackSpace`, `MovementSpace` | Curves, key readings, clips, constraints, blend readings. |
| Runtime behaviour | **`BehaviourSpace`**, `LogicSpace`, `ControlSpace`, `ResponseSpace`, `RoutineSpace`, `ProgramSpace` | Behaviour source, compiled bytecode, inputs, outputs, activation rules. |
| Rigid physical motion | **`DynamicsSpace`**, `MechanicsSpace`, `RigidBodySpace`, `CollisionSpace`, `ConstraintSpace`, `MotionBodySpace` | Mass, collision form, joints, constraints, solver readings. |
| Applied forces | **`ForceSpace`**, `InfluenceSpace`, `AccelerationSpace`, `ImpulseSpace`, `ForceFieldSpace`, `ForceVolumeSpace` | Directed force, radial force, curl force, attenuation, source geometry. |
| Fluids / gases | **`FluidSpace`**, `LiquidSpace`, `GasSpace`, `HydroSpace`, `AeroSpace`, `TransportSpace` | Velocity, pressure, density, viscosity, boundaries, solver readings. |
| Sampled 3-D medium | **`VolumeSpace`**, `DensitySpace`, `VoxelSpace`, `SpatialSampleSpace`, `MediumSpace`, `FieldVolumeSpace` | Density, distance, temperature, velocity, colour, occupancy samples. |
| Particulate matter | **`ParticleSpace`**, `MoteSpace`, `ParticleSet`, `ParticleMotionSpace`, `SpraySpace`, `GranuleSpace` | Birth rules, particle readings, colour, force links, trails, material link. |
| Breakable material | **`FractureSpace`**, `BreakageSpace`, `FragmentSpace`, `DestructionSpace`, `CrackSpace`, `ShatterSpace` | Break thresholds, fragment readings, fracture network, geometry link. |
| Joints / deformation | **`RigSpace`**, `ArticulationSpace`, `JointSpace`, `PoseRigSpace`, `DeformationSpace`, `KinematicRigSpace` | Joint layout, weights, deformation readings, motion links. |

### 4.4 World-media / capture documents

| Payload purpose | Six candidate suffixes | Selected use |
| --- | --- | --- |
| Complete aerial medium | **`AtmosphereSpace`**, `AtmopherSpace`, `AirSpace`, `WeatherSpace`, `AerialSpace`, `AirMediumSpace` | Air scattering, sky, clouds, fog, sun, moon, weather, local aerial volumes. |
| Ground / landscape | **`TerrainSpace`**, `LandscapeSpace`, `GroundSpace`, `LandformSpace`, `HeightSpace`, `BiomeSpace` | Height, material regions, vegetation placement, erosion readings. |
| Light definition | **`LightSpace`**, `LuminaireSpace`, `PhotometrySpace`, `IlluminationSpace`, `RadianceSpace`, `LightProfileSpace` | Measured light, area light, sun, colour, distribution, intensity. |
| Complete lens | **`LensSpace`**, `OpticSpace`, `OpticalSpace`, `CameraLensSpace`, `LensProfileSpace`, `ImagingOpticsSpace` | Glass, aperture, focus, distortion, coating, flare, calibration. |
| Camera body | **`CameraSpace`**, `ViewSpace`, `CaptureSpace`, `ProjectionSpace`, `CameraRigSpace`, `ViewpointSpace` | Sensor, projection, shutter, exposure, lens link, motion link. |
| Sound / acoustics | **`AudioSpace`**, `SoundSpace`, `AcousticSpace`, `WaveSpace`, `VoiceSpace`, `SpatialAudioSpace` | Samples, spatial placement, loudness, reverberation, playback readings. |
| Input layout | **`InputSpace`**, `DeviceSpace`, `GestureSpace`, `CommandSpace`, `InteractionSpace`, `ControlInputSpace` | Device profile, control mapping, axis calibration, gesture readings. |
| Multi-user transport | **`NetworkSpace`**, `SessionSpace`, `TransportSpace`, `ExchangeSpace`, `LinkSpace`, `PeerSpace` | Session transfer, replication choices, authority readings, transport details. |

### 4.5 Deliberate selections

The following choices should be locked first because they resolve the terms called out in the request:

```text
PixelSpace          -> PixelSheetCodec
AtmosphereSpace     -> AtmopherCodec
LensSpace           -> LensCodec
FractureSpace       -> FractureNetworkCodec
BehaviourSpace      -> BehaviourCodec
ContourSpace        -> ContourCodec
```

`AtmopherCodec` is retained as the requested house spelling. `AtmosphereSpace` stays readable as the suffix seen in folders, file browsers, interchange screens, plus technical documentation.

---

## 5. Format boundaries

### 5.1 Pixels versus textures

These formats must stay separate:

| Concern | `PixelSpace` | `TextureSpace` |
| --- | --- | --- |
| Main meaning | Authored or captured pixel imagery | GPU or software sampling description |
| Typical payload | Pixel planes, HDR ranges, masks, decals, baked sheet | Pixel reference, sampler readings, mip sequence, channel packing |
| Typical owner | Concept art, decal, photo, flare sheet, render result | Material, terrain, particle, UI surface, volume lookup |
| Selected codec | `PixelSheetCodec` | `TextureCodec` |

A `TextureSpace` can point at a `PixelSpace`, but a `PixelSpace` does not need texture sampler readings. This prevents one overloaded image format from serving two unrelated jobs.

### 5.2 Contours versus geometry

`ContourSpace` owns editable 2-D paths, fills, strokes, plus SVG exchange. `GeometrySpace` owns 3-D surfaces. A contour can feed geometry construction, but it remains a contour document until a geometry writer produces a new `GeometrySpace` result.

A generic curve suffix is intentionally absent. Curves occur in contours, motion, rig deformation, force paths, plus geometry construction. Their meaning belongs to the owner format.

### 5.3 Force, fluid, volume, particle

These four formats are related but not interchangeable:

```text
ForceSpace      = a physical influence definition
FluidSpace      = a fluid or gas solver description
VolumeSpace     = sampled 3-D readings
ParticleSpace   = many moving particle readings plus force, colour, material links
```

A velocity quantity can live inside `FluidSpace`, `ParticleSpace`, or `VolumeSpace`. A force quantity can live inside `ForceSpace`, then be referenced by particles or fluids. This gives a developer a clear answer to “what does this vector mean?” without a catch-all vector document.

### 5.4 Atmosphere versus volume

`AtmosphereSpace` owns the complete aerial presentation: air, sun, sky, clouds, fog, weather, plus optional local volumes. `VolumeSpace` owns one sampled 3-D payload. A cloud density field is a `VolumeSpace`; a playable daytime sky with clouds is an `AtmosphereSpace` that points at that volume.

### 5.5 Lens versus aperture

An aperture is only one lens control. `LensSpace` therefore owns the complete imaging optic. `LensCodec` reads the whole optic document. A camera body lives in `CameraSpace` and references a lens file.

### 5.6 Fracture relation layout

`FractureSpace` owns material break settings plus a `ReferenceNetwork` that names fragment connections. `FractureNetworkCodec` reads and writes that relation section. This works for pre-cut stone, glass cracks, vehicle damage, or procedural breakage without placing structural meaning in a misleading name.

---

## 6. Canonical folders

The folder layout separates reusable library content from owner-local compound units. Folders describe authoring purpose. File suffixes remain the source of type identity.

```text
Frontier/
├── Content/
│   ├── Library/
│   │   ├── Geometry/
│   │   ├── SurfaceParameters/
│   │   ├── Pixels/
│   │   ├── Textures/
│   │   ├── Materials/
│   │   ├── Contours/
│   │   ├── Colours/
│   │   ├── Typefaces/
│   │   ├── Motion/
│   │   ├── Behaviour/
│   │   ├── Dynamics/
│   │   ├── Forces/
│   │   ├── Fluids/
│   │   ├── Volumes/
│   │   ├── Particles/
│   │   ├── Fracture/
│   │   ├── Rigs/
│   │   ├── Atmospheres/
│   │   ├── Terrain/
│   │   ├── Lights/
│   │   ├── Lenses/
│   │   ├── Cameras/
│   │   ├── Audio/
│   │   ├── Input/
│   │   └── Networks/
│   │
│   ├── Instances/
│   │   ├── Vehicles/
│   │   │   └── VehicleDefault/
│   │   │       ├── VehicleDefault.Instance
│   │   │       ├── VehiclePreview.PixelSpace
│   │   │       └── VehicleNotes.txt
│   │   ├── Characters/
│   │   ├── Props/
│   │   └── Effects/
│   │
│   ├── Worlds/
│   │   ├── VehicleShowroom.WorldSpace
│   │   └── RainyRoad.WorldSpace
│   │
│   ├── Incoming/
│   │   ├── Geometry/
│   │   ├── Pixels/
│   │   ├── Contours/
│   │   └── Audio/
│   │
│   └── Frontier.ContentIndex
│
├── Workspace/
│   └── Local/
│       └── Frontier.WorkspaceSettings
│
├── Engine/
│   ├── DocumentWire/
│   │   ├── DocumentEnvelopeCodec.h
│   │   ├── SectionDirectoryCodec.h
│   │   └── ContentFingerprintCodec.h
│   ├── Codecs/
│   │   ├── Geometry/GeometryCodec.h
│   │   ├── Pixels/PixelSheetCodec.h
│   │   ├── Textures/TextureCodec.h
│   │   ├── Materials/MaterialCodec.h
│   │   ├── Contours/ContourCodec.h
│   │   ├── Behaviour/BehaviourCodec.h
│   │   ├── Forces/ForceCodec.h
│   │   ├── Fluids/FluidCodec.h
│   │   ├── Particles/ParticleCodec.h
│   │   ├── Atmospheres/AtmopherCodec.h
│   │   ├── Lenses/LensCodec.h
│   │   └── Fracture/FractureNetworkCodec.h
│   └── Interchange/
│       ├── SvgExchange/
│       ├── PixelExchange/
│       ├── GeometryExchange/
│       └── AudioExchange/
│
└── Documentation/
    └── Plans/
        └── ContentFormatLayoutPlan.md
```

### 6.1 Folder rules

1. `Library/` contains reusable standalone files only.
2. `Instances/<Category>/<Name>/` contains one compound root plus owner-local previews or notes.
3. `Worlds/` contains loadable placement documents only.
4. `Incoming/` holds external source files before conversion. It never becomes a runtime lookup location.
5. `Workspace/Local/` is per-user authoring material. It never becomes a content dependency.
6. A folder is plural; a suffix is singular.
7. Do not create a broad folder for unrelated visual media, world media, user preferences, plus code. Split by the selected format vocabulary.

---

## 7. Codec roster

| Document suffix | Reader / writer | Principal responsibility |
| --- | --- | --- |
| `Instance` | `InstanceCodec` | Compound unit, inline sections, external references, transforms. |
| `WorldSpace` | `WorldCodec` | World placement, world-level light, atmosphere, camera, instance references. |
| `GeometrySpace` | `GeometryCodec` | Geometry streams, faces, normals, surface links. |
| `SurfaceParameterSpace` | `SurfaceParameterCodec` | Surface coordinates, seams, pins, chart readings. |
| `PixelSpace` | `PixelSheetCodec` | Pixel sheets, planes, HDR range, colour link. |
| `TextureSpace` | `TextureCodec` | Sampling readings, resolution chain, channel packing. |
| `MaterialSpace` | `MaterialCodec` | Surface response, texture links, shading readings. |
| `ContourSpace` | `ContourCodec` | Paths, strokes, fills, SVG exchange. |
| `ColourSpace` | `ColourCodec` | Colour profile, spectrum, swatches, transfer curve. |
| `TypefaceSpace` | `TypefaceCodec` | Letterforms, shaping, metrics, kerning. |
| `MotionSpace` | `MotionCodec` | Timed movement, clips, curves, blend readings. |
| `BehaviourSpace` | `BehaviourCodec` | Behaviour source, bytecode, input/output readings. |
| `DynamicsSpace` | `DynamicsCodec` | Bodies, collision form, constraints, solver readings. |
| `ForceSpace` | `ForceCodec` | Physical influence definitions plus sampled force sections. |
| `FluidSpace` | `FluidCodec` | Fluid/gas readings, boundary form, volume links. |
| `VolumeSpace` | `VolumeCodec` | Sampled 3-D payloads. |
| `ParticleSpace` | `ParticleCodec` | Particle population, colour, force, material, trail readings. |
| `FractureSpace` | `FractureNetworkCodec` | Break settings, fragment relation network, geometry links. |
| `RigSpace` | `RigCodec` | Joints, deformation, weights, motion links. |
| `AtmosphereSpace` | `AtmopherCodec` | Complete aerial medium plus linked local volumes. |
| `TerrainSpace` | `TerrainCodec` | Ground form, regions, material, vegetation placement. |
| `LightSpace` | `LightCodec` | Measured / analytic light, intensity, distribution. |
| `LensSpace` | `LensCodec` | Complete imaging optic. |
| `CameraSpace` | `CameraCodec` | Camera body, projection, exposure, lens link. |
| `AudioSpace` | `AudioCodec` | Sound payload, spatial placement, acoustics. |
| `InputSpace` | `InputCodec` | Device profile, gestures, control mapping. |
| `NetworkSpace` | `NetworkCodec` | Session transport plus replication readings. |
| `WorkspaceSettings` | `WorkspaceSettingsCodec` | Local authoring choices only. |
| `ContentIndex` | `ContentIndexCodec` | Generated content lookup. |

---

## 8. Worked nesting examples

### 8.1 Particle effect

```text
SparkRain.ParticleSpace
├── ParticleBirth                 inline
├── ColourSpace                   inline
├── ForceSpace                    reference or inline
├── MaterialSpace                 reference
├── PixelSpace                    indirectly referenced by MaterialSpace
├── BehaviourSpace                inline
└── VolumeSpace                   optional reference for collision or density
```

A particle document directly owns particle-specific readings. It does not relabel a generic force or pixel payload as particle content merely because it is used there.

### 8.2 Fluid effect

```text
StreetDrain.FluidSpace
├── DynamicsSpace                 inline
├── ForceSpace                    reference
├── Velocity VolumeSpace          reference
├── Pressure VolumeSpace          reference
├── Density VolumeSpace           reference
├── MaterialSpace                 reference
└── BehaviourSpace                optional inline
```

A fluid writer can generate one or more `VolumeSpace` files. Those volumes remain independently reusable by rendering, particles, atmosphere, or diagnostics.

### 8.3 Atmosphere

```text
SummerDay.AtmosphereSpace
├── Air readings                  inline
├── LightSpace                    reference for sun or moon
├── Cloud Density VolumeSpace     reference
├── Fog VolumeSpace               reference
├── PixelSpace                    reference for star or moon sheet
└── BehaviourSpace                optional inline for weather changes
```

### 8.4 Lens / camera

```text
WidePrime.LensSpace
├── Glass readings                inline
├── Aperture readings             inline
├── Focus readings                inline
├── Distortion readings           inline
├── Coating readings              inline
└── PixelSpace                    optional reference for calibration sheet

ShowroomCamera.CameraSpace
├── Camera body readings          inline
├── LensSpace                     reference
└── MotionSpace                   optional reference
```

### 8.5 Breakable object

```text
ConcreteWall.FractureSpace
├── GeometrySpace                 reference
├── MaterialSpace                 reference
├── FractureNetwork               inline
├── Break thresholds              inline
└── ParticleSpace                 optional reference for dust
```

---

## 9. Explicit correction rules

Use these rules while reviewing every existing or planned file name:

| Situation | Selected correction |
| --- | --- |
| Raster image reader/writer | `PixelSheetCodec`, writing `PixelSpace`. |
| Entire aerial medium reader/writer | `AtmopherCodec`, writing `AtmosphereSpace`. |
| Complete lens reader/writer | `LensCodec`, writing `LensSpace`. |
| Breakable material relation reader/writer | `FractureNetworkCodec`, writing `FractureSpace`. |
| SVG or editable vector imagery | `ContourCodec`, writing `ContourSpace`. |
| Curves used for a motion, contour, force, or geometry purpose | Keep the curve section inside that owner document. |
| User preferences | `WorkspaceSettings`; keep them out of content folders. |
| Root product metadata | `ContentIndex` plus `WorkspaceSettings`; do not create one broad catch-all content suffix. |
| Source history | `OriginSection`, `BuildMethod`, `ContentFingerprint`. |
| Non-linear references | `ReferenceNetwork`, with concrete relation labels. |

---

## 10. Delivery sequence

1. **Freeze the vocabulary.** Confirm the selected suffixes in section 1 plus the requested `AtmopherCodec` spelling.
2. **Build the shared envelope.** Implement `DocumentEnvelopeCodec`, `SectionDirectoryCodec`, fingerprints, reference resolution, plus version checks before category codecs.
3. **Ship the visual spine.** Implement geometry, surface parameters, pixels, textures, materials, contours, colours, typefaces.
4. **Ship the physical spine.** Implement motion, behaviour, dynamics, force, fluid, volume, particle, fracture, rig.
5. **Ship the world spine.** Implement atmosphere, terrain, light, lens, camera, audio, input, network, world, instance.
6. **Convert incoming content.** One converter per external source family writes the selected native suffix rather than importing into a generic container.
7. **Add round-trip checks.** Each codec must read its own output byte-for-byte where lossless output is promised, resolve external references, plus preserve every embedded section.
8. **Lock public writers.** Readers may retain old-name compatibility during a transition. Writers produce only the selected suffixes, sections, folder layout, plus codec names.

---

## 11. Review checklist

Before adding a suffix, folder, codec, section, or filename, verify all of the following:

- Does it name one concrete payload or one concrete owner?
- Can a developer tell whether it is a pixel sheet, a sampler description, a material, a volume, a force, or a fluid without opening the file?
- Does the codec cover the full payload rather than a single control?
- Does an embedded section retain its native format identity?
- Is the file reusable? If yes, should it become a `Library/` reference rather than an inline copy?
- Is the folder an authoring category rather than a broad dumping location?
- Does the proposed identifier pass the current vocabulary ruling before it reaches source, documentation, or content?

This plan deliberately makes the content format catalogue broad enough for geometry, raster imagery, SVG, materials, particles, fluids, forces, atmosphere, optics, audio, world placement, plus future payload families without forcing them into one vague generic suffix.
