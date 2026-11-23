"""Frontier naming vocabulary — the machine-readable law for format names.

`BannedWordsVocabulary.md` states which words are barred; this module states
which words are *required* in a given slot, and mechanically enforces both.

Consumers:
  Tools/FormatTaxonomy.py      -- generates the suffix registry + candidate tables
  Tools/NamingAudit.py         -- audits registry + real filenames/folders on disk
  Documentation/Naming/*       -- generated ledgers (never hand-edit those)

Match modes per banned word:
  "token"     -- barred only as a whole identifier token (camel/snake split)
  "substring" -- barred anywhere inside the identifier
  "tail"      -- barred as a whole token or as the tail of a token (map -> Lightmap)
"""

# ---------------------------------------------------------------------------
# Barred words. "mode" chooses token / substring / tail matching.
# ---------------------------------------------------------------------------

BLACKLIST = {
    # --- primary blacklist (BannedWordsVocabulary.md 1.1) ---
    "action": "token", "adjacency": "substring", "affinity": "substring",
    "and": "token", "archive": "substring", "artifact": "substring",
    "artwork": "substring", "assembly": "substring", "asset": "substring",
    "badge": "substring", "binding": "substring", "bloom": "substring",
    "bridge": "substring", "cache": "substring", "cadence": "substring",
    "card": "token", "carousel": "substring", "classification": "substring",
    "complex": "token", "concurrency": "substring", "concurrence": "substring",
    "concurrent": "substring", "context": "token", "continuum": "substring",
    "contract": "token", "coverage": "token", "dev": "token",
    "digest": "substring", "domain": "token", "draw": "token",
    "element": "token", "emblem": "substring", "emitter": "substring",
    "evaluator": "substring", "facet": "token", "fence": "token",
    "floating": "substring", "flow": "token", "frame": "token",
    "glyph": "substring", "graph": "token", "hook": "token",
    "hub": "token", "inscription": "substring", "interpolant": "substring",
    "item": "token", "listener": "substring", "manifest": "substring",
    "map": "tail", "member": "token", "menu": "token", "mesh": "substring",
    "mutation": "token", "nexus": "substring", "occupant": "substring",
    "palette": "substring", "pass": "token", "pill": "token",
    "pipeline": "substring", "pool": "token", "popups": "substring",
    "popup": "substring", "predicate": "substring", "probe": "substring",
    "reclamation": "substring", "reservoir": "substring", "stage": "token",
    "store": "token", "strata": "substring", "stratum": "substring",
    "submission": "substring", "table": "token", "topology": "substring",
    "vacancy": "substring", "remap": "token",
    # --- grandiose / centre-of-universe / mythology ---
    "apex": "substring", "sentinel": "substring", "vanguard": "substring",
    "oracle": "substring", "titan": "substring", "goliath": "substring",
    "maestro": "substring", "core": "token", "matrix": "substring",
    "catalyst": "substring", "conductor": "substring", "vortex": "substring",
    "aether": "substring", "zenith": "substring", "nova": "token",
    "genesis": "substring", "odyssey": "substring", "helios": "substring",
    "orion": "substring", "kronos": "substring",
    # --- generic OO / AI tropes ---
    "manager": "substring", "handler": "substring", "dispatcher": "substring",
    "processor": "substring", "controller": "substring",
    "coordinator": "substring", "service": "substring", "utility": "substring",
    "helper": "substring", "representation": "substring", "traversal": "token",
    "kernel": "substring", "record": "token", "node": "token",
    "module": "substring", "data": "token", "info": "token",
    "object": "token", "stuff": "token", "thing": "token",
    "common": "token", "base": "token", "entity": "token", "model": "token",
    "kind": "token", "role": "token", "design": "token", "system": "token",
    "subsystem": "substring", "scene": "token", "shell": "token",
    "widget": "substring", "popover": "substring", "actor": "token",
    "kit": "token", "tween": "substring", "hierarchy": "substring",
    "backend": "substring", "fronted": "substring",
    # --- job titles + kinship (whole category barred) ---
    "worker": "substring", "provider": "substring", "observer": "substring",
    "parent": "token", "child": "token", "children": "token",
    "sibling": "substring", "ancestor": "substring",
    "descendant": "substring", "orphan": "substring", "kin": "token",
    "family": "substring", "lineage": "substring",
    # --- barred verbs / boolean prefixes / void nouns ---
    "make": "token", "accept": "token", "commit": "substring",
    "compose": "substring", "composite": "substring", "harness": "substring",
    "trial": "token", "trials": "token", "emit": "token", "emission": "substring",
    "flag": "token", "flags": "token", "state": "token", "value": "token",
    "spawn": "token", "kill": "token", "watch": "token", "listen": "substring",
    "update": "token", "perform": "token",
    "designation": "substring", "extension": "token", "recordstore": "substring",
    # --- session directives from the user (2026-11-23) ---
    "dag": "substring", "recipe": "substring", "provenance": "substring",
    "studio": "substring", "peer": "substring", "script": "substring",
    "tree": "token",
}

# Barred only in these full compounds: `graph` as a substring would forbid the
# approved unit `TypographyRepository`, so the ban targets the node-graph names
# themselves plus the `-map` texture nouns.
BANNED_COMPOUNDS = (
    "vectorgraphics", "vectorgraphic", "scenegraph", "shadergraph", "rendergraph",
    "noisegraph", "dependencygraph", "scriptgraph", "blendgraph", "lightmap",
    "shadowmap", "bumpmap", "environmentmap", "colormap", "texturemap", "normalmap",
    "detailmap", "maskmap",
)

# Compounds the repository already sanctions, so the substring bar must not fire.
# `ClipmapRepository` is the approved replacement for `cache`; `RecordEntry` and
# `RecordToken` are approved while bare `Record` stays barred; `Descriptor` is
# sanctioned by `DescriptorSlotRegister`, so it is tolerated in C++ but never used
# as a format name where a plain noun exists.
EXEMPT_COMPOUNDS = {"clipmap", "descriptor", "recordentry", "recordtoken"}

# Barred for function identifiers only (SKILL-Naming.md 4). Format nouns are not
# judged by this list, which is why `.ConstraintSet` and `ColourRamp` stay legal.
FUNCTION_VERBS = ("get", "set", "make", "accept", "commit", "emit", "spawn", "kill",
                  "update", "perform", "watch", "listen", "handle", "manage", "compose")

# Abbreviation bar: `Config` only ever appears as `Configuration`.
ABBREVIATIONS = {"config": "Configuration", "cfg": "Configuration"}

# Not blacklisted but rejected when a direct mechanical name exists.
DISCOURAGED = {
    "atlas": "Plate / Page / Register",
    "slice": "Span / Range",
    "substrate": "Coating / Finish / Medium",
    "chart": "Tile / Plate / Section",
    "bundle": "Container / Instance",
    "buffer": "Explicit layout name",
    "cluster": "Lattice / Group",
    "channel": "Named quantity",
}

# ---------------------------------------------------------------------------
# Grammar law: which suffix class each slot must use.
# ---------------------------------------------------------------------------

# `Space` is reserved: a domain an Instance embeds and the runtime resolves
# whole. Nothing higher than these exists for that quantity of content.
SPACE_DOMAINS = {
    "GeometrySpace": "3-D form: PolygonNetwork, creased normals, ClusterPyramid",
    "MaterialSpace": "surface scattering response, lobes, coating order",
    "TextureSpace": "sampled texel data addressed through surface parameters",
    "PixelSpace": "editable raster canvas with layers and plates",
    "VectorSpace": "2-D contour art in normalized ViewBox extents",
    "ParticleSpace": "point ensembles with per-particle attribute streams",
    "FieldSpace": "continuous volumetric quantities: force, velocity, density",
    "PhysicsSpace": "bodies, joints, collision response, solver limits",
    "AudioSpace": "sound sources, buses, room acoustics, playback scheduling",
    "BehaviourSpace": "authored decision logic and signal routing at runtime",
}

# Suffix classes and what each one commits to. A format name must end in one
# of these (or be a bare domain noun for measured physical quantities).
SUFFIX_CLASSES = {
    "Space": "an embeddable domain a runtime resolves whole",
    "Codec": "encodes/decodes one format; never carries the container suffix",
    "Network": "authored connection structure with evaluable links",
    "Sequence": "ordered executable or playback steps",
    "Stack": "ordered layers applied bottom-up",
    "Register": "indexed lookup layout replacing table/map/array",
    "Profile": "measured or sampled distribution over a parameter axis",
    "Curve": "1-D function of a scalar input",
    "Volume": "bounded 3-D sampled quantity",
    "Lattice": "spatially indexed samples with wrap/clip rules",
    "Plate": "packed 2-D page of sub-images",
    "Sheet": "authored 2-D raster or vector page",
    "Set": "unordered collection of one type",
    "Layout": "spatial arrangement with no simulation meaning",
    "Configuration": "boot-critical machine parameters (never 'Config')",
    "Settings": "human-tunable preferences and quality presets",
    "Index": "root lookup document listing other documents",
    "Definition": "authored parameter record with no derived payload",
    "Rules": "declarative decision inputs for a solver",
    "Binary": "compiled opaque output",
}

# Words the naive naming instinct reaches for, already barred, and the
# concrete substitute to use instead. Keyed by the concept, not the code unit.
TRAPS = {
    "SceneGraph / scene graph": "EnclosingIndex + FirstNestedIndex containment ledger",
    "Dependency DAG": "Derivation (acyclic ledger, topological order)",
    "Bake recipe": "BakeDescriptor (inputs + parameters + versions)",
    "Provenance": "Origin / DerivationTrace / HitMissReporter",
    "Parent / Child / Sibling / Peer": "EnclosingIndex / FirstNestedIndex / FollowingIndex",
    "Texture atlas": "TexturePlate / PageRegister",
    "Lightmap / shadow map / bump map": "BakedLightVolume / ShadowCascadePlanes / NormalField",
    "Keyframe": "TimeKey / PoseKey",
    "Palette": "ColourRamp / SwatchRegister",
    "Shader graph": "ShaderNetwork",
    "Vector graphics": "VectorSpace / VectorContour",
    "Emitter": "ParticleSource / SourceVolume",
    "Audio FX graph": "FilterChain / AudioRouteRegister",
    "Material layer strata": "CoatingStack / FinishLayer",
    "Hook / callback": "SignalOutlet / EntrypointDeclaration",
    "Event bus": "SignalBus / BroadcastRegister",
    "Scripting hooks": "BehaviourSpace / BehaviourNetwork",
    "Cache": "CompiledRepository / LookupBuffer",
    "Store": "Repository",
    "Manifest": "DocumentHeader / TomlHeader / IndexHeader",
    "Asset": "Document / ContentEntry / ResourceFile",
    "Sprite sheet": "SpritePlate / IconPlate",
    "Mesh": "PolygonNetwork",
    "Node": "CircuitSlot / EntryIndex",
    "Blend tree": "BlendRegister / BlendNetwork",
    "Metadata": "DocumentHeader fields (explicit names only)",
    "Subsystem": "the domain unit name, bare",
}

CAMERA_BREAKDOWN = [
    ("Framing & projection", "`.CameraView`", "sensor fit, gate aspect, projection, clip spans, lens link"),
    ("Whole optical assembly", "`.Lens`", "focal travel, stops, blade count, distortion, dispersion, transmission, flare links, focus breathing"),
    ("Aperture & blades", "section `ApertureBlades` inside `.Lens`", "blade count, curvature, iris polygon, bokeh shape, diffraction spikes"),
    ("Sensor & exposure", "`.SensorResponse`", "well capacity, gain, read noise, colour filter array, exposure response curve"),
    ("Lens flare / ghosts", "`.GlarePlate`", "ghost and starburst plates, veiling glare thresholds, anamorphic squeeze"),
    ("Depth of field solve", "section `FocusSolver` inside `.CameraView`", "focus distance, f-stop, circle of confusion, CoC bounds"),
]

# ---------------------------------------------------------------------------
# Matching helpers
# ---------------------------------------------------------------------------


def SplitTokens(identifier):
    """Split PascalCase / camelCase / snake_case / kebab / dotted into tokens."""
    out, current = [], []
    for ch in identifier:
        if not ch.isalnum():
            if current:
                out.append("".join(current))
                current = []
            continue
        if ch.isupper() and current and (current[-1].islower() or len(current) > 1
                                        and current[-2].isupper() and current[-1].isupper()):
            out.append("".join(current))
            current = [ch]
        else:
            current.append(ch)
    if current:
        out.append("".join(current))
    return [t.lower() for t in out]


def FindViolations(identifier, extra_substring=()):
    """Return (hard, discouraged) violation lists for one identifier."""
    name = identifier.lower()
    tokens = SplitTokens(identifier)
    hard = []
    if any(t in EXEMPT_COMPOUNDS for t in tokens):
        name = "".join(t for t in tokens if t not in EXEMPT_COMPOUNDS)
        tokens = [t for t in tokens if t not in EXEMPT_COMPOUNDS]
    for word, mode in BLACKLIST.items():
        hit = False
        if mode == "substring":
            hit = word in name
        elif mode == "token":
            hit = word in tokens
        elif mode == "tail":
            hit = any(t == word or t.endswith(word) for t in tokens)
        if hit:
            hard.append(f"{word} ({mode})")
    for word in BANNED_COMPOUNDS:
        if word in name:
            hard.append(f"{word} (barred compound)")
    for word in extra_substring:
        if word.lower() in name:
            hard.append(f"{word} (session directive)")
    for token in tokens:
        for abbrev, full in ABBREVIATIONS.items():
            if token == abbrev or (token.endswith(abbrev) and len(token) > len(abbrev)):
                hard.append(f"{abbrev} (abbreviation; write {full})")
    discouraged = [f"{w} -> {fix}" for w, fix in DISCOURAGED.items() if w in name]
    return hard, discouraged
