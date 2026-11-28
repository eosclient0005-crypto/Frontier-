import * as THREE from 'three';
import { clamp, lerp, mulberry32, smoothstep } from '../math/noise';
import { ProceduralTextureLibrary } from '../graphics/proceduralTextures';

/**
 * LEVEL [13] · ABANDONED ROLLERCOASTER & BOARDWALK PIER (HURRICANE SURGE)
 *
 * Chunk pattern (chunk index modulo 3):
 *   0 → BANKED_RING     : 360° wooden wall-of-death ring banked 70°, lifted 7m on a wooden ramp.
 *   1 → CORKSCREW_LOOP  : vertical 9m corkscrew loop with a 4m lateral helix (exit clears entry).
 *   2 → PENDULUM_SHIP   : severed pier gap spanned only by a swinging pirate-ship plank.
 *
 * Wall-rides: a vehicle at ≥ COASTER_ENTRY_SPEED that enters a track's capture corridor is snapped
 * onto the track frame. The normal is arbitrary (70° banked walls, inverted loop tops). It stays on
 * while it keeps above the local minimum speed; below that it drops onto the flooded pier deck.
 */

export const COASTER_OCEAN_Y = -3.0;
export const COASTER_SEABED_Y = -22.0;
export const COASTER_ENTRY_SPEED = 24.0;
export const COASTER_GAP_HALF = 7.4;

const DS = 0.5; // track sample spacing (m)
const RING_RADIUS = 20.0;
const RING_HEIGHT = 7.0;
const RING_BANK_RAD = (70 * Math.PI) / 180;
const RING_MIN_SPEED = 23.2; // sqrt(g·tan70°·R) at R = 20 m
const RING_DRIFT = 5.0;
const LOOP_RADIUS = 9.0;
const LOOP_MIN_SPEED = 11.5;
const LOOP_DRIFT = 4.0;
const RAMP_HALF_W = 3.2;
const CAR_CLEAR = 0.95;
const PENDULUM_ARM = 18.0;
const PENDULUM_AMP = 0.42; // ±24°
const PENDULUM_OMEGA = (2 * Math.PI) / 9.0; // 9 s swing period
const PLANK_HALF_LEN = 10.0;
const PLANK_HALF_W = 3.2;
const PIER_EDGE_OFFSET = 58.0; // beyond the road edge where the pier ends

export type CoasterChunkKind = 'BANKED_RING' | 'CORKSCREW_LOOP' | 'PENDULUM_SHIP';

export function coasterKindForChunk(chunkIndex: number): CoasterChunkKind {
  const m = ((chunkIndex % 3) + 3) % 3;
  return m === 0 ? 'BANKED_RING' : m === 1 ? 'CORKSCREW_LOOP' : 'PENDULUM_SHIP';
}

/** 0..1 mask of the severed pier gap (PENDULUM_SHIP chunks only). */
export function coasterGapMaskAt(z: number, chunkLength: number): number {
  const ci = Math.floor(-z / chunkLength);
  if (coasterKindForChunk(ci) !== 'PENDULUM_SHIP') return 0;
  const zc = -(ci + 0.5) * chunkLength;
  return 1 - smoothstep(COASTER_GAP_HALF, COASTER_GAP_HALF + 1.2, Math.abs(z - zc));
}

export interface RideTrack {
  count: number;
  px: Float64Array;
  py: Float64Array;
  pz: Float64Array;
  tx: Float64Array;
  ty: Float64Array;
  tz: Float64Array;
  nx: Float64Array;
  ny: Float64Array;
  nz: Float64Array;
  bx: Float64Array;
  by: Float64Array;
  bz: Float64Array;
  minSpeed: Float64Array;
  minX: number;
  maxX: number;
  minY: number;
  maxY: number;
  minZ: number;
  maxZ: number;
}

/** Walkable wooden approach ramp (terrain strip) between two (z, height) ends in one lane. */
export interface WoodenRamp {
  zA: number;
  hA: number;
  zB: number;
  hB: number;
  laneOffset: number;
}

export interface PendulumShip {
  pivotX: number;
  pivotZ: number;
  swing: THREE.Group; // galleon hull + deck (deck top = physical plank surface)
  chains: THREE.Mesh[];
  pillars: THREE.Group; // overhead gantry (static)
}

export interface CoasterPierChunkData {
  chunkIndex: number;
  kind: CoasterChunkKind;
  track: RideTrack | null;
  ramps: WoodenRamp[];
  laneX: (z: number) => number;
  ship: PendulumShip | null;
}

interface PointList {
  pts: THREE.Vector3[];
  refN: THREE.Vector3[];
}

export class CoasterPierBuilder {
  public readonly atmosphereFxGroup = new THREE.Group();

  private readonly woodMat: THREE.MeshStandardMaterial;
  private readonly darkWoodMat: THREE.MeshStandardMaterial;
  private readonly rustMat: THREE.MeshStandardMaterial;
  private readonly pilingMat: THREE.MeshStandardMaterial;
  private readonly boothMat: THREE.MeshStandardMaterial;
  private readonly floodMat: THREE.MeshStandardMaterial;
  private readonly shipHullMat: THREE.MeshStandardMaterial;
  private readonly sailMat: THREE.MeshStandardMaterial;
  private readonly neonMat: THREE.MeshBasicMaterial;
  private readonly neonPinkMat: THREE.MeshBasicMaterial;
  private readonly bulbMat: THREE.MeshBasicMaterial;
  private readonly oceanMat: THREE.MeshStandardMaterial;
  private readonly foamMat: THREE.MeshBasicMaterial;

  private readonly unitBoxGeo = new THREE.BoxGeometry(1, 1, 1);
  private readonly unitCylGeo = new THREE.CylinderGeometry(0.5, 0.5, 1, 6);
  private readonly bulbGeo = new THREE.SphereGeometry(0.22, 6, 5);

  private readonly _dummy = new THREE.Object3D();
  private readonly _m = new THREE.Matrix4();
  private readonly _n = new THREE.Vector3();
  private readonly _t = new THREE.Vector3();
  private readonly _r = new THREE.Vector3();
  private readonly _p = new THREE.Vector3();

  private clock = 0;
  private rainPoints!: THREE.Points;
  private rainPositions!: Float32Array;
  private foamMesh!: THREE.InstancedMesh;
  private foamBase!: Float32Array;
  private readonly spinners: THREE.Object3D[] = [];
  private readonly activeShips: PendulumShip[] = [];

  /** Current wall-ride: track owner chunk and sample index. */
  private ride: { data: CoasterPierChunkData; idx: number } | null = null;

  public onRideEngage: ((pos: THREE.Vector3) => void) | null = null;
  public onRideDrop: ((pos: THREE.Vector3) => void) | null = null;

  constructor(textures: ProceduralTextureLibrary) {
    this.woodMat = new THREE.MeshStandardMaterial({ color: 0x8c5e36, roughness: 0.95, metalness: 0 });
    this.darkWoodMat = new THREE.MeshStandardMaterial({ color: 0x4a2f1b, roughness: 0.98, metalness: 0 });
    this.rustMat = new THREE.MeshStandardMaterial({ color: 0x6c4a33, roughness: 0.7, metalness: 0.35 });
    this.pilingMat = new THREE.MeshStandardMaterial({ color: 0x2e2118, roughness: 1.0, metalness: 0 });
    this.boothMat = new THREE.MeshStandardMaterial({ color: 0xb7a58a, roughness: 0.9, metalness: 0 });
    this.floodMat = new THREE.MeshStandardMaterial({
      color: 0x1d4c58,
      roughness: 0.1,
      metalness: 0.2,
      transparent: true,
      opacity: 0.78,
    });
    this.shipHullMat = new THREE.MeshStandardMaterial({ color: 0x3b1f14, roughness: 0.85, metalness: 0 });
    this.sailMat = new THREE.MeshStandardMaterial({ color: 0xe7dcc2, roughness: 0.9, side: THREE.DoubleSide });
    this.neonMat = new THREE.MeshBasicMaterial({ color: 0x7dfcff });
    this.neonPinkMat = new THREE.MeshBasicMaterial({ color: 0xff3fb4 });
    this.bulbMat = new THREE.MeshBasicMaterial({ color: 0xffc36a });
    this.oceanMat = new THREE.MeshStandardMaterial({
      color: 0x0c3543,
      roughness: 0.22,
      metalness: 0.15,
      transparent: true,
      opacity: 0.94,
    });
    this.foamMat = new THREE.MeshBasicMaterial({ color: 0xdfeff3, transparent: true, opacity: 0.85 });

    this.buildAtmosphere(textures);
  }

  // ───────────────────────────────────────────────────────────────────────────
  // ATMOSPHERE
  // ───────────────────────────────────────────────────────────────────────────
  private buildAtmosphere(textures: ProceduralTextureLibrary): void {
    const ocean = new THREE.Mesh(new THREE.PlaneGeometry(1400, 1400), this.oceanMat);
    ocean.rotation.x = -Math.PI / 2;
    ocean.position.set(0, COASTER_OCEAN_Y, -400);
    ocean.receiveShadow = true;
    this.atmosphereFxGroup.add(ocean);

    const rng = mulberry32(60131);
    const foamCount = 90;
    this.foamMesh = new THREE.InstancedMesh(new THREE.BoxGeometry(1, 1, 1), this.foamMat, foamCount);
    this.foamBase = new Float32Array(foamCount * 4);
    for (let i = 0; i < foamCount; i++) {
      const side = i % 2 === 0 ? -1 : 1;
      this.foamBase[i * 4] = side * (82 + rng() * 200);
      this.foamBase[i * 4 + 1] = -400 + (rng() - 0.5) * 900;
      this.foamBase[i * 4 + 2] = 4 + rng() * 12;
      this.foamBase[i * 4 + 3] = rng() * Math.PI * 2;
    }
    this.atmosphereFxGroup.add(this.foamMesh);

    const rainCount = 1200;
    this.rainPositions = new Float32Array(rainCount * 3);
    for (let i = 0; i < rainCount; i++) {
      this.rainPositions[i * 3] = (rng() - 0.5) * 140;
      this.rainPositions[i * 3 + 1] = rng() * 45;
      this.rainPositions[i * 3 + 2] = (rng() - 0.5) * 220;
    }
    const rainGeo = new THREE.BufferGeometry();
    rainGeo.setAttribute('position', new THREE.BufferAttribute(this.rainPositions, 3));
    this.rainPoints = new THREE.Points(
      rainGeo,
      new THREE.PointsMaterial({
        map: textures.particleSoft,
        color: 0xb9d8e8,
        size: 0.35,
        transparent: true,
        opacity: 0.45,
        depthWrite: false,
      })
    );
    this.rainPoints.frustumCulled = false;
    this.atmosphereFxGroup.add(this.rainPoints);
  }

  /** Per-frame ambient animation (foam, rain, pendulum galleons, Ferris wheels). */
  public stepAmbient(dt: number, elapsed: number, playerZ: number): void {
    this.clock = elapsed;

    for (let i = 0; i < this.foamBase.length / 4; i++) {
      const crest = Math.max(0, Math.sin(elapsed * 0.9 + this.foamBase[i * 4 + 3]));
      const amp = this.foamBase[i * 4 + 2];
      this._dummy.position.set(this.foamBase[i * 4], COASTER_OCEAN_Y + 0.05 + crest * 0.6, this.foamBase[i * 4 + 1]);
      this._dummy.rotation.set(0, 0, 0);
      this._dummy.scale.set(amp * 0.5 + 1.0, 0.4 + crest * 1.2, 2.2 + crest * 1.4);
      this._dummy.updateMatrix();
      this.foamMesh.setMatrixAt(i, this._dummy.matrix);
    }
    this.foamMesh.instanceMatrix.needsUpdate = true;

    const pos = this.rainPositions;
    for (let i = 0; i < pos.length / 3; i++) {
      pos[i * 3 + 1] -= dt * (34 + (i % 7));
      if (pos[i * 3 + 1] < 0) pos[i * 3 + 1] += 45;
    }
    (this.rainPoints.geometry.attributes.position as THREE.BufferAttribute).needsUpdate = true;
    this.rainPoints.position.set(0, 0, playerZ);

    for (const s of this.spinners) s.rotation.z += dt * 0.32;

    const th = this.pendulumAngle();
    const cosT = Math.cos(th);
    const sinT = Math.sin(th);
    for (const ship of this.activeShips) {
      const cx = ship.pivotX;
      const cy = PENDULUM_ARM - PENDULUM_ARM * cosT;
      const cz = ship.pivotZ + PENDULUM_ARM * sinT;
      ship.swing.position.set(cx, cy, cz);
      ship.swing.rotation.set(-th, 0, 0);
      for (let c = 0; c < ship.chains.length; c++) {
      const side = c === 0 ? -1 : 1;
      const top = this._p.set(cx + side * 2.6, PENDULUM_ARM + 0.9, ship.pivotZ);
      const bottom = this._t.set(cx + side * 2.6, cy + 0.4, cz);
      const dir = this._n.subVectors(bottom, top);
      const len = dir.length();
      const chain = ship.chains[c];
      chain.position.addVectors(top, bottom).multiplyScalar(0.5);
      chain.quaternion.setFromUnitVectors(this._r.set(0, 1, 0), dir.normalize());
      chain.scale.set(0.12, len, 0.12);
      }
    }
  }

  // ───────────────────────────────────────────────────────────────────────────
  // TRACK GENERATION
  // ───────────────────────────────────────────────────────────────────────────
  private finalizeTrack(list: PointList, minSpeed: number): RideTrack {
    const n = list.pts.length;
    const t: RideTrack = {
      count: n,
      px: new Float64Array(n),
      py: new Float64Array(n),
      pz: new Float64Array(n),
      tx: new Float64Array(n),
      ty: new Float64Array(n),
      tz: new Float64Array(n),
      nx: new Float64Array(n),
      ny: new Float64Array(n),
      nz: new Float64Array(n),
      bx: new Float64Array(n),
      by: new Float64Array(n),
      bz: new Float64Array(n),
      minSpeed: new Float64Array(n),
      minX: Infinity,
      maxX: -Infinity,
      minY: Infinity,
      maxY: -Infinity,
      minZ: Infinity,
      maxZ: -Infinity,
    };
    const tan = new THREE.Vector3();
    const nor = new THREE.Vector3();
    const bin = new THREE.Vector3();
    for (let i = 0; i < n; i++) {
      const p = list.pts[i];
      const a = list.pts[Math.max(0, i - 1)];
      const b = list.pts[Math.min(n - 1, i + 1)];
      tan.subVectors(b, a);
      if (tan.lengthSq() < 1e-10) tan.set(0, 0, -1);
      tan.normalize();
      nor.copy(list.refN[i]);
      nor.addScaledVector(tan, -nor.dot(tan));
      if (nor.lengthSq() < 1e-8) nor.set(0, 1, 0);
      nor.normalize();
      bin.crossVectors(tan, nor).normalize();

      t.px[i] = p.x;
      t.py[i] = p.y;
      t.pz[i] = p.z;
      t.tx[i] = tan.x;
      t.ty[i] = tan.y;
      t.tz[i] = tan.z;
      t.nx[i] = nor.x;
      t.ny[i] = nor.y;
      t.nz[i] = nor.z;
      t.bx[i] = bin.x;
      t.by[i] = bin.y;
      t.bz[i] = bin.z;
      t.minSpeed[i] = minSpeed;
      t.minX = Math.min(t.minX, p.x);
      t.maxX = Math.max(t.maxX, p.x);
      t.minY = Math.min(t.minY, p.y);
      t.maxY = Math.max(t.maxY, p.y);
      t.minZ = Math.min(t.minZ, p.z);
      t.maxZ = Math.max(t.maxZ, p.z);
    }
    return t;
  }

  /** Banked 360° wall-of-death ring; starts at P0 heading -Z and returns drifted +X by RING_DRIFT. */
  private buildBankedRing(P0: THREE.Vector3): PointList {
    const out: PointList = { pts: [], refN: [] };
    const Side = new THREE.Vector3(1, 0, 0);
    const Fwd = new THREE.Vector3(0, 0, -1);
    const C = P0.clone().addScaledVector(Side, RING_RADIUS);
    const steps = Math.ceil((2 * Math.PI * RING_RADIUS) / DS);
    const cosB = Math.cos(RING_BANK_RAD);
    const sinB = Math.sin(RING_BANK_RAD);
    for (let k = 0; k <= steps; k++) {
      const th = (k / steps) * Math.PI * 2;
      const P = C.clone()
        .addScaledVector(Side, -RING_RADIUS * Math.cos(th))
        .addScaledVector(Fwd, RING_RADIUS * Math.sin(th))
        .addScaledVector(Side, (RING_DRIFT * th) / (Math.PI * 2));
      const inward = new THREE.Vector3(C.x - P.x, 0, C.z - P.z).normalize();
      // Surface inclined 70° from horizontal: normal leans 70° from vertical, toward the centre.
      const N = new THREE.Vector3(0, cosB, 0).addScaledVector(inward, sinB);
      out.pts.push(P);
      out.refN.push(N);
    }
    return out;
  }

  /** Vertical corkscrew loop in the YZ plane with a lateral helix drift. Bottom sits on the deck. */
  private buildCorkscrewLoop(P0: THREE.Vector3): PointList {
    const out: PointList = { pts: [], refN: [] };
    const Up = new THREE.Vector3(0, 1, 0);
    const Side = new THREE.Vector3(1, 0, 0);
    const Fwd = new THREE.Vector3(0, 0, -1);
    const C = P0.clone().addScaledVector(Up, LOOP_RADIUS);
    const steps = Math.ceil((2 * Math.PI * LOOP_RADIUS) / DS);
    for (let k = 0; k <= steps; k++) {
      const th = (k / steps) * Math.PI * 2;
      const P = C.clone()
        .addScaledVector(Up, -LOOP_RADIUS * Math.cos(th))
        .addScaledVector(Fwd, LOOP_RADIUS * Math.sin(th))
        .addScaledVector(Side, (LOOP_DRIFT * th) / (Math.PI * 2));
      const N = new THREE.Vector3().subVectors(C, P).normalize();
      out.pts.push(P);
      out.refN.push(N);
    }
    return out;
  }

  // ───────────────────────────────────────────────────────────────────────────
  // CHUNK POPULATION
  // ───────────────────────────────────────────────────────────────────────────
  public populateCoasterPierChunk(
    chunkGroup: THREE.Group,
    chunkIndex: number,
    zStart: number,
    chunkLength: number,
    centerXAt: (z: number) => number,
    halfWidth: number
  ): CoasterPierChunkData {
    const kind = coasterKindForChunk(chunkIndex);
    const rng = mulberry32(((chunkIndex + 8191) * 2246822519) >>> 0);
    const laneX = centerXAt;
    const data: CoasterPierChunkData = {
      chunkIndex,
      kind,
      track: null,
      ramps: [],
      laneX,
      ship: null,
    };

    const pilingMats: THREE.Matrix4[] = [];
    const boothMats: THREE.Matrix4[] = [];
    const floodMats: THREE.Matrix4[] = [];
    const gapZ = zStart - chunkLength * 0.5;

    // 1. Pilings along both outer edges of the pier, dropping into the storm seabed.
    const pilingH = 0 - COASTER_SEABED_Y;
    for (const side of [-1, 1]) {
      for (let z = zStart - 4; z > zStart - chunkLength; z -= 7.5) {
        this._dummy.position.set(laneX(z) + side * (halfWidth + PIER_EDGE_OFFSET), COASTER_SEABED_Y + pilingH * 0.5, z);
        this._dummy.rotation.set(0, 0, 0);
        this._dummy.scale.set(0.9, pilingH, 0.9);
        this._dummy.updateMatrix();
        pilingMats.push(this._dummy.matrix.clone());
      }
    }

    // 2. Flooded midway arcade booths with a few dead neon tubes.
    for (const side of [-1, 1]) {
      for (let k = 0; k < 3; k++) {
        const z = zStart - chunkLength * (0.12 + k * 0.1) - rng() * 3;
        if (kind === 'PENDULUM_SHIP' && Math.abs(z - gapZ) < 12) continue;
        const bx = laneX(z) + side * (halfWidth + 9.5 + rng() * 6);
        this._dummy.position.set(bx, 1.7, z);
        this._dummy.rotation.set(0, 0, 0);
        this._dummy.scale.set(6.2, 3.4, 4.6);
        this._dummy.updateMatrix();
        boothMats.push(this._dummy.matrix.clone());
        this._dummy.position.set(bx, 3.6, z);
        this._dummy.scale.set(7.0, 0.5, 5.2);
        this._dummy.updateMatrix();
        boothMats.push(this._dummy.matrix.clone());
        // Flooded floor: standing water ~0.8 m deep over the midway.
        this._dummy.position.set(bx, 0.8, z);
        this._dummy.rotation.set(-Math.PI / 2, 0, 0);
        this._dummy.scale.set(6.4, 4.8, 1);
        this._dummy.updateMatrix();
        floodMats.push(this._dummy.matrix.clone());
        const sign = new THREE.Mesh(this.unitBoxGeo, rng() > 0.45 ? this.neonMat : this.neonPinkMat);
        sign.position.set(bx - side * 3.0, 4.5, z);
        sign.scale.set(4.2, 0.6, 0.12);
        chunkGroup.add(sign);
      }
    }

    // 3. Apron set-pieces: Ferris wheel (loop chunks) or carousel ruin (ring chunks).
    if (kind === 'CORKSCREW_LOOP') {
      const wheelZ = zStart - chunkLength * 0.78;
      const wheel = this.buildFerrisWheel();
      wheel.position.set(laneX(wheelZ) - (halfWidth + 26), 0, wheelZ);
      chunkGroup.add(wheel);
    } else if (kind === 'BANKED_RING') {
      const carouselZ = zStart - chunkLength * 0.84;
      const carousel = this.buildCarouselRuin();
      carousel.position.set(laneX(carouselZ) + halfWidth + 22, 0, carouselZ);
      chunkGroup.add(carousel);
    }

    // 4. Broken string lights strung along both sides of the midway.
    for (const side of [-1, 1]) {
      for (let z = zStart - 6; z > zStart - chunkLength; z -= 6) {
        if (rng() < 0.22) continue; // dead bulb
        const bulb = new THREE.Mesh(this.bulbGeo, this.bulbMat);
        bulb.position.set(laneX(z) + side * (halfWidth + 12), 6.2 - Math.sin(z * 0.2) * 0.6, z);
        chunkGroup.add(bulb);
      }
    }

    // 5. Kind-specific coaster / ship content.
    if (kind === 'BANKED_RING') {
      const topZ = zStart - 48;
      const entryZ = zStart - 26;
      const exitZ = zStart - 70;
      const ringStart = new THREE.Vector3(laneX(topZ), RING_HEIGHT, topZ);
      data.track = this.finalizeTrack(this.buildBankedRing(ringStart), RING_MIN_SPEED);
      data.ramps.push({ zA: topZ, hA: RING_HEIGHT, zB: entryZ, hB: 0, laneOffset: 0 });
      data.ramps.push({ zA: topZ, hA: RING_HEIGHT, zB: exitZ, hB: 0, laneOffset: RING_DRIFT });
      this.addTrackVisuals(chunkGroup, data.track);
    } else if (kind === 'CORKSCREW_LOOP') {
      const bottomZ = zStart - 26;
      const loopStart = new THREE.Vector3(laneX(bottomZ), 0, bottomZ);
      data.track = this.finalizeTrack(this.buildCorkscrewLoop(loopStart), LOOP_MIN_SPEED);
      this.addTrackVisuals(chunkGroup, data.track);
    } else {
      const ship = this.buildPendulumShip(laneX(gapZ), gapZ);
      chunkGroup.add(ship.swing);
      for (const c of ship.chains) chunkGroup.add(c);
      chunkGroup.add(ship.pillars);
      data.ship = ship;
      this.activeShips.push(ship);
    }

    this.flushInstanced(chunkGroup, this.unitCylGeo, this.pilingMat, pilingMats);
    this.flushInstanced(chunkGroup, this.unitBoxGeo, this.boothMat, boothMats);
    this.flushInstanced(chunkGroup, this.unitBoxGeo, this.floodMat, floodMats);

    return data;
  }

  /** Removes per-chunk animated references when the streamer drops a chunk. */
  public forgetChunk(data: CoasterPierChunkData): void {
    if (data.ship) {
      const i = this.activeShips.indexOf(data.ship);
      if (i >= 0) this.activeShips.splice(i, 1);
    }
    if (this.ride && this.ride.data === data) this.ride = null;
  }

  private flushInstanced(group: THREE.Group, geo: THREE.BufferGeometry, mat: THREE.Material, mats: THREE.Matrix4[]): void {
    if (mats.length === 0) return;
    const im = new THREE.InstancedMesh(geo, mat, mats.length);
    for (let i = 0; i < mats.length; i++) im.setMatrixAt(i, mats[i]);
    im.instanceMatrix.needsUpdate = true;
    im.castShadow = true;
    im.receiveShadow = true;
    group.add(im);
  }

  private addTrackVisuals(group: THREE.Group, track: RideTrack): void {
    const railOffset = 0.72;
    for (const side of [-1, 1]) {
      const pts: THREE.Vector3[] = [];
      for (let i = 0; i < track.count; i += 2) {
        pts.push(
          new THREE.Vector3(
            track.px[i] + track.bx[i] * railOffset * side - track.nx[i] * 0.16,
            track.py[i] + track.by[i] * railOffset * side - track.ny[i] * 0.16,
            track.pz[i] + track.bz[i] * railOffset * side - track.nz[i] * 0.16
          )
        );
      }
      if (pts.length < 2) continue;
      const curve = new THREE.CatmullRomCurve3(pts, false, 'centripetal');
      const tube = new THREE.Mesh(new THREE.TubeGeometry(curve, pts.length * 2, 0.12, 5, false), this.rustMat);
      tube.castShadow = true;
      tube.userData.uniqueGeo = true;
      group.add(tube);
    }

    const ties: THREE.Matrix4[] = [];
    const posts: THREE.Matrix4[] = [];
    for (let i = 0; i < track.count; i += 3) {
      this._p.set(track.px[i], track.py[i], track.pz[i]);
      this._r.set(track.bx[i], track.by[i], track.bz[i]);
      this._n.set(track.nx[i], track.ny[i], track.nz[i]);
      this._t.set(track.tx[i], track.ty[i], track.tz[i]).negate();
      this._m.makeBasis(this._r, this._n, this._t);
      this._dummy.position.copy(this._p).addScaledVector(this._n, -0.2);
      this._dummy.quaternion.setFromRotationMatrix(this._m);
      this._dummy.scale.set(2.1, 0.22, 0.32);
      this._dummy.updateMatrix();
      ties.push(this._dummy.matrix.clone());

      // Trestle posts from the track down to the flooded deck.
      if (this._p.y > 1.2 && i % 12 === 0) {
        this._dummy.position.set(this._p.x, this._p.y * 0.5, this._p.z);
        this._dummy.quaternion.identity();
        this._dummy.scale.set(0.38, this._p.y, 0.38);
        this._dummy.updateMatrix();
        posts.push(this._dummy.matrix.clone());
      }
    }
    this.flushInstanced(group, this.unitBoxGeo, this.darkWoodMat, ties);
    this.flushInstanced(group, this.unitCylGeo, this.woodMat, posts);
  }

  private buildFerrisWheel(): THREE.Group {
    const root = new THREE.Group();
    const spinner = new THREE.Group();
    spinner.position.set(0, 24, 0);
    root.add(spinner);
    this.spinners.push(spinner);

    const rimR = 16.0;
    const rim = new THREE.Mesh(new THREE.TorusGeometry(rimR, 0.42, 6, 32), this.rustMat);
    rim.userData.uniqueGeo = true;
    spinner.add(rim);
    for (let i = 0; i < 12; i++) {
      const a = (i / 12) * Math.PI * 2;
      const spoke = new THREE.Mesh(this.unitBoxGeo, this.darkWoodMat);
      spoke.position.set(Math.cos(a) * rimR * 0.5, Math.sin(a) * rimR * 0.5, 0);
      spoke.rotation.z = a;
      spoke.scale.set(rimR, 0.28, 0.28);
      spinner.add(spoke);
      if (i % 2 === 0 || i === 3) {
        const gon = new THREE.Mesh(this.unitBoxGeo, i % 4 === 0 ? this.neonMat : this.boothMat);
        gon.position.set(Math.cos(a) * rimR, Math.sin(a) * rimR - 0.9, 0);
        gon.scale.set(1.8, 1.4, 1.6);
        spinner.add(gon);
      }
    }
    const hub = new THREE.Mesh(this.unitCylGeo, this.rustMat);
    hub.rotation.x = Math.PI / 2;
    hub.scale.set(1.6, 2.2, 1.6);
    spinner.add(hub);
    for (const s of [-1, 1]) {
      const leg = new THREE.Mesh(this.unitBoxGeo, this.darkWoodMat);
      leg.position.set(s * 6.5, 12, 0);
      leg.rotation.z = -s * 0.26;
      leg.scale.set(0.8, 26, 0.8);
      root.add(leg);
    }
    return root;
  }

  private buildCarouselRuin(): THREE.Group {
    const g = new THREE.Group();
    const base = new THREE.Mesh(this.unitCylGeo, this.boothMat);
    base.scale.set(12, 1.0, 12);
    base.position.set(0, 0.2, 0);
    g.add(base);
    const roof = new THREE.Mesh(new THREE.ConeGeometry(12.5, 6.0, 10, 1, true), this.sailMat);
    roof.position.set(0, 8.8, 0);
    roof.userData.uniqueGeo = true;
    g.add(roof);
    for (let i = 0; i < 10; i++) {
      const a = (i / 10) * Math.PI * 2;
      const pole = new THREE.Mesh(this.unitCylGeo, this.rustMat);
      pole.position.set(Math.cos(a) * 8.6, 4.2, Math.sin(a) * 8.6);
      pole.scale.set(0.35, 8.4, 0.35);
      g.add(pole);
      if (i % 3 === 0) {
        const horse = new THREE.Mesh(this.unitBoxGeo, this.neonPinkMat);
        horse.position.set(Math.cos(a) * 8.0, 2.4, Math.sin(a) * 8.0);
        horse.scale.set(0.9, 2.3, 0.6);
        g.add(horse);
      }
    }
    return g;
  }

  /**
   * Pirate galleon hanging from an overhead pivot. Local frame: origin = deck top at rest, +Z = bow.
   * `swing` is repositioned each frame (pivot + arm), so the deck top traces the physical plank.
   */
  private buildPendulumShip(pivotX: number, pivotZ: number): PendulumShip {
    const pillars = new THREE.Group();
    const beam = new THREE.Mesh(this.unitBoxGeo, this.darkWoodMat);
    beam.position.set(pivotX, PENDULUM_ARM + 1.0, pivotZ);
    beam.scale.set(16, 1.2, 1.2);
    pillars.add(beam);
    for (const s of [-1, 1]) {
      const post = new THREE.Mesh(this.unitBoxGeo, this.darkWoodMat);
      post.position.set(pivotX + s * 7.6, PENDULUM_ARM * 0.5, pivotZ);
      post.scale.set(1.0, PENDULUM_ARM, 1.0);
      pillars.add(post);
    }

    const swing = new THREE.Group();
    const deck = new THREE.Mesh(this.unitBoxGeo, this.woodMat);
    deck.scale.set(6.0, 0.25, 18.0);
    deck.position.set(0, -0.125, 0);
    swing.add(deck);
    const hull = new THREE.Mesh(this.unitBoxGeo, this.shipHullMat);
    hull.scale.set(6.4, 2.2, 19.0);
    hull.position.set(0, -1.3, 0);
    swing.add(hull);
    const bow = new THREE.Mesh(this.unitBoxGeo, this.shipHullMat);
    bow.scale.set(4.2, 1.6, 3.0);
    bow.position.set(0, -1.2, 10.2);
    bow.rotation.x = -0.4;
    swing.add(bow);
    for (const z of [-4.5, 3.5]) {
      const mast = new THREE.Mesh(this.unitCylGeo, this.darkWoodMat);
      mast.position.set(0, 6.0, z);
      mast.scale.set(0.35, 12.0, 0.35);
      swing.add(mast);
      const sail = new THREE.Mesh(new THREE.PlaneGeometry(6.2, 8.0), this.sailMat);
      sail.position.set(0, 7.0, z + 0.2);
      swing.add(sail);
    }
    const flag = new THREE.Mesh(this.unitBoxGeo, this.neonPinkMat);
    flag.position.set(0, 13.2, -4.5);
    flag.scale.set(1.8, 1.0, 0.1);
    swing.add(flag);
    for (let i = -3; i <= 3; i++) {
      const b = new THREE.Mesh(this.bulbGeo, this.bulbMat);
      b.position.set(3.2, 1.6, i * 2.6);
      swing.add(b);
    }

    const chains: THREE.Mesh[] = [];
    for (let c = 0; c < 2; c++) {
      const chain = new THREE.Mesh(this.unitCylGeo, this.rustMat);
      chains.push(chain);
    }
    return { pivotX, pivotZ, swing, chains, pillars };
  }

  // ───────────────────────────────────────────────────────────────────────────
  // QUERIES: pendulum plank, ramps
  // ───────────────────────────────────────────────────────────────────────────
  /** Swing angle of the pendulum ship (rad) at the current ambient clock. */
  public pendulumAngle(): number {
    return PENDULUM_AMP * Math.sin(PENDULUM_OMEGA * this.clock);
  }

  /**
   * Height of a pendulum plank at (x,z), or null when (x,z) is off the plank.
   * The plank is the galleon's deck plane: y = cy + tanθ·(z - cz), spanning ±PLANK_HALF_LEN·cosθ along the swing.
   * outVel receives the surface velocity (vertical + longitudinal carry), used for tire velocity inheritance.
   */
  public samplePendulumPlank(data: CoasterPierChunkData, x: number, z: number, outVel: THREE.Vector3 | null): number | null {
    const ship = data.ship;
    if (!ship) return null;
    const th = this.pendulumAngle();
    const thDot = PENDULUM_AMP * PENDULUM_OMEGA * Math.cos(PENDULUM_OMEGA * this.clock);
    const cosT = Math.cos(th);
    const tanT = Math.tan(th);
    const cz = ship.pivotZ + PENDULUM_ARM * Math.sin(th);
    const cy = PENDULUM_ARM - PENDULUM_ARM * cosT;
    const dz = z - cz;
    if (Math.abs(dz) > PLANK_HALF_LEN * cosT) return null;
    if (Math.abs(x - ship.pivotX) > PLANK_HALF_W) return null;
    const h = cy + tanT * dz;
    if (outVel) {
      outVel.set(0, (dz / (cosT * cosT)) * thDot, PENDULUM_ARM * cosT * thDot);
    }
    return h;
  }

  /** Walkable wooden approach-ramp height at (x,z), or null outside any ramp lane. */
  public sampleRampHeight(data: CoasterPierChunkData, x: number, z: number): number | null {
    for (const r of data.ramps) {
      const zMin = Math.min(r.zA, r.zB);
      const zMax = Math.max(r.zA, r.zB);
      if (z < zMin || z > zMax) continue;
      const lane = data.laneX(z) + r.laneOffset;
      if (Math.abs(x - lane) > RAMP_HALF_W) continue;
      const t = clamp((z - r.zA) / (r.zB - r.zA), 0, 1);
      return lerp(r.hA, r.hB, smoothstep(0, 1, t));
    }
    return null;
  }

  // ───────────────────────────────────────────────────────────────────────────
  // 240Hz WALL-RIDE CAPTURE
  // ───────────────────────────────────────────────────────────────────────────
  public isRiding(): boolean {
    return this.ride !== null;
  }

  public releaseRide(): void {
    this.ride = null;
  }

  /**
   * Post-integration substep. Returns true when the vehicle was constrained to a coaster track.
   * Pose = track point + normal·clearance; up = track normal (may point inward/down on loop tops
   * and 70° banks); forward = track tangent; velocity projected onto the tangent.
   */
  public stepRide(
    carPos: THREE.Vector3,
    carQuat: THREE.Quaternion,
    linearVel: THREE.Vector3,
    angularVel: THREE.Vector3,
    chunks: CoasterPierChunkData[]
  ): boolean {
    const speed = linearVel.length();

    if (this.ride) {
      const { data, idx } = this.ride;
      const tr = data.track;
      if (!tr || chunks.indexOf(data) < 0) {
        this.ride = null;
      } else {
        let best = -1;
        let bestD2 = Infinity;
        const lo = Math.max(0, idx - 80);
        const hi = Math.min(tr.count - 1, idx + 80);
        for (let i = lo; i <= hi; i++) {
          const dx = carPos.x - tr.px[i];
          const dy = carPos.y - tr.py[i];
          const dz = carPos.z - tr.pz[i];
          const d2 = dx * dx + dy * dy + dz * dz;
          if (d2 < bestD2) {
            bestD2 = d2;
            best = i;
          }
        }
        const onRail = best >= 0 && bestD2 < 3.0 * 3.0;
        if (!onRail || speed < tr.minSpeed[best]) {
          if (this.onRideDrop) this.onRideDrop(carPos);
          this.ride = null;
        } else {
          const next = this.applyTrackPose(tr, best, carPos, carQuat, linearVel, angularVel);
          if (next < 0) {
            // Ran off the end of the track: hand back to normal physics (exit ramp / deck).
            this.ride = null;
          } else {
            this.ride.idx = next;
            return true;
          }
        }
      }
    }

    if (speed < COASTER_ENTRY_SPEED) return false;

    let bestData: CoasterPierChunkData | null = null;
    let bestIdx = -1;
    let bestD2 = Infinity;
    for (const data of chunks) {
      const tr = data.track;
      if (!tr) continue;
      if (
        carPos.x < tr.minX - 4 ||
        carPos.x > tr.maxX + 4 ||
        carPos.y < tr.minY - 4 ||
        carPos.y > tr.maxY + 4 ||
        carPos.z < tr.minZ - 4 ||
        carPos.z > tr.maxZ + 4
      ) {
        continue;
      }
      for (let i = 0; i < tr.count; i++) {
        const dx = carPos.x - tr.px[i];
        const dy = carPos.y - tr.py[i];
        const dz = carPos.z - tr.pz[i];
        const dn = dx * tr.nx[i] + dy * tr.ny[i] + dz * tr.nz[i];
        if (dn < -1.4 || dn > 2.8) continue;
        const db = dx * tr.bx[i] + dy * tr.by[i] + dz * tr.bz[i];
        if (Math.abs(db) > 2.6) continue;
        const sAlong = dx * tr.tx[i] + dy * tr.ty[i] + dz * tr.tz[i];
        if (sAlong < -0.6) continue; // behind the sample: not an entry
        if (i === tr.count - 1 && sAlong > 0.6) continue; // past the exit: not a re-entry
        const d2 = dx * dx + dy * dy + dz * dz;
        if (d2 < bestD2) {
          bestD2 = d2;
          bestData = data;
          bestIdx = i;
        }
      }
    }

    if (bestData && bestData.track) {
      const next = this.applyTrackPose(bestData.track, bestIdx, carPos, carQuat, linearVel, angularVel);
      if (next >= 0) {
        this.ride = { data: bestData, idx: next };
        if (this.onRideEngage) this.onRideEngage(carPos);
        return true;
      }
    }
    return false;
  }

  /**
   * Projects the car onto the track polyline around sample i, interpolates the frame between the
   * two bracketing samples, and writes the pose. Returns the nearest sample index to continue from,
   * or -1 when the car has run past either end of the track.
   */
  private applyTrackPose(
    tr: RideTrack,
    i: number,
    carPos: THREE.Vector3,
    carQuat: THREE.Quaternion,
    linearVel: THREE.Vector3,
    angularVel: THREE.Vector3
  ): number {
    const sAlong =
      (carPos.x - tr.px[i]) * tr.tx[i] + (carPos.y - tr.py[i]) * tr.ty[i] + (carPos.z - tr.pz[i]) * tr.tz[i];
    const j = sAlong >= 0 ? Math.min(tr.count - 1, i + 1) : Math.max(0, i - 1);
    if (j === i) {
      // At an end of the track: moving past the exit or well behind the entry ends the ride.
      if ((i === tr.count - 1 && sAlong > 0.02) || (i === 0 && sAlong < -0.6)) return -1;
    }
    const chord =
      j === i
        ? 1.0
        : Math.hypot(tr.px[j] - tr.px[i], tr.py[j] - tr.py[i], tr.pz[j] - tr.pz[i]) || 1e-6;
    const t = j === i ? 0 : clamp(Math.abs(sAlong) / chord, 0, 1);

    const px = lerp(tr.px[i], tr.px[j], t);
    const py = lerp(tr.py[i], tr.py[j], t);
    const pz = lerp(tr.pz[i], tr.pz[j], t);
    const n = this._n.set(
      lerp(tr.nx[i], tr.nx[j], t),
      lerp(tr.ny[i], tr.ny[j], t),
      lerp(tr.nz[i], tr.nz[j], t)
    ).normalize();
    const fwd = this._t.set(
      lerp(tr.tx[i], tr.tx[j], t),
      lerp(tr.ty[i], tr.ty[j], t),
      lerp(tr.tz[i], tr.tz[j], t)
    ).normalize();
    if (linearVel.dot(fwd) < 0) fwd.negate();
    // Car local: +X right, +Y up, +Z backward. right = up × backward.
    const back = this._r.copy(fwd).negate();
    const right = this._p.crossVectors(n, back).normalize();
    this._m.makeBasis(right, n, back);
    carQuat.setFromRotationMatrix(this._m);

    carPos.set(px, py, pz).addScaledVector(n, CAR_CLEAR);
    const along = linearVel.dot(fwd);
    linearVel.copy(fwd).multiplyScalar(along);
    angularVel.multiplyScalar(0.2);

    return t > 0.5 ? j : i;
  }

  /** Plank velocity for tire velocity inheritance (returns true when standing on a moving plank). */
  public sampleSurfaceVelocityInChunk(data: CoasterPierChunkData, x: number, z: number, outVel: THREE.Vector3): boolean {
    return this.samplePendulumPlank(data, x, z, outVel) !== null;
  }

  public dispose(): void {
    this.rainPoints.geometry.dispose();
    this.foamMesh.dispose();
  }
}
