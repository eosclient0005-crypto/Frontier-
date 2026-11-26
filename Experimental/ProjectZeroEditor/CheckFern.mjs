import assert from "node:assert/strict";
import { build } from "esbuild";
import { pathToFileURL } from "node:url";
import { mkdtemp, writeFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import path from "node:path";

const dir = await mkdtemp(path.join(tmpdir(), "frontier-fern-"));
try {
  const result = await build({
    stdin: { contents: `import React from 'react'; import { renderToStaticMarkup } from 'react-dom/server';
      import { FernGeometry } from './FernGeometry.js'; import { FernParameters } from './FernSpecification.js'; import FernPanel, { FernRow } from './FernPanel.jsx';
      export { FernGeometry, FernParameters, FernRow }; export const renderPanel = values => renderToStaticMarkup(React.createElement(FernPanel, { Subject: FernRow, Values: values, Change: () => {}, Hidden: false, ToggleHidden: () => {} }));`,
      resolveDir: process.cwd(), loader: "jsx" },
    bundle: true, platform: "node", format: "cjs", write: false, define: { "process.env.NODE_ENV": '"production"' }, loader: { ".css": "empty" },
  });
  const bundle = path.join(dir, "test.cjs");
  await writeFile(bundle, result.outputFiles[0].contents);
  const { FernGeometry, FernParameters, FernRow, renderPanel } = await import(pathToFileURL(bundle));
  assert.equal(FernRow.Panel, "fern");
  const panel = renderPanel({ FernPreset: "lady", FernFronds: 12 });
  assert.match(panel, /2-Pinnate Lacy/);
  assert.match(panel, /Fronds/);
  assert.match(panel, /Overview/);
  const params = FernParameters({ FernPreset: "lady", FernFronds: 12, FernParameters: { frondCount: 7, stipeRadius: 0.006, detailTier: 2, hueBase: "#428831" } });
  assert.equal(params.architecture, "bipinnate_lacy");
  assert.equal(params.frondCount, 7);
  assert.equal(params.stipeRadius, .006);
  assert.equal(params.detailTier, 2);
  assert.equal(params.hueBase, "#428831");
  assert.equal(FernParameters({ FernFronds: 999 }).frondCount, 32);
  let fingerprint;
  for (const variant of ["sword", "lady", "ostrich"]) {
    const plant = FernGeometry({ FernPreset: variant, FernFronds: 8, FernSeed: 42 });
    assert.ok(plant.children.length > 0, `${variant} has meshes`);
    let triangles = 0;
    for (const mesh of plant.children) {
      const positions = mesh.geometry.attributes.position;
      const normals = mesh.geometry.attributes.normal;
      const colours = mesh.geometry.attributes.color;
      assert.equal(positions.count, normals.count);
      assert.equal(positions.count, colours.count);
      assert.ok(mesh.geometry.index.count > 0 && mesh.geometry.index.count % 3 === 0);
      assert.ok(positions.array.every(Number.isFinite));
      assert.ok(normals.array.every(Number.isFinite));
      assert.ok(mesh.geometry.index.array.every(index => index < positions.count));
      triangles += mesh.geometry.index.count / 3;
    }
    if (variant === "sword") fingerprint = plant.children.map(mesh => Array.from(mesh.geometry.attributes.position.array.slice(0, 60)));
    console.log(`${variant}: ${plant.children.length} meshes, ${triangles} triangles`);
  }
  const same = FernGeometry({ FernPreset: "sword", FernFronds: 8, FernSeed: 42 });
  assert.deepEqual(same.children.map(mesh => Array.from(mesh.geometry.attributes.position.array.slice(0, 60))), fingerprint);
  const fewer = FernGeometry({ FernParameters: { frondCount: 4 } });
  const more = FernGeometry({ FernParameters: { frondCount: 8 } });
  assert.ok(more.children.reduce((n, mesh) => n + mesh.geometry.index.count, 0) > fewer.children.reduce((n, mesh) => n + mesh.geometry.index.count, 0));
  const changed = FernGeometry({ FernParameters: { frondCount: 8, hueBase: "#dd2233" } });
  assert.ok(changed.children.some((mesh, i) => mesh.geometry.attributes.color.array.some((value, j) => value !== more.children[i].geometry.attributes.color.array[j])));
  console.log("Fern geometry, parameter mapping and inspector checks passed.");
} finally { await rm(dir, { recursive: true, force: true }); }
