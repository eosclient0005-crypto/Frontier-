# Analytic inspector evidence

## Visual comparison

- `cube-overview-comparison.png` places the browser reference and native C++ overview capture side by side. It is **evidence of the current gap**, not proof of visual parity. The reference has nine pages; the native inspector has eight. Spacing, icons, typography, controls, and animation still differ.
- `native-cube-overview.png` and `native-torus-overview.png` are unmodified native C++ inspector captures produced by `StandaloneInspectorsValidation`, rendered with the validation's software ImGui rasterizer. They are not screenshots of a running Vulkan desktop application. The test fixture places the objects in a folder named Hero Props; the browser reference uses the Geometry breadcrumb.
- The unmodified browser captures are in [`../../ProjectZeroEditor/AnalyticReference/`](../../ProjectZeroEditor/AnalyticReference/), one for each of the nine cube inspector pages. They were captured from the running Project Zero HTML UI with headless Chromium at a 1600 × 900 viewport, with the inspector cropped to its 340 × 825 scroll area.
- `BuildComparison.mjs` creates the labelled comparison from the original images. Run `npm ci` in `Experimental/ProjectZeroEditor`, then from the repository root run `node Exhibits/Images/AnalyticInspector/BuildComparison.mjs`.

## Native carousel phase

The shared native strip now wraps its previous/next buttons and uses the active control palette. It also handles more than 16 pages and ignores disabled pointer input. The cube and torus captures and comparison were regenerated after this change. `CarouselValidation.txt` records the focused checks. The strip is shared by other native inspectors, so these changes affect them too. Keyboard navigation, swipe, smooth rail scrolling and page-turn animation are not implemented by this phase.

## Validation results

- `GeometryInspectorThemeValidation.txt`: the focused native theme test passed. It verifies card and strip fills in ImGui draw vertices for OLED, Dim and Light, plus carousel pointer navigation, not full-screen visual parity.
- `StandaloneInspectorsValidation.txt`: the broader native validation returned **failure**, with 11 failed preview-upload expectations across atmosphere, starfield, lights and camera settings. The cube capture and shape classification passed. Do not interpret the final generic PASS line in the log as a passing suite; its process exit was 1.
- `StandaloneInspectorsValidation` requires the pinned typefaces prepared by `Automation/PrepareFonts.py`. With the native Editor build configured and fonts installed, run `cmake --build Build/Editor --target StandaloneInspectorsValidation`, then run `Build/Editor/StandaloneInspectorsValidation`. Its images are generated under `Exhibits/Images/StandaloneInspectors/`; copy `14-AnalyticCube-Overview.png` here to update the cube capture, then rebuild the comparison.

No native desktop, Vulkan output, or page-transition animation was verified by these images.
