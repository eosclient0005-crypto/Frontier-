# Frontier — Project Zero fern UI study

The browser editor on this branch began from Project Zero Editor in [`eosclient0001-rgb/Frontier`, branch `arena/d929ef36-frontier`](https://github.com/eosclient0001-rgb/Frontier/tree/arena%2Fd929ef36-frontier), revision `f7bfbf7d5894f87b161f656c98e076f9ead55681`.

The changes between that revision and upstream revision `d3ae36aa7d0ef189fdac08c4ee86364fe15d9d35` have been applied to this partial checkout, including the Construct import workflow and Project Browser report. Conflicting editor changes were combined and the standalone HTML was rebuilt. This is a source-level integration, **not a Git history merge** of the full upstream repository.

The fern mesher and its supporting files in `Experimental/BotanicalDemo` come from [`egoistm1inbox/Frontier`, branch `arena/2c12bbb8-frontier`](https://github.com/egoistm1inbox/Frontier/tree/arena/2c12bbb8-frontier), revision `41bfeddf82949dd8e4c3bd96dcde5f16d1f97373`.

## Native source integration and inspector work

The native C++ source and build support from `egoistinbox-hue/Frontier` branch
`arena/f490d972-frontier`, revision `27254c0dbde0302f535261cb8fb4a0ab5d182f14`,
were selectively merged into this branch. The incoming `Experimental/` was **not** merged.
The merge imports `Engine/`, build automation and configuration, native projects and C++
validation sources. It deliberately excludes the upstream runtime assets in `Content/` and
`EngineContent/` and the upstream non-C++ exhibits, so this is not a complete runtime checkout.
Those omitted runtime assets are needed for the editor's fonts, icons and content; if wanted locally,
they can be extracted from the merged parent without touching `Experimental/`:

```sh
git archive 27254c0dbde0302f535261cb8fb4a0ab5d182f14 Content EngineContent | tar -x
python3 Automation/PrepareDependencies.py
python3 Automation/PrepareShaderToolchain.py
python3 Automation/PrepareFonts.py
cmake -S . -B Build/Editor -G Ninja -DFRONTIER_PROFILE=Editor
cmake --build Build/Editor --target Frontier
```

The native geometry inspector's common card surfaces and main text now use the active control
palette. Its retained inspector cache includes those colors so theme transitions trigger a redraw.
`GeometryInspectorThemeValidation` checks the actual card drawing color for OLED, Dim and Light.
The native `Frontier` target builds in this environment using GLFW without X11 or Wayland support;
that build is not a usable desktop preview. The broader `StandaloneInspectorsValidation` builds
after updating a stale camera-setting field, but currently reports 11 failures in unrelated
atmosphere, starfield, lighting and camera preview-upload expectations.
A software-rasterized native torus overview is at `Exhibits/Images/AnalyticInspector/native-torus-overview.png`.
Reference captures of the browser cube inspector are in `Exhibits/ProjectZeroEditor/AnalyticReference`.

This is **not yet a 1:1 native port**: the native inspector still has eight pages, not the
browser's nine; card behavior, geometry data binding, page-turn animation, icons and several
hard-coded colors remain to be matched. A native offscreen visual comparison is still required.
The native desktop application needs a desktop/window system and Vulkan; the HTML server below
is a separate reference, not the native editor.

## Try the fern

```sh
python3 Experimental/ProjectZeroEditor/Serve.py 8092
```

Open **Construct → Plants → Fern**, then **Add to scene**. Select the fern in the Outliner. Its inspector uses the geometry-style page strip with an **Overview**, **Geometry** transform and **Generator** page. The generator exposes the demo's Shape, Detail and Surface parameters, including fern architecture, detail tier, fronds, pinnules, lamina, crosiers, sori and pigments. Changes regenerate the browser mesh; high detail tiers can consume substantial time and memory. The Construct and Outliner icon is an offline raster capture of the generated default mesh, not a hand-drawn symbol.

The Material page edits the generator's pigments; the full geometry material stack, Components, UV, Fracture, SDF, LOD and live icon capture are **not yet connected** to the fern. This study does not create a native C++ object or port the demo's shaders, wind simulation, substrate, or individual organ selection. It uses the generator's vertex colours with a three.js material, and starts at the realtime detail tier.

## Rebuild and check

```sh
cd Experimental/ProjectZeroEditor
npm ci
npm run build
npm run test:fern
npm run capture:fern # optional: rebuild the mesh icon, then run npm run build
```

The standalone `index.html` is rebuilt from the checked-in sources. The DOM integration test simulates the browser without WebGL; the 3D renderer still needs visual testing in a real browser. Editor state is stored in browser local storage; JSON export/import is available in its File menu.
