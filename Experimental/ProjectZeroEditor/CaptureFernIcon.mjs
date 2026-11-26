// Rasterise the actual default fern mesh for the Construct and Outliner asset icon.
// This is an offline icon capture, not the browser viewport renderer.
import { build } from "esbuild";
import { createCanvas } from "@napi-rs/canvas";
import { writeFile, mkdtemp, rm } from "node:fs/promises";
import path from "node:path";
import os from "node:os";
import { pathToFileURL } from "node:url";
const tmp = await mkdtemp(path.join(os.tmpdir(), "frontier-fern-icon-"));
try {
  const output = await build({ entryPoints: ["FernGeometry.js"], bundle: true, format: "esm", platform: "node", write: false });
  const bundle = path.join(tmp, "fern.mjs");
  await writeFile(bundle, output.outputFiles[0].contents);
  const { FernGeometry } = await import(pathToFileURL(bundle));
  const plant = FernGeometry({});
  // Orthographic view from above and in front, using the mesh's per-vertex colours.
  const unit = (v) => { const n = Math.hypot(...v); return v.map(x => x / n); };
  const cross = (a, b) => [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]];
  const dot = (a, b) => a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
  const view = unit([2, 1.65, 2.8]), right = unit(cross([0, 1, 0], view)), up = unit(cross(view, right)), light = unit([-.35, .9, .6]);
  const triangles = []; let minX=Infinity,maxX=-Infinity,minY=Infinity,maxY=-Infinity;
  for (const mesh of plant.children) {
    const p=mesh.geometry.attributes.position.array, n=mesh.geometry.attributes.normal.array, c=mesh.geometry.attributes.color.array, idx=mesh.geometry.index.array;
    const projected = new Float64Array(p.length);
    for (let j=0;j<p.length;j+=3) {
      const v=[p[j],p[j+1],p[j+2]], x=dot(v,right), y=dot(v,up), z=dot(v,view);
      projected.set([x,y,z],j); minX=Math.min(minX,x);maxX=Math.max(maxX,x);minY=Math.min(minY,y);maxY=Math.max(maxY,y);
    }
    for(let k=0;k<idx.length;k+=3) {
      const a=idx[k]*3,b=idx[k+1]*3,d=idx[k+2]*3;
      const normal=unit([n[a]+n[b]+n[d], n[a+1]+n[b+1]+n[d+1], n[a+2]+n[b+2]+n[d+2]]);
      const shade=.48+.52*Math.abs(dot(normal,light));
      const srgb = value => Math.round(255*Math.min(1, Math.max(0, value <= .0031308 ? value*12.92 : 1.055*Math.pow(value,1/2.4)-.055)));
      const color=[0,1,2].map(i => srgb((c[a+i]+c[b+i]+c[d+i])/3*shade));
      triangles.push({pts:[a,b,d].map(i=>[projected[i],projected[i+1]]),depth:(projected[a+2]+projected[b+2]+projected[d+2])/3,color});
    }
  }
  const canvas=createCanvas(256,256), ctx=canvas.getContext('2d'), scale=218/Math.max(maxX-minX,maxY-minY), midX=(minX+maxX)/2, midY=(minY+maxY)/2;
  triangles.sort((a,b)=>a.depth-b.depth);
  for(const t of triangles) {
    ctx.fillStyle=`rgb(${t.color.join(',')})`;ctx.beginPath();
    t.pts.forEach(([x,y],i)=>{const X=128+(x-midX)*scale,Y=128-(y-midY)*scale; if(i)ctx.lineTo(X,Y);else ctx.moveTo(X,Y);});
    ctx.closePath();ctx.fill();
  }
  await writeFile('Images/EditorAssets/asset-fern.png', await canvas.encode('png'));
  console.log(`Captured fern mesh: ${triangles.length} triangles`);
} finally { await rm(tmp,{recursive:true,force:true}); }
