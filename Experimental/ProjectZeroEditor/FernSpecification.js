import { DEFAULT_FERN_PARAMS, FERN_ARCH_NAMES, BROADLEAF_NAMES } from "../BotanicalDemo/src/generators/fernParams.ts";

export { FERN_ARCH_NAMES, BROADLEAF_NAMES };
export const FernPresets = {
  sword: { label: "Western sword fern", latin: "POLYSTICHUM MUNITUM", architecture: "pinnate_sword" },
  lady: { label: "Lady fern", latin: "ATHYRIUM FILIX-FEMINA", architecture: "bipinnate_lacy" },
  ostrich: { label: "Ostrich fern", latin: "MATTEUCCIA STRUTHIOPTERIS", architecture: "shuttlecock_ostrich" },
};
// The original demo controls and ranges; stipe radius is presented in mm but stored in metres.
export const FernGroups = {
  shape: [
    ["Rosette & Frond Stature", "macro", [
      ["Frond Count", "frondCount", 3, 32, 1], ["Frond Length", "frondLength", .2, 1.5, .02, "m"],
      ["Blade Width", "frondWidth", .06, .48, .005, "m"], ["Rosette Splay", "rosetteAngle", 10, 75, 1, "°"],
      ["Gravity Arch Droop", "archDroop", 5, 130, 1, "°"], ["Stipe Petiole Ratio", "stipeFraction", .06, .55, .01],
      ["Stipe Radius", "stipeRadius", 1.2, 11, .2, "mm", 1000],
    ]],
    ["Compound Pinnae & Pinnules", "division", [
      ["Pinna Pairs", "pinnaPairs", 6, 48, 1], ["Pinnule Sub-Pairs", "pinnulePairs", 4, 16, 1], ["Insertion Angle", "pinnaAngle", 35, 95, 1, "°"],
    ]],
    ["Rhizome Crown & Caudex", "", [
      ["Caudex Trunk Height", "caudexHeight", .01, 1.2, .01, "m"], ["Crown Radius", "crownRadius", .02, .18, .005, "m"], ["Ramenta Chaff Scales", "ramentaDensity", 0, 1, .05],
    ]],
  ],
  detail: [
    ["3D Leaf Venation & Lamina Relief", "ultra-mesh", [
      ["Lamina Thickness", "laminaThicknessMm", .2, 1.6, .05, "mm"], ["Midrib / Costa Relief", "midribRelief", 0, 1.5, .05], ["Lateral Vein Relief", "veinRelief", 0, 1.5, .05], ["Lateral Vein Pairs", "veinCount", 4, 26, 1], ["Bullate Quilting", "bullation", 0, 1.2, .04], ["Transverse V-Camber", "laminaCamber", -.5, .7, .02],
    ]],
    ["Margins, Auricles & Curl", "silhouette", [
      ["Serration / Lobe Depth", "serrationDepth", 0, .38, .01], ["Serration Teeth", "serrationCount", 4, 28, 1], ["Basal Sword Auricle", "auricle", 0, 1, .05], ["Margin Ruffle Wave", "edgeUndulation", 0, 1, .04], ["Apex Tip Droop Curl", "tipCurl", -.4, .8, .03],
    ]],
    ["Circinate Crosiers & Abaxial Sori", "spores", [
      ["Fiddlehead Crosiers", "crosierCount", 0, 9, 1], ["Crosier Emergent H", "crosierHeight", .06, .45, .01, "m"], ["Spiral Coil Turns", "crosierTurns", 1.2, 3.6, .1], ["Abaxial Sori Coverage", "soriCoverage", 0, 1, .05], ["Sori Dome Diameter", "soriSizeMm", .6, 2.6, .1, "mm"],
    ]],
  ],
  surface: [["Chlorophyll, Cuticle & Senescence", "SSS", [["Outer Frond Aging", "senescence", 0, .85, .02]]]],
};
const Limits = Object.fromEntries(Object.values(FernGroups).flatMap(Groups => Groups.flatMap(([, , Controls]) => Controls.map(([, Key, Min, Max, , , Factor = 1]) => [Key, [Min / Factor, Max / Factor]]))));
export function FernParameters(Values = {}) {
  const Previous = Values.FernParameters && typeof Values.FernParameters === "object" ? Values.FernParameters : {};
  const Legacy = {
    architecture: (FernPresets[Values.FernPreset] || FernPresets.sword).architecture,
    ...(Values.FernFronds !== undefined ? { frondCount: Values.FernFronds } : {}),
    ...(Values.FernLength !== undefined ? { frondLength: Values.FernLength } : {}),
    ...(Values.FernDroop !== undefined ? { archDroop: Values.FernDroop } : {}),
  };
  const Params = { ...DEFAULT_FERN_PARAMS, detailTier: 1, ...Legacy };
  for (const [Key, Default] of Object.entries(DEFAULT_FERN_PARAMS)) {
    if (!(Key in Previous)) continue;
    const Value = Previous[Key];
    if (typeof Default === "number" && Number.isFinite(+Value) && Value !== null && Value !== "") {
      const [Min, Max] = Limits[Key] || (Key === "detailTier" ? [1, 4] : [-Infinity, Infinity]);
      Params[Key] = Math.max(Min, Math.min(Max, +Value));
    } else if (Key === "architecture" && Object.hasOwn(FERN_ARCH_NAMES, Value)) Params[Key] = Value;
    else if (Key === "broadleafPlaced" && Object.hasOwn(BROADLEAF_NAMES, Value)) Params[Key] = Value;
    else if (Key.startsWith("hue") && /^#[0-9a-f]{6}$/i.test(Value)) Params[Key] = Value;
  }
  for (const [Key, [Min, Max]] of Object.entries(Limits)) {
    const Value = Number(Params[Key]);
    Params[Key] = Number.isFinite(Value) ? Math.max(Min, Math.min(Max, Value)) : DEFAULT_FERN_PARAMS[Key];
  }
  Params.detailTier = Math.round(Params.detailTier);
  return Params;
}
