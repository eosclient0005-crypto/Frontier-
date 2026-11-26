// Browser-only adapter for the botanical demo's procedural fern mesher.
import { FernMesher } from "../BotanicalDemo/src/generators/fernMesher.ts";
import { FernParameters } from "./FernSpecification.js";
import { THREE } from "../Shared/Studio/Render.js";
const NumberIn = (Value, Default, Min, Max) => {
  const N = Number(Value);
  return Value === undefined || !Number.isFinite(N) ? Default : Math.max(Min, Math.min(Max, N));
};
export function FernGeometry(Values = {}) {
  const Params = FernParameters(Values);
  const Seed = Math.round(NumberIn(Values.FernSeed, 42, 1, 999999));
  const Result = new FernMesher(Params, Seed).build("fern");
  const Plant = new THREE.Group();
  for (const Part of Result.parts) {
    if (!Part.vertexCount || !(Part.quads.length || Part.tris.length)) continue;
    const Indices = [...Part.tris];
    for (let I = 0; I < Part.quads.length; I += 4) {
      const [A, B, C, D] = Part.quads.slice(I, I + 4);
      Indices.push(A, B, C, A, C, D);
    }
    const Geometry = new THREE.BufferGeometry();
    Geometry.setAttribute("position", new THREE.Float32BufferAttribute(Part.positions, 3));
    Geometry.setAttribute("normal", new THREE.BufferAttribute(Part.computeNormals(), 3));
    Geometry.setAttribute("color", new THREE.Float32BufferAttribute(Part.colors, 3));
    Geometry.setIndex(Indices);
    const Mesh = new THREE.Mesh(Geometry, new THREE.MeshStandardMaterial({ vertexColors: true, roughness: 0.72, side: THREE.DoubleSide }));
    Mesh.castShadow = Mesh.receiveShadow = true;
    Plant.add(Mesh);
  }
  Plant.userData.fern = true;
  Plant.userData.stats = Result.stats;
  return Plant;
}
