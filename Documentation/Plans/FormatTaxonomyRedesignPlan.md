# Frontier Format Taxonomy — Redesign Plan

> **What this replaces**: `Documentation/Plans/WorldSerializationDagPlan.md` (the 56-suffix
> registry drafted earlier). Everything in it that was mechanically sound is kept; every
> name that violated a banned word, or used `Space` as decoration instead of meaning, is
> re-derived here.
>
> **Companion artifacts, all generated from one table** (`Tools/FormatTaxonomy.py`):
> · [`Naming/FormatCandidateLedger.md`](../Naming/FormatCandidateLedger.md) — 91 format slots,
> **five or more alternatives each** (637 names total)
> · [`Naming/FormatSuffixRegistry.toml`](../Naming/FormatSuffixRegistry.toml) — the machine-readable law
> · `Tools/NamingAudit.py` — proves every one of those names is legal; currently **0 failures**

---

## 0. Answers to the six objections

| # | Objection | Decision |
| :---: | :--- | :--- |
| 1 | `VectorField`, `.VectorGraphics`, `.CurveSpace`, `.EditorConfiguration`, `.ProjectSpace` are words I don't like | `.VectorGraphics` and every `…Graph` name are **barred by your own blacklist** (`graph`) — see §9. Vector 2-D art becomes **`.VectorSpace`**; a curve becomes plain **`.Curve`** (no `Space`); `.EditorConfiguration` is split into **`.WorkspaceLayout`** (editor toggles) and **`.RuntimeConfiguration`** (machine boot); `.ProjectSpace` becomes **`.ProjectIndex`**. |
| 2 | Banned words are in use | `DependencyDAG` → **`Engine/Serialization/Derivation/`**, `BakeRecipeScheduler` → **`BakeScheduler`**, `BakeRecipe` → **`BakeSpecification`**, `ProvenanceInspector` → **`DerivationReporter`**, `NextPeerIndex` → **`FollowingIndex`**. `dag`, `recipe`, `provenance`, `peer`, `studio`, `script`, `tree` are now **enforced bars** in `Tools/FormatNamingVocabulary.py`, not just prose in a document. |
| 3 | `PixelSheetCodec` — wasn't it PixelSpace? | Correct. Raster authoring is a domain: **`.PixelSpace`** owned by **`PixelCodec`**. What was left over — packed pages of sub-images — is a *packing detail*, so it becomes **`.SpritePlate`** / **`.TexturePlate`**, never a domain name. |
| 4 | `SkyAtmosphereCodec` → `AtmosphereCodec` | Done: **`.Atmosphere`** + `AtmosphereCodec`. "Sky" is one section of it, alongside `AirProfile`, `CelestialBodies`, `AerialPerspective`. |
| 5 | `LensApertureCodec` is wrong — it should describe the whole lens | Done: **`.Lens`** + `LensCodec` owns the *entire optical assembly*; `ApertureBlades` is a section inside it. Framing moved to **`.CameraView`**, sensor/exposure to **`.SensorResponse`** (§7). |
| 6 | Don't use `tree`; use `Network` | **`.FractureNetwork`** + `FractureNetworkCodec`. `tree` is barred in the vocabulary module, which also forces `BlendTree` → `BlendRegister` and `WideBoundingTree` → `BoundingLattice`. |

**Coverage claim, stated plainly**: the earlier draft enumerated 56 suffixes. This one
enumerates **91 format slots** across 15 groups, each with ≥5 vetted alternatives, plus the
tier model, the embedding contract, the resolution order, the alias-ownership law and a
runnable gate. Fluids, force fields, procedural scattering, cloth, ragdoll, crowd steering,
virtual texturing, colour measurement, typography, audio DSP, sessions and network policy are
all specified, not hand-waved.

---

## 1. Three laws, and they do all the work

**Law 1 — `Space` is a type, not a decoration.**
A format may end in `Space` **only** if it is a *domain*: content that is spatially or temporally
extended, is independently addressable, and is resolved whole by the runtime. There is nothing
*above* a domain to contain it, which is exactly why those — and only those — earn the suffix.
Everything else is a definition, a profile, a section, or a packing layout, and is named bare.
`Tools/NamingAudit.py` check **C2** fails any other `…Space` in a format name **or a folder name**,
so `Engine/SimulationSpace/` and `Engine/DistanceFieldSpace/` are gone as a side effect.

**Law 2 — a barred word is a barred word, in identifiers, formats, folders and comments.**
Enforced by substring where the word is distinctive (`dag`, `graph`, `mesh`, `cache`, `node`) and by
whole-token where English would otherwise break (`state`, `set`-verb, `core`, `base`, `scene`,
`model`, `value`). §9 lists the three places the previous blacklist contradicted *itself*, with the
correction to adopt.

**Law 3 — one codec per format, two framings.**
A codec's name is the format's stem plus `Codec`, and it never carries the container suffix:
**`GeometryCodec`**, never `GeometrySpaceCodec` (which fixes the four existing
`WorldSpaceCodec` / `VolumetricSpaceCodec` / `PhotometrySpaceCodec` / `ComponentSpaceCodec` units).
The *same* codec reads a standalone document and a payload embedded inside an Instance — a format
has one definition regardless of framing (check **C3**).

Corollaries that fall out of the laws: `Settings` = human-tunable, `Configuration` = machine-critical
(never `Config`), `Index` = a lookup listing other documents, `Register` = an indexed layout replacing
`table`/`map`/`array`, `Plate` = a packed 2-D page, `Network` = authored connectable structure,
`Sequence` = ordered executable steps, `Stack` = ordered layers, `Profile` = a measured or sampled
distribution over a parameter axis, `Volume` = bounded 3-D samples, `Lattice` = spatially indexed
samples with wrap/clip rules. Full class table in `Tools/FormatNamingVocabulary.py` → `SUFFIX_CLASSES`.

---

## 2. Three tiers: Instance → Space → section

```text
Tier 0  Content/Vehicles/ControlVehicle.Instance        composite, embeds domains, one per authored thing
            ├─ GeometrySpace   ├─ PixelSpace   ├─ TextureSpace   ├─ MaterialSpace
            ├─ FieldSpace      ├─ ParticleSpace├─ AudioSpace     ├─ BehaviourSpace
            └─ PhysicsSpace    └─ (any domain, embedded or linked)
Tier 1  Content/Geometry/Vehicles/ControlVehicleBody.GeometrySpace     a domain on its own, shareable
Tier 2  sections inside either framing: .PolygonNetwork, ApertureBlades, VelocityField, CoatingStack
```

An **Instance** is the unit you author and open. A **Space** is the unit the runtime resolves.
A **section** is the unit the baker invalidates. Tiers are not three formats; they are three
*framings* of the same two-tier wire container (TOML header + 64 B-aligned binary directory) already
used by `Content/*.Space`.

Why `.Instance` and not `.InstanceSpace`: an Instance is *not* a domain — it is the container that
holds domains. Giving it the same suffix as its own contents would destroy the one distinction Law 1
creates. This also settles the old `.InSpace` → `.InstanceSpace` debate: the abbreviation was wrong for
a different reason, and the correct name drops `Space` entirely.

---

## 3. Embedding contract (the part the earlier plan never specified)

Each embedded domain is a *section group* in the parent's Tier-2 directory. Its `FormatTag` is the
domain's own suffix, so `GeometryCodec` decodes it with the identical entry point it uses for a
standalone file. Payload bytes are **byte-identical** in both framings — no second schema to keep in
sync, which is the only way 91 formats stay maintainable.

```toml
['.DocumentHeader']
Format = 'Frontier.Instance'
DocumentRole = 'Vehicle'            # World | Vehicle | Effect | Prop | Character | Environment
DocumentTitle = 'Control Vehicle'
SchemaVersion = 1
ArchetypePath = 'EngineContent/Vehicles/StandardWheelBase.Instance'
BinaryAlignmentBytes = 64

[GeometryMetrics]
CreaseAngleDegrees = 38.0

[['.EmbeddedSpace']]                # by value: bytes live in this file
FormatTag = '.GeometrySpace'        # same tag, same codec, same section directory
EmbedMode = 'Embedded'
SectionGroup = 'Body'
ByteLength = 77016
ChecksumFnv1a64 = '0x8f3a1904c271d05b'

[['.EmbeddedSpace']]
FormatTag = '.MaterialSpace'
EmbedMode = 'Linked'                # by reference: shared, so not duplicated
DocumentPath = 'Content/Materials/Automotive/ControlVehicleMetallicPaint.MaterialSpace'

[['.EmbeddedSpace']]
FormatTag = '.FieldSpace'
EmbedMode = 'Inherited'             # from the archetype, unmodified
OverriddenFields = ['WakeStrength']

[['.EmbeddedSpace']]
FormatTag = '.BehaviourSpace'
EmbedMode = 'Embedded'
DocumentTitle = 'Driver Assistance Logic'

# === FRONTIER_BINARY_BOUNDARY ===
```

**Four rules.**

1. **Embed when it is not shareable; link when it is.** An effect's own colour ramp is embedded; the
   paint material four instances wear is linked. The test is *reuse*, not convenience.
2. **Resolution order is fixed**: `Embedded` bytes → `Linked` path → `Inherited` from archetype →
   `EngineContent/` default. First hit wins, and a miss at the last level is an error, never a silent
   C++ fallback (the `ShowcaseGalleryScene` habit this plan kills).
3. **Containment is a forest.** An Instance may embed or link Instances at most one level deep, and an
   embedded payload may never reference its parent. That is what makes the load order a single
   topological pass of `Derivation` instead of a fix-point loop.
4. **Sections are the invalidation grain.** Editing `ApertureBlades` inside `.Lens` rebakes glare only;
   editing `.Curve` consumers rebakes nothing but their look-ups. Fine-grained dirtiness survives
   embedding because section identities are preserved, not flattened.

**Second example — a particle effect, which is where domains meet quantities.**

```toml
# Content/Effects/DustPlume.Instance  (DocumentRole = 'Effect')
[['.EmbeddedSpace']] FormatTag = '.ParticleSpace' EmbedMode = 'Embedded'
  [ParticleMaster] LifetimeSpanSeconds = [0.4, 2.6]  EnsembleCapacity = 65536
  [ParticleColour] FormatTag = '.ColourRamp'  RampCurveCount = 3
  [ParticleSize]   FormatTag = '.Curve'
[['.EmbeddedSpace']] FormatTag = '.FieldSpace' EmbedMode = 'Embedded'
  [VelocityField] Units = 'm/s'  ExtentVoxels = [32, 16, 32]  WrapRule = 'Clamp'
  [ForceField]    Units = 'm/s^2'  Coupling = 'Acceleration'
[['.EmbeddedSpace']] FormatTag = '.BehaviourSpace' EmbedMode = 'Linked'
  DocumentPath = 'Content/Behaviour/WindResponse.BehaviourSpace'
```

`ParticleSpace` owns the ensembles and their attribute streams; the velocity it writes *out* and the
force it reads *in* are `FieldSpace` sections, referenced by identity, never copied. Colour is a
`.ColourRamp` section, because a ramp is a curve over a scalar — not a domain, so no `Space`.

---

## 4. The `Space` roster (this is the whole list)

| Domain | Holds | Codec | Engine unit |
| :--- | :--- | :--- | :--- |
| `.GeometrySpace` | PolygonNetwork, creased normals, `ClusterPyramid` LODs | `GeometryCodec` | `Engine/GeometrySpace` |
| `.MaterialSpace` | lobe response, coating order, 128 B compact register | `MaterialCodec` | `Engine/MaterialSpace` |
| `.TextureSpace` | sampled texel data addressed through surface parameters | `TextureCodec` | `Engine/TextureSpace` |
| `.PixelSpace` | editable raster canvas: layers, blend order, plates | `PixelCodec` | `Engine/PixelSpace` |
| `.VectorSpace` | 2-D contours in normalized `ViewBox` extents | `VectorCodec` | `Engine/VectorSpace` |
| `.ParticleSpace` | point ensembles with per-particle attribute streams | `ParticleCodec` | `Engine/ParticleSpace` |
| `.FieldSpace` | continuous volumetric quantities: force, velocity, density, temperature | `FieldCodec` | `Engine/FieldSpace` |
| `.PhysicsSpace` | bodies, joints, collision response, solver limits | `PhysicsCodec` | `Engine/PhysicsSpace` |
| `.AudioSpace` | sources, buses, room acoustics, playback scheduling | `AcousticCodec` | `Engine/AudioSpace` |
| `.BehaviourSpace` | authored decision logic and signal routing at runtime | `BehaviourCodec` | `Engine/BehaviourSpace` |

Ten domains, each with its own five alternatives in the ledger. **The strict variant you described**
— `Geometry` · `Particles` · `Physics` · `Audio` only — is decision **D1** in §15, and every row of the
ledger already carries the **bare variant** column for it (`.Material`, `.Texture`, `.PixelCanvas`,
`.VectorContour`, `.FieldVolume`, `.Behaviour`), so choosing the strict roster is a one-line registry
edit, not a rewrite. The four you named are core because *nothing contains them*; the six I added are
in the same position once an engine is general purpose (a texture is as irreducible as a mesh, and so
is a force field), which is why they qualify under Law 1 rather than by convenience.

---

## 5. Everything that is *not* a domain — the 81 bare names

Grouped by owner; **five or more alternatives for each** are in the
[ledger](../Naming/FormatCandidateLedger.md), so picking a different one is a registry edit.

| Group | Chosen names |
| :--- | :--- |
| Light & optics | `.Atmosphere` · `.Luminaire` · `.CandelaProfile` · `.RadianceLattice` · `.IrradianceRegister` · `.GlarePlate` · `.SpectralProfile` |
| Camera & imaging | `.CameraView` · `.Lens` · `.SensorResponse` · `.CameraRig` |
| Image & 2-D output | `.SpritePlate` · `.TexturePlate` · `.PatternStamp` · `.SvgExchange` |
| Surface detail | `.SurfaceParameter` · `.CoatingStack` · `.ScatterSamples` · `.NormalField` |
| Time & motion | `.Timeline` · `.Curve` · `.PoseSequence` · `.BlendRegister` · `.ShapeTargets` · `.TriggerZone` · `.TimerProfile` |
| Particles & fields | `.ParticleSource` · `.RibbonStrip` · `.FlockRules` · `.ForceField` · `.FluidVolume` · `.CloudDeck` · `.FogVolume` · `.NoiseNetwork` · `.ScatterRules` |
| Physics & destruction | `.ConstraintSet` · `.FractureNetwork` · `.ShardCluster` · `.ConvexHull` · `.ImpactProfile` · `.RagdollLinkage` · `.ClothPanel` · `.VehicleTuning` |
| Logic & behaviour | `.BehaviourNetwork` · `.SignalBus` · `.VariableRegister` · `.EligibilityTest` · `.SessionSnapshot` · `.NetworkPolicy` |
| Interface & type | `.Interface` · `.Tooltip` · `.FontBake` · `.VisualTheme` · `.VisualToken` |
| Shading & execution | `.RenderSequence` · `.ShaderNetwork` · `.ComputeProgram` · `.ShaderBinary` · `.ToneStack` · `.SamplerRegister` · `.ViewportConfiguration` |
| Colour & measurement | `.ColourDefinition` · `.ColourRamp` · `.GradingCube` |
| Derived spatial | `.RayStructure` · `.BoundingLattice` · `.DistanceField` · `.Clipset` |

Three things to notice, because they are where the earlier draft went wrong:

- **The `…Field` names are sections, the `…Space` name is the domain.** `.ForceField`,
  `.VelocityField`, `.PressureField`, `.DensityField`, `.TemperatureField` are section tags inside
  `.FieldSpace`. That settles **ForceField vs VectorField**: neither is the document. `VectorField`
  names the *mathematical type* of the array; `ForceField` names the *runtime role* of one quantity.
  Under your own rule — name the reality, not the category — the container is `FieldSpace` and each
  section is named for the physical quantity it stores, with explicit units. Choose the pair
  **`.FieldSpace` + `.ForceField`** and both complaints disappear at once.
- **Fluids are a field with more quantities.** `.FluidVolume` is one authored document whose sections
  *are* `VelocityField`, `PressureField`, `VorticityField`, `DivergenceField` on a shared lattice, so a
  fluid and a wind field and a debris vortex all stream through one codec. `flow` is barred, so no
  name in the registry says it.
- **`Set` survives, `Set` as a verb does not.** `.ConstraintSet`, `.JointSet`, `.IconSet` are legal
  because the bar in `SKILL-Naming.md` §4 is on **function verbs** (`Get`, `Set`). That distinction is
  now explicit in the vocabulary module (`FUNCTION_VERBS`), which the previous draft blurred by
  treating the verb list as a blanket noun ban.

---

## 6. Engine root documents — the updated list you asked for

```
Frontier/
├── Engine/Serialization/Roots/
│   ├── ProjectIndex            .ProjectIndex         ← replaces .ProjectSpace
│   ├── RuntimeConfiguration    .RuntimeConfiguration ← replaces .EngineConfiguration
│   ├── EngineSettings          .EngineSettings       ← replaces .GraphicsSettings
│   ├── PlatformSettings        .PlatformSettings     ← new, layered over EngineSettings
│   ├── UserPreferences         .UserPreferences      ← user directory, never committed
│   ├── DiagnosticsConfiguration                        ← replaces .DebugConfiguration
│   └── InterchangeConfiguration                        ← third-party import/export knobs
└── Engine/Editor/Layout/
    └── WorkspaceLayout         .WorkspaceLayout      ← replaces StudioLayout / EditorLayout
```

| Chosen | Five or more alternatives | Carries | Must not carry |
| :--- | :--- | :--- | :--- |
| **`.WorkspaceLayout`** | `.DockLayout` · `.PanelLayout` · `.WindowLayout` · `.AuthoringLayout` · `.PaneLayout` | panel splits, viewport gizmo scale, debug toggles, snap sizes | anything a shipped build reads |
| **`.EngineSettings`** | `.QualitySettings` · `.PreferenceRegister` · `.DisplaySettings` · `.SettingsPreset` · `.EnginePreferences` | resolution, V-Sync, FPS cap, shadow and texture quality presets, audio device, accessibility | boot-critical numbers |
| **`.RuntimeConfiguration`** | `.BootConfiguration` · `.ExecutionConfiguration` · `.StartupConfiguration` · `.RuntimeLimits` · `.EngineConfiguration` | tick rate, memory ceilings, validation layers, startup `.Instance` path, execution channel counts | human preferences |
| **`.PlatformSettings`** | `.TargetSettings` · `.PlatformOverrides` · `.HardwareTargetSettings` · `.PlatformProfile` · `.DeviceSettings` | per-target swapchain, resolution scale, input device class, packaging | duplicated shared values |
| **`.ProjectIndex`** | `.ProjectRegistry` · `.ContentIndex` · `.ProjectRepository` · `.ProjectLedger` · `.DocumentIndex` | every document path, startup links, dependency locks, schema floor | payload of any document |

The split matters more than the names: `Settings` is what a person edits, `Configuration` is what the
loader must not run without, `Index` is what points at everything else. Three documents, three
failure modes, three layers of precedence:
`PlatformSettings` → `EngineSettings` → `RuntimeConfiguration` defaults, with `WorkspaceLayout`
overlaid only when `FRONTIER_PROFILE_EDITOR=1`.

---

## 7. Camera and optics, decomposed (the `.LensAperture` fix)

| Concern | Format | Owner | Note |
| :--- | :--- | :--- | :--- |
| framing, gate aspect, projection, clip spans | **`.CameraView`** | `CameraViewCodec` | links a lens; owns no glass |
| focal travel, stops, blade count, distortion, dispersion, transmission, breathing, flare links | **`.Lens`** | `LensCodec` | the *whole* assembly — what you objected to being called by its one part |
| aperture blades and iris polygon | section `ApertureBlades` inside `.Lens` | `LensCodec` | a section, not a document |
| well capacity, gain, read noise, CFA, exposure response | **`.SensorResponse`** | `SensorResponseCodec` | split out of the old catch-all |
| ghosts, starbursts, veiling glare | **`.GlarePlate`** | `GlareCodec` | `bloom` stays barred |
| focus solve, circle of confusion | section `FocusSolver` inside `.CameraView` | `CameraViewCodec` | needs the view, so it lives there |

One optics folder per physical thing: `Engine/Components/Camera/` for view, lens and sensor;
`Engine/GeometricRaster/Atmosphere/` for glare and flare plates. No `Optics` grab-bag holding both.

---

## 8. Containment links — kinship and `peer` both gone

| Field | Chosen | Retired |
| :--- | :--- | :--- |
| enclosure | `EnclosingIndex` | `ParentIndex` |
| first contained | `FirstNestedIndex` | `FirstChildIndex` |
| next in the same set | `FollowingIndex` | `NextPeerIndex` |
| previous in the same set | `PrecedingIndex` | `PreviousPeerIndex` |
| nesting level | `DepthLevel` | `ParentDepth` |

`NextPeerIndex` was itself a violation of your directive, and it was written into the *approved*
replacements table of `BannedWordsVocabulary.md` §2. `Following`/`Preceding` state the memory relation
without a social metaphor, and every alternative for each is in the ledger.

---

## 9. Blacklist corrections this audit forced (governance changes)

Running the blacklist literally against the vocabulary the repository itself approves produced three
hard contradictions. They need a ruling in `BannedWordsVocabulary.md`, which §14 of this plan encodes
in `Tools/FormatNamingVocabulary.py`:

| Conflict | Why it is a contradiction | Ruling adopted |
| :--- | :--- |:--- |
| `graph` barred, yet `TypographyRepository` and `VectorGraphic` are approved | substring matching forbids the approved units, so the bar cannot mean substring | `graph` barred **as a token** and as a **named compound**: `SceneGraph`, `ShaderGraph`, `RenderGraph`, `NoiseGraph`, `VectorGraphics`, `DependencyGraph`. Typography survives; the offending compounds do not |
| `facet` barred, yet `.Interface` is a core Frontier noun | `Interface` contains `facet` | `facet` barred **as a token** (it means a polyhedron face, and `Face`/`Faces` are the substitutes) |
| `action` barred, yet `Reaction`, `Attraction`, `Transaction` are physics vocabulary | substring matching forbids them | `action` barred **as a token** — it means the gameplay verb (`DoAction`), not chemistry |
| `dev` barred, yet `Device`, `Development` are load-bearing | substring matching forbids the build profiles | `dev` barred **as a token** |
| `cache` barred and `ClipmapRepository` approved | `Clipmap` ends in `map`, `Repository` was the approved `cache` substitute | **compound exemption list**: `clipmap`, `descriptor`, `RecordEntry`, `RecordToken` are sanctioned; `Lightmap`, `Shadowmap`, `Bumpmap`, `NormalMap`, `ColourMap`, `EnvironmentMap` stay **barred** |
| `script` barred per your directive, and `Descriptor` contains `script` | the bar would forbid `DescriptorSlotRegister`, which the style guide approves | `script` barred as substring (so no `ScriptSpace`, no `ScriptAsset`); `descriptor` exempt **in C++ only** and avoided in every format name — hence **`BakeSpecification`** |
| `record` barred bare, `RecordEntry` approved | kept | unchanged, encoded as an exemption on the compound |

Say the word and I will open this as a patch to `BannedWordsVocabulary.md` §1–2 with the mode column.

---

## 10. Derivation — the bake pipeline without `Dag`, `Recipe` or `Provenance`

```
Engine/Serialization/
├── Container/       InstanceCodec · SuffixRegistry · AtomicFileWriter · ContainerIntegrityValidator
│                    RevisionSectionCodec (embedded .Revision parametric stream)
├── Derivation/      DerivationLedger        ← acyclic bake order, topological sort, cycle-path report
│                    BakeSpecification       ← inputs + parameters + compiler versions + units + colour
│                    ContentHashCalculator   ← xxHash64 dirty scan, SHA-256 build key, FNV-1a 64 sections
│                    BakeScheduler           ← dirty-only parallel dispatch, dedup, cancellation
│                    DerivationReporter       ← why one output was reused or rebuilt
│                    CompiledDiskRepository  ← Build/CompiledRepository/<Category>/<Hash>.CompiledBinary
├── Codecs/          15 groups, 91 formats, one codec each (see ledger)
└── Migration/       LegacySuffixMigrator (read-only legacy decode) · RetiredAliasResolver
```

The structure is still a directed acyclic graph in the mathematical sense — `DerivationLedger`
verifies acyclicity and reports the exact cycle path — but nothing on disk or in an identifier says
`Dag`, `Graph`, `Recipe` or `Provenance`. Five alternatives for each of those four names are in the
ledger's *Build & Derivation* group (`Derivation` · `BuildLattice` · `DerivationLattice` ·
`CompileOrder` · `DependencyLattice` · `BakeOrder`), so the slot is decidable without reopening the
vocabulary.

Kept from the earlier draft verbatim, because they were already clean: the two-tier TOML + 64 B
container, `AtomicFileWriter` publish-only-if-current, `.Revision` embedding with its 215 MiB /
750 MiB caps, git-ignored `Build/CompiledRepository/`, reachability-aware pruning that never deletes
authored documents, and device-keyed (Vulkan driver UUID) rebuild of `.RayStructure` and
`.ShaderBinary`.

---

## 11. Complete rename ledger — every suffix from the 56-suffix draft

| Draft name | New name | Why |
| :--- | :--- | :--- |
| `.ProjectSpace` | `.ProjectIndex` | an index, not a domain |
| `.EngineConfiguration` | `.RuntimeConfiguration` | `EngineConfiguration` also collides with the `config` abbreviation bar in spirit; the new name says *when* it is read |
| `.GraphicsSettings` | `.EngineSettings` | `Graphics` reads as `graph` + `ics`; the settings are broader than graphics anyway |
| `.UserProfile` | `.UserPreferences` | keeps preferences out of the profile/record ambiguity |
| `.WorldSpace` | `.Instance` (`DocumentRole = 'World'`) | a world is a composite, not a domain |
| `.InstanceSpace` / `.DocumentSpace` | `.Instance` | a container must not share its contents' suffix |
| `.ComponentSpace` | `.Component` | already clean once `Space` is reserved |
| `.RenderSpace` | `.ViewportConfiguration` | it was authored viewport settings |
| `.EditorLayout` / `StudioLayout` | `.WorkspaceLayout` | `studio` barred per your directive; the object is a workspace |
| `.EditorConfiguration` | split: `.WorkspaceLayout` + `.RuntimeConfiguration` | it mixed editor toggles with boot parameters |
| `.PixelSheet` | split: `.PixelSpace` + `.SpritePlate` | domain versus packed page |
| `.ImageSpace` | `.PixelSpace` | your correction |
| `.VolumetricSpace` | `.FieldSpace` (+ `.Atmosphere`, `.CloudDeck`, `.FogVolume`) | one grab-bag became a domain plus three documents |
| `.PhotometrySpace` | `.Luminaire` + `.CandelaProfile` | authored source versus measured distribution |
| `.SkyAtmosphere` | `.Atmosphere` | your correction |
| `.LensAperture` | `.Lens` (+ `.SensorResponse`, `.CameraView`) | the whole assembly, split by concern |
| `.FractureTree` | `.FractureNetwork` | your correction; `tree` now barred |
| `.VectorField` | `.FieldSpace` + section `.ForceField` / `.VelocityField` | container versus quantity |
| `.FluidGrid` | `.FluidVolume` | `grid` implies one solver layout; the volume is the data |
| `.ParticleSequence` | `.ParticleSpace` | it is a domain, and `Sequence` means execution order here |
| `.SpawnSource` | `.ParticleSource` | `spawn` barred as a verb |
| `.RibbonTrail` | `.RibbonStrip` | geometry named for what it builds |
| `.SwarmBehavior` | `.FlockRules` | rules, not behaviour-by-name; `adjacency` avoided via `NeighbourRules` |
| `.ScriptingSpace` (your example) | `.BehaviourSpace` + `.BehaviourNetwork` | `script` barred, `hook` barred |
| `.PostStack` | `.ToneStack` | matches the approved `RadiometricToneStack` |
| `.RadianceClipmap` | `.RadianceLattice` | `clipmap` survives only in C++ (`ClipmapRepository`) |
| `.Irradiance` | `.IrradianceRegister` | the register is the layout |
| `.SpectralCurve` | `.SpectralProfile` | sampled over wavelength, so a profile |
| `.IesProfile` | `.CandelaProfile` | measured quantity, not a vendor acronym |
| `.UVSpace` / `.SurfaceParameterSpace` | `.SurfaceParameter` | geometry's sub-format |
| `.ColourSpace` | `.ColourDefinition` (folder `Engine/Colour/`) | primaries and transfer curves are a definition |
| `.FontSpace` | `.FontBake` | derived data; `glyph` barred so contours are named directly |
| `.CurveSpace` | `.Curve` | shared parameter type |
| `.VectorGraphics` | `.VectorSpace` | `graph` compound ban |
| `.ShaderGraph` / `.RenderGraph` / `.NoiseGraph` | `.ShaderNetwork` · `.RenderSequence` · `.NoiseNetwork` | same reason |
| `.DependencyDAG` | `Engine/Serialization/Derivation/` | your directive |
| `BakeRecipeScheduler` | `BakeScheduler` | `recipe` barred |
| `ProvenanceInspector` | `DerivationReporter` | `provenance` barred |
| `NextPeerIndex` | `FollowingIndex` | `peer` barred |
| `.EventBus` | `.SignalBus` | `bus` kept, `event` replaced by the concrete payload |
| `.VariableTable` | `.VariableRegister` | `table` barred |
| `.BlendTree` | `.BlendRegister` | `tree` barred |
| `.SamplerState` | `.SamplerRegister` | `state` barred |
| `.BoundingVolumeTree` | `.BoundingLattice` | `tree` barred |
| `.NormalMap` / `.BumpMap` | `.NormalField` | `map` barred in every form |
| `.Bloom` | `.GlarePlate` | `bloom` barred |
| `.Emitter` / `.EmitterNode` | `.ParticleSource` | `emitter`, `node` barred |
| `.ComputeKernel` | `.ComputeProgram` | `kernel` barred |
| `.Mesh…` | `PolygonNetwork` | `mesh` barred |
| `.Atlas` / `.Palette` / `.Strata` | `.SpritePlate` / `.ColourRamp` / `.CoatingStack` | discouraged or barred |
| `.NetworkSpace` | `.NetworkPolicy` | `topology`/`graph` avoided; the file is a policy |
| `.Acoustic` | `.AudioSpace` | domain, matching the existing `Engine/AudioSpace/` |
| `.TriggerZone` · `.TimerProfile` · `.Timeline` · `.Curve` · `.Tooltip` · `.Interface` · `.ConstraintSet` · `.ShardCluster` · `.ConvexHull` · `.ImpactProfile` · `.NoiseNetwork` · `.RayStructure` · `.DistanceField` · `.TextureSpace` · `.MaterialSpace` · `.GeometrySpace` · `.VariableRegister` · `.ComputeProgram` · `.RenderSequence` · `.ShaderNetwork` · `.Component` · `.Soundscape` | **unchanged** | already compliant and already precise — renaming them would be churn for its own sake |

---

## 12. Folder structure, top down

```
Frontier/
├── Engine/
│   ├── Serialization/{Container,Derivation,Migration}/            # §10
│   ├── Interchange/                         # strictly third-party: .ttf .otf .obj .gltf .step
│   │                                        #   .ies .exr .png .hdr .wav .ogg .svg MaterialX
│   ├── GeometrySpace/  MaterialSpace/  TextureSpace/  PixelSpace/  VectorSpace/
│   ├── ParticleSpace/  FieldSpace/     PhysicsSpace/  AudioSpace/  BehaviourSpace/
│   ├── Colour/  DistanceField/  SpatialAcceleration/  Animation/  Shaders/
│   ├── GeometricRaster/{Atmosphere,DirectLighting,GlobalIllumination,Decals}/
│   ├── Components/{Camera,Controls,Appearance,ContentBrowser,Docking}/
│   ├── Editor/Layout/                       # editor-only, CMake-excluded elsewhere
│   └── Lifecycle/                            # Initialize -> Tick -> Finalize
├── Content/
│   ├── Instances/{Worlds,Vehicles,Effects,Props,Characters}/       # *.Instance
│   ├── Geometry/  Materials/  Textures/  Pixels/  Vectors/         # *.GeometrySpace etc.
│   ├── SurfaceParameters/  Physics/  Particles/  Fields/  Audio/
│   ├── Behaviour/  Luminaires/  Candela/  Atmosphere/  Optics/
│   ├── Typography/  Interfaces/  Projects/
│   └── ProjectIndex
├── EngineContent/                            # shipped defaults, same suffixes
├── Exhibits/                                 # *Validation targets and deterministic .png output
└── Tools/FormatTaxonomy.py · NamingAudit.py · FormatNamingVocabulary.py
```

`Engine/` mirrors `Content/` **one domain per folder, no wrapper levels**: `Engine/SimulationSpace/
PhysicsSpace/ConstraintSolver/` collapses to `Engine/PhysicsSpace/ConstraintSolver/`, because the
wrapper `Space` was naming a container that does not exist. `Content/Environment/` splits into
`Atmosphere/`, `Clouds/`, `Fog/`, `CelestialBodies/` — `Environment` was a category word, and the
bar's own rule is to name the reality.

---

## 13. Enforcement, not intention

```bash
python3 Tools/FormatTaxonomy.py                       # regenerate registry + ledger
python3 Tools/NamingAudit.py                          # C1-C6 over 637 names: 0 failures
python3 Tools/NamingAudit.py --check-tree Engine Content --strict   # CI gate over real files
```

`--check-tree` is the piece that keeps this honest after the rename: it audits every file and folder
stem in a checkout against the same law, so a `SceneGraph.cpp` cannot survive a review. Add it to
`.github/workflows/frontier-ci.yml` next to the existing formatting steps, and give
`SuffixRegistry` a test that fails if a registered suffix has no owning codec (check **C4** already
does this for the registry, and once `Engine/Serialization/` exists it can load the real one).

---

## 14. Sequence

| Phase | Deliverable | Exit gate |
| :---: | :--- | :--- |
| 0 | lock **D1–D6** (§15); land this plan + generated registry + audit in CI | `NamingAudit.py --strict` green on the repo as it stands |
| 1 | `Container/`: `InstanceCodec`, `SuffixRegistry` with retired-alias table, `AtomicFileWriter`, `ContainerIntegrityValidator` | round-trip, case-insensitive alias decode, overlap/truncation and FNV-1a 64 corruption checks pass |
| 2 | `Derivation/`: ledger, `BakeSpecification`, hashes, `BakeScheduler`, `DerivationReporter`; first vertical bake = `.FontBake` from `.ttf` and `.Atmosphere` LUTs | 0 ms rebake on cold restart after an unrelated edit; cycle detected and reported; crash-safe publish |
| 3 | `Engine/Serialization/Codecs/` for the ten domains; `Content/` re-generated with `.Instance` + domain files | an `Instance` loads from embedded, linked and inherited payload identically; missing last-level default is an error, no C++ fallback |
| 4 | root documents (`ProjectIndex`, `RuntimeConfiguration`, `EngineSettings`, `PlatformSettings`, `UserPreferences`, `WorkspaceLayout`) wired into boot; hardcoded showcase scenes deleted | startup reads only documents; `Editor` toggles excluded from `Development`/`Shipping` |
| 5 | non-domain formats in the §5 order above; `LegacySuffixMigrator` read-only decode; folder collapses (`Colour/`, `DistanceField/`, `PhysicsSpace/`) | `--check-tree` green over the whole tree; every legacy file on disk still opens |
| 6 | `BannedWordsVocabulary.md` patch with the mode column and the §9 rulings | the document and the tool agree; a deliberately bad name fails CI |

---

## 15. Decisions I need from you

| ID | Question | Recommendation | If you pick otherwise |
| :---: | :--- | :--- | :--- |
| **D1** | `Space` roster: ten embeddable domains, or the strict four (`Geometry` · `Particles` · `Physics` · `Audio`)? | **ten** — a texture, a material and a field are as irreducible as a mesh in a general engine | flip one registry constant; the ledger already lists the bare variant per row |
| **D2** | Field container: **`.FieldSpace`** with quantity sections (`.ForceField`, `.VelocityField`, …) | **this** | `.ForceField` as a document name — narrower, and you lose density and temperature |
| **D3** | Logic domain: `.BehaviourSpace` + `.BehaviourNetwork` | **this** (British spelling matches `Colour`) | `.LogicSpace` + `.LogicCircuit`, or `.RuleSpace` + `.RuleNetwork` — all five alternatives are vetted |
| **D4** | Composite extension: bare **`.Instance`** everywhere with `DocumentRole`, or per-domain `.WorldInstance` / `.EffectInstance`? | **bare `.Instance`** | add a `DocumentRole`-derived suffix; the alias table already supports both |
| **D5** | Raster domain: **`.PixelSpace`** (+ `.SpritePlate` for packed pages) | **this** | `.RasterSpace` / `.CanvasSpace` / `.LayerSpace` are alternatives 2–4 |
| **D6** | Adopt the §9 blacklist rulings into `BannedWordsVocabulary.md`? | **yes**, with the mode column | the tool keeps its stricter reading and `.Interface` and `TypographyRepository` would have to be renamed too |

Nothing in this plan blocks Phase 0: whatever you answer, the ledger already contains a legal name for
that slot, and `NamingAudit.py` will prove it before a single header is written.
