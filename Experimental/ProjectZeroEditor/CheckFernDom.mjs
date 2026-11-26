// Exercises the browser UI in a DOM simulator. The 3D WebGL view is not available here.
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { JSDOM, VirtualConsole } from "jsdom";
import { createCanvas } from "@napi-rs/canvas";
const Errors = [], Console = new VirtualConsole();
Console.on("jsdomError", Error => Errors.push(Error.cause?.stack || Error.message));
Console.on("error", (...Messages) => { if (!String(Messages[0]).includes("WebGLRenderer")) Errors.push(Messages.join(" ")); });
const Dom = new JSDOM(readFileSync(new URL("./index.html", import.meta.url), "utf8"), {
  url: "http://localhost:8092/", runScripts: "dangerously", pretendToBeVisual: true, virtualConsole: Console,
  beforeParse(Window) {
    Window.structuredClone = structuredClone;
    Object.defineProperty(Window.navigator, "userAgent", { value: "Mozilla/5.0 Chrome/130.0.0.0 Safari/537.36" });
    Window.HTMLCanvasElement.prototype.getContext = function (Type) { if (Type !== "2d") return null; this._canvas ||= createCanvas(this.width || 300, this.height || 150); return this._canvas.getContext("2d"); };
    Window.HTMLCanvasElement.prototype.toDataURL = function () { return this._canvas?.toDataURL() || ""; };
    Window.matchMedia = () => ({ matches: true, addListener() {}, removeListener() {}, addEventListener() {}, removeEventListener() {} });
    Window.ResizeObserver = class { observe() {} disconnect() {} };
    Window.HTMLElement.prototype.scrollIntoView = function () {};
  },
});
try {
  const Window = Dom.window, Document = Window.document;
  const Wait = () => new Promise(Resolve => setTimeout(Resolve, 120));
  const Click = async (Element) => { assert.ok(Element, "Missing control"); Element.click(); await Wait(); };
  const Button = (Scope, Text) => [...Scope.querySelectorAll("button")].find(Item => Item.textContent.trim() === Text);
  await new Promise(Resolve => setTimeout(Resolve, 900));
  await Click(Document.querySelector('button[aria-label="Construct"]'));
  const Modal = Document.querySelector('[role="dialog"][aria-label="Construct"]');
  assert.ok(Modal);
  // Follow-up 85's upstream Import entry is present alongside the fern.
  assert.ok([...Modal.querySelectorAll("button")].some(Item => Item.textContent.includes("Import")));
  await Click(Button(Modal, "Plants"));
  const Template = Modal.querySelector('[data-construct-id="fern-template"]');
  assert.ok(Template.querySelector('img[data-native-icon="asset-fern"]'));
  await Click(Template);
  assert.ok(Modal.querySelector('[role="tab"][aria-label="Overview"]'));
  await Click(Button(Modal, "Add to scene"));
  assert.equal(Document.querySelector('[role="dialog"][aria-label="Construct"]'), null);
  const Row = [...Document.querySelectorAll("[data-outliner-row]")].find(Element => Element.textContent.includes("Fern"));
  assert.ok(Row); assert.equal(Row.getAttribute("aria-selected"), "true");
  const Panel = Document.querySelector('[data-panel="fern"]');
  assert.ok(Panel.querySelector('[role="tabpanel"][aria-label="Overview"]'));
  assert.ok(Panel.querySelector('img[src^="data:image/png"]'));
  await Click(Panel.querySelector('[role="tab"][aria-label="Geometry"]'));
  assert.ok(Panel.textContent.includes("Mesh source"));
  await Click(Panel.querySelector('[role="tab"][aria-label="Generator"]'));
  const Architecture = Panel.querySelector('select[aria-label="Architecture"]');
  Architecture.value = "bipinnate_lacy"; Architecture.dispatchEvent(new Window.Event("change", { bubbles: true })); await Wait();
  assert.ok(Panel.querySelector('input[aria-label="Pinnule Sub-Pairs"]'));
  const Range = Panel.querySelector('input[aria-label="Frond Count"]');
  Object.getOwnPropertyDescriptor(Window.HTMLInputElement.prototype, "value").set.call(Range, "8");
  Range.dispatchEvent(new Window.Event("input", { bubbles: true })); Range.dispatchEvent(new Window.Event("change", { bubbles: true })); await Wait();
  await Click(Button(Panel, "Detail"));
  assert.ok(Panel.querySelector('input[aria-label="Lamina Thickness"]'));
  await Click(Button(Panel, "Surface"));
  assert.ok(Panel.querySelector('input[aria-label="Mature Mesophyll"]'));
  await Click(Row.querySelector('button[aria-label="Toggle Fern visibility"]')); assert.ok(Row.classList.contains("hidden-row"));
  await Click(Row.querySelector('button[aria-label="Toggle Fern visibility"]')); assert.ok(!Row.classList.contains("hidden-row"));
  await new Promise(Resolve => setTimeout(Resolve, 300));
  const Saved = JSON.parse(Window.localStorage.getItem("Frontier.ProjectZeroHtml.v1"));
  const Fern = Saved.Rows.find(Item => Item.Panel === "fern");
  assert.equal(Saved.Values[Fern.Id].FernParameters.architecture, "bipinnate_lacy");
  assert.equal(Saved.Values[Fern.Id].FernParameters.frondCount, 8);
  assert.deepEqual(Errors, []);
  console.log("Construct, upstream Import, geometry tabs, generator controls, icon, visibility and persistence passed (DOM simulation; no WebGL).");
} finally { Dom.window.close(); }
