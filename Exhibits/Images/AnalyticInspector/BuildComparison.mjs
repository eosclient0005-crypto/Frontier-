// Assemble unmodified browser and native captures into a labelled comparison.
// Run after npm ci in Experimental/ProjectZeroEditor: node Exhibits/Images/AnalyticInspector/BuildComparison.mjs
import { createRequire } from 'node:module';
import { fileURLToPath } from 'node:url';
import { dirname, resolve } from 'node:path';
import { writeFile } from 'node:fs/promises';
const here = dirname(fileURLToPath(import.meta.url));
const require = createRequire(resolve(here, '../../../Experimental/ProjectZeroEditor/package.json'));
const { createCanvas, loadImage } = require('@napi-rs/canvas');
const browser = await loadImage(resolve(here, '../../ProjectZeroEditor/AnalyticReference/cube-01-overview.png'));
const native = await loadImage(resolve(here, 'native-cube-overview.png'));
const canvas = createCanvas(752, 955), ctx = canvas.getContext('2d');
ctx.fillStyle = '#191b20'; ctx.fillRect(0, 0, canvas.width, canvas.height);
ctx.fillStyle = '#f1f2f5'; ctx.font = 'bold 20px sans-serif';
ctx.fillText('Cube inspector · overview comparison', 24, 30);
ctx.font = '13px sans-serif'; ctx.fillStyle = '#bac5d4';
ctx.fillText('Browser reference · Project Zero', 24, 58);
ctx.fillText('Native C++ · software raster', 388, 58);
ctx.drawImage(browser, 24, 70); ctx.drawImage(native, 388, 70);
ctx.fillStyle = '#e5b96e'; ctx.font = '13px sans-serif';
ctx.fillText('Comparison only: native parity and page-turn animation are not yet complete.', 24, 925);
await writeFile(resolve(here, 'cube-overview-comparison.png'), await canvas.encode('png'));
