import React, { useEffect, useId, useRef, useState } from "react";
import TransformPanel from "./TransformPanel.jsx";
import { PageStrip, PageTurn, PageLook } from "./PageCarousel.jsx";
import { FernGroups, FernParameters, FERN_ARCH_NAMES, BROADLEAF_NAMES } from "./FernSpecification.js";
import "./FernPanel.css";

export const FernRow = { Id: "fern-template", Name: "Fern", Panel: "fern", Icon: "asset-fern", Parent: "showcase", Description: "Procedural plant geometry · browser preview" };
const Sections = [
  ["overview", "Overview", "overview", "#7cb7ff"],
  ["components", "Components", "components", "#b59cff"],
  ["geometry", "Geometry", "geometry", "#5fe0a0"],
  ["generator", "Generator", "geometry", "#76c886"],
  ["material", "Material", "material", "#ff7eb6"],
  ["uv", "UV", "uv", "#7ab8ff"], ["fracture", "Fracture", "fracture", "#ff6b6b"],
  ["sdf", "SDF", "sdf", "#95c3ad"], ["lod", "LOD", "lod", "#edc181"],
  ["capture", "Icon capture", "capture", "#8fd3e8"],
];
// The same page strip and transition mechanics used by the geometry inspector.
function FernCarousel({ Pages }) {
  const [Index, Choose] = useState(0);
  const Motion = PageTurn(Index), Swipe = useRef(null);
  const Go = (Next) => { const Target = (Next + Sections.length) % Sections.length; Motion.Turn(Target); Choose(Target); };
  const Drawn = [Motion.Leaving, Index].filter(At => At !== null && Sections[At]);
  return <section className="pi-carousel" aria-label="Categories" data-navigation="carousel" style={{ "--pi-current": Sections[Index][3] }}>
    <PageStrip Owner="Category" Label="Categories" Index={Index} Pick={Go} Pages={Sections.map(([Id, Label]) => ({ Id, Label, ClassName: "pi-pill" }))} />
    <div className={"pc-pages" + (Motion.Leaving !== null ? " pc-turning" : "")}>{Drawn.map(At => {
      const [Id, Caption] = Sections[At], { className, hidden, ...Shown } = PageLook(At, Index, Motion);
      return <div key={Id} className={"pi-page " + className} role="tabpanel" aria-label={Caption} data-page={Id} {...Shown} {...(At === Index ? {
        onPointerDown: Event => { Swipe.current = Event.target.closest("input, textarea, button, select, [role=slider], label, [draggable=true]") ? null : { X: Event.clientX, Y: Event.clientY }; },
        onDragStart: () => { Swipe.current = null; },
        onPointerUp: Event => { const Start = Swipe.current; Swipe.current = null; if (!Start) return; const Dx = Event.clientX - Start.X, Dy = Event.clientY - Start.Y; if (Math.abs(Dx) > 60 && Math.abs(Dx) > Math.abs(Dy) * 1.5) Go(Index + (Dx < 0 ? 1 : -1)); },
      } : {})}>{Pages[Id]}</div>;
    })}</div>
  </section>;
}
const FernImage = ({ Size = 96 }) => <img className="fern-image" width={Size} height={Size} src={globalThis.window?.NativeAssets?.Icons?.["asset-fern"]} alt="" />;
const Section = ({ Title, Hint, Children, Open = true }) => <details className="fern-sec" open={Open || undefined}><summary><b>{Title}</b>{Hint && <small>{Hint}</small>}<span aria-hidden="true">⌄</span></summary><div className="fern-sec-body">{Children}</div></details>;

function Field({ Spec, Value, Change }) {
  const [Label, Key, Min, Max, Step, Unit = "", Factor = 1] = Spec;
  const Id = useId();
  const Shown = +((Value * Factor).toFixed(4));
  const [Draft, EditDraft] = useState(String(Shown));
  const Numeric = useRef(null);
  useEffect(() => { if (document.activeElement !== Numeric.current) EditDraft(String(Shown)); }, [Shown]);
  const Set = (Raw) => { const N = Number(Raw); if (Number.isFinite(N)) Change(Key, Math.min(Max, Math.max(Min, N)) / Factor); };
  return <div className="fern-ctl"><div className="fern-ctl-top"><label htmlFor={`fern-${Id}-${Key}`}>{Label}</label><span><input ref={Numeric} aria-label={`${Label} value`} type="number" value={Draft} min={Min} max={Max} step={Step} onChange={Event => EditDraft(Event.target.value)} onBlur={() => { if (Draft !== "") Set(Draft); else EditDraft(String(Shown)); }} onKeyDown={Event => { if (Event.key === "Enter") Event.currentTarget.blur(); }} /><small>{Unit}</small></span></div><input id={`fern-${Id}-${Key}`} aria-label={Label} type="range" min={Min} max={Max} step={Step} value={Shown} onChange={Event => Set(Event.target.value)} /></div>;
}
function SelectField({ Label, Value, Options, Change }) {
  return <label className="fern-select-label">{Label}<select aria-label={Label} value={Value} onChange={Event => Change(Event.target.value)}>{Options.map(([Key, Name]) => <option value={Key} key={Key}>{Name}</option>)}</select></label>;
}
function Surface({ Params, Set }) {
  return <div className="fern-generator"><Section Title="Chlorophyll, Cuticle & Senescence" Hint="SSS" Children={<>
    {[["Mature Mesophyll", "hueBase"], ["Young Flush / Crosier", "hueYoung"], ["Stipe & Rachis", "hueStipe"]].map(([Label, Key]) => <label className="fern-colour" key={Key}><span>{Label}</span><input type="color" aria-label={Label} value={Params[Key]} onChange={Event => Set(Key, Event.target.value)} /><code>{Params[Key]}</code></label>)}
    {FernGroups.surface[0][2].map(Spec => <Field key={Spec[1]} Spec={Spec} Value={Params[Spec[1]]} Change={Set} />)}
    <p className="fern-note">Pigments colour the generated vertices. The botanical demo’s translucency shader is not connected here.</p>
  </>} /></div>;
}
function Generator({ Params, Set, Values, Change }) {
  const [Tab, SetTab] = useState("shape");
  return <div className="fern-generator">
    <div className="fern-generator-head"><div><span>PROCEDURAL GEOMETRY</span><h3>Fern generator</h3></div><span className="fern-badge">DEMO</span></div>
    <div className="fern-tabs" role="tablist" aria-label="Fern generator pages">{[["shape", "Shape"], ["detail", "Detail"], ["surface", "Surface"]].map(([Key, Name]) => <button type="button" role="tab" aria-selected={Tab === Key} className={Tab === Key ? "active" : ""} onClick={() => SetTab(Key)} key={Key}>{Name}</button>)}</div>
    {Tab === "shape" && <>
      <Section Title="Botanical Architecture" Hint="frond plan" Children={<>
        <SelectField Label="Architecture" Value={Params.architecture} Options={Object.entries(FERN_ARCH_NAMES)} Change={Value => Set("architecture", Value)} />
        {Params.architecture === "broadleaf_rosette" && <SelectField Label="Leaf Species" Value={Params.broadleafPlaced} Options={Object.entries(BROADLEAF_NAMES)} Change={Value => Set("broadleafPlaced", Value)} />}
        <SelectField Label="Mesh Detail Tier" Value={String(Params.detailTier)} Options={[["1", "1 · Light Preview"], ["2", "2 · Standard Quad"], ["3", "3 · High-Poly Botanical"], ["4", "4 · Cinema Ultra-Mesh"]]} Change={Value => Set("detailTier", Number(Value))} />
        <label className="fern-seed">Seed <input type="number" aria-label="Fern seed" min="1" max="999999" value={Values.FernSeed ?? 42} onChange={Event => Change("FernSeed", Math.max(1, Math.min(999999, Number(Event.target.value) || 1)))} /></label>
        <p className="fern-note">Higher mesh tiers can take significant time and memory in the browser.</p>
      </>} />
      {FernGroups.shape.map(([Title, Hint, Fields], Index) => {
        if (Index === 1 && Params.architecture === "broadleaf_rosette") return null;
        return <Section key={Title} Title={Title} Hint={Hint} Open={Index !== 2} Children={Fields.filter(([, Key]) => Key !== "pinnulePairs" || ["bipinnate_lacy", "tree_fern", "maidenhair_fan"].includes(Params.architecture)).map(Spec => <Field key={Spec[1]} Spec={Spec} Value={Params[Spec[1]]} Change={Set} />)} />;
      })}
    </>}
    {Tab === "detail" && FernGroups.detail.map(([Title, Hint, Fields]) => <Section key={Title} Title={Title} Hint={Hint} Children={Fields.map(Spec => <Field key={Spec[1]} Spec={Spec} Value={Params[Spec[1]]} Change={Set} />)} />)}
    {Tab === "surface" && <Surface Params={Params} Set={Set} />}
  </div>;
}

export default function FernPanel({ Subject, Values, Change, Hidden, ToggleHidden }) {
  const Params = FernParameters(Values);
  const Set = (Key, Value) => Change("FernParameters", { ...(Values.FernParameters || {}), [Key]: Value });
  const Unsupported = (Name) => <section className="pi-card fern-unavailable"><h3>{Name}</h3><p>Not connected to fern geometry in this browser study. This page does not operate on a substitute primitive.</p></section>;
  const Overview = <>
    <section className="fern-hero"><FernImage Size={150} /><div className="fern-hero-copy"><span>GEOMETRY / PROCEDURAL</span><h2>{Subject.Name}</h2><p>{FERN_ARCH_NAMES[Params.architecture]}</p></div><span className="fern-hero-badge">FERN</span></section>
    <section className="pi-card"><div className="pi-card-head"><h3>Overview</h3><span className="pi-card-aside">Browser scene</span></div><div className="fern-overview-grid"><div><span>Fronds</span><b>{Math.round(Params.frondCount)}</b></div><div><span>Length</span><b>{Params.frondLength.toFixed(2)} <small>m</small></b></div><div><span>Detail</span><b>{Params.detailTier} <small>/ 4</small></b></div></div><p className="pi-footnote">Generated 3D mesh · deterministic seed {Values.FernSeed ?? 42}. Native C++ and botanical wind are not connected.</p></section>
    <section className="pi-card"><div className="pi-card-head"><h3>Quick controls</h3></div><button type="button" className="fern-visibility" aria-pressed={!Hidden} onClick={ToggleHidden}>{Hidden ? "Hidden" : "Visible"}</button><label className="fern-notes">Notes<textarea aria-label="Fern notes" value={Values.Notes || ""} onChange={Event => Change("Notes", Event.target.value)} placeholder="Add notes…" /></label></section>
  </>;
  const Pages = {
    overview: Overview,
    geometry: <><TransformPanel Values={Values} Change={Change} Note="Browser transform; no native plant is created." /><section className="pi-card"><h3>Mesh source</h3><p className="pi-footnote">Procedural botanical geometry. Edit its architecture and anatomy on the Generator page.</p></section></>,
    generator: <Generator Params={Params} Set={Set} Values={Values} Change={Change} />,
    material: <Surface Params={Params} Set={Set} />,
    components: Unsupported("Components"), uv: Unsupported("UV"), fracture: Unsupported("Fracture"), sdf: Unsupported("SDF"), lod: Unsupported("LOD"),
    capture: <section className="pi-card fern-capture"><h3>Object icon</h3><FernImage Size={128} /><p className="pi-footnote">Captured from the default generated fern mesh for the Construct menu and Outliner. A live capture tool is not yet connected.</p></section>,
  };
  return <div className="generic-inspector fern-inspector" data-panel="fern"><header className="inspector-heading"><div className="breadcrumbs">Inspector / Geometry</div><h1>{Subject.Name}</h1><p className="eyebrow">Procedural fern · HTML scene preview</p></header><div className="primitive-inspector"><FernCarousel key={Subject.Id} Pages={Pages} /></div></div>;
}
