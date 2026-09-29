// Draws each frame the bridge built: one dynamic geometry holding this
// frame's world-space triangles. Like the native backend it draws in phases
// (world, coplanar decals, vehicles, then textures with partial alpha,
// blended without depth writes) and renders a vehicle shadow map first.
// Camera, projection, fog and the shadow camera come from the bridge, so
// three.js never computes its own view.
import * as THREE from 'three';
import { PAL_HEIGHT, PAL_WIDTH } from './constants';
import {
  ASSET_TRACK_MODEL_BANK_1, ASSET_TRACK_MODEL_BANK_2, MATERIAL_ENV_CLUT, NO_MATERIAL, Rage, SPAN_FIELDS, type DecodedTexture, type TextureLevel,
} from './rage';
import { NAMEPLATE_HEIGHT, type Nameplate } from './nameplates';
import {
  ATLAS_LAYER_HEIGHT, ATLAS_LAYER_WIDTH, ATLAS_LEVEL_ORIGINS, mirrorFragment, mirrorVertex, nameplateFragment, nameplateVertex, shadowFragment, shadowVertex, skyFragment, skyVertex, worldFragment, worldVertex,
} from './shaders';

const VERTEX_CAPACITY = 600_000;
/* Decoding is the expensive part of a palette change; spread it out. */
const DECODES_PER_FRAME = 48;
/* Texture array layers: grown by doubling, and least-recently-drawn
 * materials give up their layer once the array is at its limit. */
const ATLAS_INITIAL_LAYERS = 64;
const ATLAS_MAX_LAYERS = 512;
const ATLAS_LAYER_BYTES = ATLAS_LAYER_WIDTH * ATLAS_LAYER_HEIGHT * 4;
/* Draw groups: the two world materials and the one shadow material. */
const OPAQUE = 0;
const TRANSPARENT = 1;
/* render_world.h RenderAssetSet: vehicles are model-bank meshes. */
const ASSET_MODEL_BANK = 0;
/* Field offsets within one span of Rage.spans() (SPAN_FIELDS each). */
const SPAN = {
  firstVertex: 0, count: 1, material: 2, assetSet: 3, assetSource: 4, assetKey: 5, materialVariant: 6,
  hasCarPaint: 7, paint1: 8, paint2: 9, instanceFlags: 10, materialFlags: 11, depthDecal: 12, alpha: 13, custom: 14,
} as const;

/* The mirror's place on the PAL screen (render/mirror_pass.c,
 * render/rear_view_mirror.c): 240 lines tall, centred horizontally. */
const MIRROR_X = 0x56;
const MIRROR_WIDTH = 0x94;
const MIRROR_HEIGHT = 0x24;
/* The black TILE behind it, two pixels wider on each side. */
const MIRROR_FRAME_X = 0x54;
const MIRROR_FRAME_WIDTH = 0x98;
const MIRROR_FRAME_HEIGHT = 0x28;

/* One camera's uniforms, copied out of the module for a pass. */
interface View { camera: Float32Array; sky: Float32Array }

type Uniform<T> = { value: T };
interface Span { start: number; count: number; entry: MaterialEntry; vehicle: boolean; blended: boolean }
interface Group { start: number; count: number; material: number }

/** Appends a draw range, extending the previous one when it continues it. */
function addMerged(groups: Group[], start: number, count: number, material: number): void {
  const last = groups[groups.length - 1];
  if (last && last.material === material && last.start + last.count === start) last.count += count;
  else groups.push({ start, count, material });
}
interface MaterialEntry {
  layer: number; // texture array layer, -1 when untextured
  transparent: boolean;
  lastUsed: number; // frame number
}

const vec4 = () => ({ value: new THREE.Vector4() });

export class Renderer {
  private readonly webgl: THREE.WebGLRenderer;
  private readonly scene = new THREE.Scene();
  private readonly shadowScene = new THREE.Scene();
  private readonly camera = new THREE.Camera();
  private readonly geometry = new THREE.BufferGeometry();
  private readonly shadowGeometry = new THREE.BufferGeometry();
  private readonly vertexData: Float32Array;
  private readonly vertexBuffer: THREE.InterleavedBuffer;
  private readonly layerData: Float32Array;
  private readonly layerAttribute: THREE.BufferAttribute;
  private readonly worldMaterials: THREE.RawShaderMaterial[];
  private readonly shadowMaterial: THREE.RawShaderMaterial;
  private readonly atlasUniform = { value: null } as Uniform<THREE.DataArrayTexture | null>;
  private atlas: THREE.DataArrayTexture | null = null;
  /* A new array uploads whole; per-layer updates only after that (three.js
   * would otherwise upload just the queued layers of a fresh array). */
  private atlasFresh = false;
  private readonly freeLayers: number[] = [];
  private frameNumber = 0;
  private readonly entries = new Map<string, MaterialEntry>();
  /* Last entry each material was drawn with, shown while a new page decodes. */
  private readonly shown = new Map<string, MaterialEntry>();
  private readonly untextured: MaterialEntry;
  private readonly shadowTarget: THREE.WebGLRenderTarget;
  private readonly shared = {
    uCameraPosition: vec4(), uViewRow0: vec4(), uViewRow1: vec4(), uViewRow2: vec4(),
    uProjection: vec4(), uLightDirection: vec4(), uAmbient: vec4(), uDiffuse: vec4(),
    uShadowPosition: vec4(), uShadowRow0: vec4(), uShadowRow1: vec4(), uShadowRow2: vec4(),
    uShadowProjection: vec4(),
    uShadowMap: { value: null } as Uniform<THREE.Texture | null>,
    uShadowEnabled: { value: 0 },
    uShadowResolution: { value: 1 },
  };
  /* native_sky.frag.glsl's NativeSkyColors block and panorama. */
  private readonly sky = {
    uSkyTop: vec4(), uSkyMiddle: vec4(), uSkyHorizon: vec4(), uSkyBottom: vec4(),
    uSkyGridOrigin: vec4(), uSkyGridBasis: vec4(), uSkyGridParams: vec4(),
    uPanorama: { value: null } as Uniform<THREE.Texture | null>,
  };
  /* Without a panorama the native backend binds one transparent texel. */
  private readonly noPanorama = Renderer.panorama({ data: new Uint8Array(4), width: 1, height: 1 });
  private skyRevision: number | null = null;
  private paletteHash = 0;
  private page = 0;
  private stale = new Set<string>();
  /* Rear-view mirror: the same materials over its own span groups, drawn
   * from the mirror camera into a small target, then flipped into the panel. */
  private readonly mirrorScene = new THREE.Scene();
  private readonly mirrorGeometry = new THREE.BufferGeometry();
  private readonly compositeScene = new THREE.Scene();
  private readonly compositeUniforms = { uMirror: { value: null } as Uniform<THREE.Texture | null> };
  private mirrorTarget: THREE.WebGLRenderTarget | null = null;
  private mainView: View = { camera: new Float32Array(28), sky: new Float32Array(28) };
  private mirrorView: View = { camera: new Float32Array(28), sky: new Float32Array(28) };
  private mirrorPanelY: number | null = null;
  shadows = true;
  private readonly nameGeometry = nameQuad();
  private readonly nameTags = new Map<number, NameTag>();

  constructor(canvas: HTMLCanvasElement, private readonly rage: Rage) {
    this.webgl = new THREE.WebGLRenderer({
      canvas, antialias: true, powerPreference: 'high-performance',
      // Only the automated check (scripts/e2e.mjs) reads pixels back.
      preserveDrawingBuffer: location.hash === '#e2e',
    });
    this.webgl.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    this.webgl.outputColorSpace = THREE.LinearSRGBColorSpace;
    // Draw in the order the groups are laid out: the native phase order.
    this.webgl.sortObjects = false;

    const floats = rage.packedFloats;
    this.vertexData = new Float32Array(VERTEX_CAPACITY * floats);
    this.vertexBuffer = new THREE.InterleavedBuffer(this.vertexData, floats);
    this.vertexBuffer.setUsage(THREE.DynamicDrawUsage);
    const layout: [string, number, number][] = [
      ['position', 3, 0], ['uv', 2, 3], ['color', 4, 5], ['normal', 3, 9], ['fog', 4, 12],
      ['lighting', 1, 16], ['environmentLight', 3, 17], ['depthBias', 1, 20], ['shadowReception', 1, 21],
    ];
    for (const [name, size, offset] of layout) {
      const attribute = new THREE.InterleavedBufferAttribute(this.vertexBuffer, size, offset);
      this.geometry.setAttribute(name, attribute);
      this.mirrorGeometry.setAttribute(name, attribute);
      if (name === 'position' || name === 'uv') this.shadowGeometry.setAttribute(name, attribute);
    }
    this.layerData = new Float32Array(VERTEX_CAPACITY);
    this.layerAttribute = new THREE.BufferAttribute(this.layerData, 1);
    this.layerAttribute.setUsage(THREE.DynamicDrawUsage);
    for (const geometry of [this.geometry, this.shadowGeometry, this.mirrorGeometry]) {
      geometry.setAttribute('layer', this.layerAttribute);
    }
    for (const geometry of [this.geometry, this.shadowGeometry, this.mirrorGeometry])
      geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(), Infinity);

    const resolution = rage.shadowResolution();
    this.shadowTarget = new THREE.WebGLRenderTarget(resolution, resolution, {
      format: THREE.RedFormat, type: THREE.UnsignedByteType, generateMipmaps: false,
      minFilter: THREE.NearestFilter, magFilter: THREE.NearestFilter,
      depthTexture: new THREE.DepthTexture(resolution, resolution, THREE.FloatType),
    });
    this.shared.uShadowMap.value = this.shadowTarget.depthTexture;
    this.shared.uShadowResolution.value = resolution;

    this.growAtlas(ATLAS_INITIAL_LAYERS);
    const worldMaterial = (transparent: boolean) => new THREE.RawShaderMaterial({
      glslVersion: THREE.GLSL3,
      vertexShader: worldVertex,
      fragmentShader: worldFragment,
      uniforms: { ...this.shared, uAtlas: this.atlasUniform },
      side: THREE.DoubleSide,
      depthFunc: THREE.LessEqualDepth,
      transparent,
      blending: transparent ? THREE.NormalBlending : THREE.NoBlending,
      depthWrite: !transparent,
    });
    this.worldMaterials = [worldMaterial(false), worldMaterial(true)];
    this.shadowMaterial = new THREE.RawShaderMaterial({
      glslVersion: THREE.GLSL3,
      vertexShader: shadowVertex,
      fragmentShader: shadowFragment,
      uniforms: { ...this.shared, uAtlas: this.atlasUniform },
      side: THREE.DoubleSide,
      blending: THREE.NoBlending,
    });
    this.untextured = { layer: -1, transparent: false, lastUsed: 0 };
    const world = new THREE.Mesh(this.geometry, this.worldMaterials);
    world.frustumCulled = false;
    const casters = new THREE.Mesh(this.shadowGeometry, [this.shadowMaterial]);
    casters.frustumCulled = false;
    this.shadowScene.add(casters);

    const sky = new THREE.Mesh(
      new THREE.BufferGeometry().setAttribute(
        'position', new THREE.Float32BufferAttribute([-1, -1, 0, 3, -1, 0, -1, 3, 0], 3)),
      new THREE.RawShaderMaterial({
        glslVersion: THREE.GLSL3, vertexShader: skyVertex, fragmentShader: skyFragment,
        uniforms: { ...this.shared, ...this.sky }, depthTest: false, depthWrite: false,
      }));
    this.sky.uPanorama.value = this.noPanorama;
    sky.frustumCulled = false;
    this.scene.add(sky, world);

    const mirrorSky = new THREE.Mesh(sky.geometry, sky.material);
    mirrorSky.frustumCulled = false;
    const mirrorWorld = new THREE.Mesh(this.mirrorGeometry, this.worldMaterials);
    mirrorWorld.frustumCulled = false;
    this.mirrorScene.add(mirrorSky, mirrorWorld);
    const composite = new THREE.Mesh(
      new THREE.PlaneGeometry(2, 2),
      new THREE.RawShaderMaterial({
        glslVersion: THREE.GLSL3, vertexShader: mirrorVertex, fragmentShader: mirrorFragment,
        uniforms: this.compositeUniforms, depthTest: false, depthWrite: false,
      }));
    composite.frustumCulled = false;
    this.compositeScene.add(composite);
  }

  /** Recreates the texture array with room for `layers`, keeping its contents. */
  private growAtlas(layers: number) {
    const previous = this.atlas;
    const data = new Uint8Array(layers * ATLAS_LAYER_BYTES);
    const oldLayers = previous ? previous.image.depth : 0;
    if (previous) data.set(previous.image.data as Uint8Array);
    const atlas = new THREE.DataArrayTexture(data, ATLAS_LAYER_WIDTH, ATLAS_LAYER_HEIGHT, layers);
    atlas.format = THREE.RGBAFormat;
    atlas.type = THREE.UnsignedByteType;
    atlas.magFilter = THREE.LinearFilter;
    atlas.minFilter = THREE.LinearFilter;
    atlas.generateMipmaps = false;
    atlas.colorSpace = THREE.NoColorSpace;
    atlas.needsUpdate = true;
    this.atlas = atlas;
    this.atlasFresh = true;
    this.atlasUniform.value = atlas;
    for (let layer = layers - 1; layer >= oldLayers; layer--) this.freeLayers.push(layer);
    previous?.dispose();
  }

  /** A layer for a new material: a free one, a larger array, or the layer of
   *  the material drawn longest ago (never one drawn this frame). */
  private allocateLayer(): number {
    if (!this.freeLayers.length) {
      const layers = this.atlas!.image.depth;
      if (layers < ATLAS_MAX_LAYERS) this.growAtlas(Math.min(ATLAS_MAX_LAYERS, layers * 2));
    }
    const free = this.freeLayers.pop();
    if (free !== undefined) return free;
    let oldest: [string, MaterialEntry] | null = null;
    for (const item of this.entries) {
      if (item[1].layer >= 0 && item[1].lastUsed < this.frameNumber && (!oldest || item[1].lastUsed < oldest[1].lastUsed)) oldest = item;
    }
    if (!oldest) return -1;
    this.entries.delete(oldest[0]);
    for (const [identity, entry] of this.shown) if (entry === oldest[1]) this.shown.delete(identity);
    return oldest[1].layer;
  }

  /** Writes a decoded mip chain into its layer and queues the upload. */
  private writeLayer(layer: number, decoded: DecodedTexture) {
    const atlas = this.atlas!;
    const data = atlas.image.data as Uint8Array;
    const base = layer * ATLAS_LAYER_BYTES;
    decoded.levels.forEach((level, index) => {
      const [ox, oy] = ATLAS_LEVEL_ORIGINS[index];
      for (let y = 0; y < level.height; y++) {
        data.set(level.data.subarray(y * level.width * 4, (y + 1) * level.width * 4),
                 base + ((oy + y) * ATLAS_LAYER_WIDTH + ox) * 4);
      }
    });
    if (!this.atlasFresh) atlas.addLayerUpdate(layer);
    atlas.needsUpdate = true;
  }

  /* The cloud sheet repeats round the turn but never upwards, and is sampled
   * a texel at a time with no mips, as the native sky sampler does. */
  private static panorama(image: TextureLevel): THREE.DataTexture {
    const texture = new THREE.DataTexture(image.data, image.width, image.height, THREE.RGBAFormat);
    texture.wrapS = THREE.RepeatWrapping;
    texture.wrapT = THREE.ClampToEdgeWrapping;
    texture.magFilter = THREE.NearestFilter;
    texture.minFilter = THREE.NearestFilter;
    texture.generateMipmaps = false;
    texture.colorSpace = THREE.NoColorSpace;
    texture.needsUpdate = true;
    return texture;
  }

  /* ModernNativeEnsureSkyTexture: decode again whenever the palette, page or
   * cloud row changes; a failed decode leaves only the gradient. */
  private updatePanorama() {
    const revision = this.rage.skyRevision();
    if (revision === this.skyRevision) return;
    this.skyRevision = revision;
    const decoded = this.rage.decodeSky();
    const previous = this.sky.uPanorama.value;
    this.sky.uPanorama.value = decoded ? Renderer.panorama(decoded) : this.noPanorama;
    if (previous && previous !== this.noPanorama) previous.dispose();
  }

  /* Track model banks decode against the current track texture page, which
   * retail swaps by track section; terrain and course already carry the page
   * in their material variant. */
  private static keyOf(spans: Uint32Array, f: number, page: number): string {
    const set = spans[f + SPAN.assetSet];
    const paged = set === ASSET_TRACK_MODEL_BANK_1 || set === ASSET_TRACK_MODEL_BANK_2;
    return `${Renderer.identityOf(spans, f)}:${spans[f + SPAN.materialVariant]}:${paged ? page : 0}`;
  }

  /** The material regardless of page and variant. */
  private static identityOf(spans: Uint32Array, f: number): string {
    return `${spans[f + SPAN.assetSet]}:${spans[f + SPAN.assetSource]}:${spans[f + SPAN.assetKey]}:` +
           `${spans[f + SPAN.material]}:${spans[f + SPAN.hasCarPaint]}:` +
           `${spans[f + SPAN.paint1]}:${spans[f + SPAN.paint2]}:${spans[f + SPAN.custom]}`;
  }

  private entryFor(spans: Uint32Array, span: number, budget: { decodes: number }): MaterialEntry | null {
    const f = span * SPAN_FIELDS;
    if (spans[f + SPAN.material] === NO_MATERIAL) return this.untextured;
    const key = Renderer.keyOf(spans, f, this.page);
    const identity = Renderer.identityOf(spans, f);
    const existing = this.entries.get(key);
    const palette = (spans[f + SPAN.materialFlags] & MATERIAL_ENV_CLUT) !== 0;
    if (existing && !(palette && this.stale.has(key))) return this.show(identity, existing);
    // Out of budget: keep what this material showed last (the previous page
    // or variant), as retail keeps drawing while it swaps VRAM rows.
    if (budget.decodes <= 0) return existing ?? this.shown.get(identity) ?? null;
    budget.decodes--;
    const decoded = this.rage.decodeTexture(span);
    this.stale.delete(key);
    if (!decoded) {
      this.entries.set(key, this.untextured);
      return this.show(identity, this.untextured);
    }
    if (existing && existing.layer >= 0) {
      // Palette change: redraw the image in place, in the same layer.
      this.writeLayer(existing.layer, decoded);
      existing.transparent = decoded.transparent;
      return this.show(identity, existing);
    }
    const layer = this.allocateLayer();
    if (layer < 0) return this.show(identity, this.untextured);
    this.writeLayer(layer, decoded);
    const entry: MaterialEntry = { layer, transparent: decoded.transparent, lastUsed: this.frameNumber };
    this.entries.set(key, entry);
    return this.show(identity, entry);
  }

  private show(identity: string, entry: MaterialEntry): MaterialEntry {
    entry.lastUsed = this.frameNumber;
    this.shown.set(identity, entry);
    return entry;
  }

  /** Uploads the frame the bridge just built with `vertexCount` vertices. */
  update(vertexCount: number) {
    this.frameNumber++;
    this.page = this.rage.texturePage();
    const hash = this.rage.paletteHash();
    if (hash !== this.paletteHash) {
      this.paletteHash = hash;
      this.stale = new Set(this.entries.keys());
    }
    const mirror = this.rage.mirror();
    const totalVertices = vertexCount + (mirror ? mirror.vertexCount : 0);
    const packed = this.rage.packedVertices(totalVertices);

    // Sort the spans into draw phases, then lay their vertices out in draw
    // order. Every span carries its texture layer, so each run of opaque or
    // blended spans is one draw call, in native span order.
    const { fields: spans, mainSpans } = this.rage.spans();
    const spanCount = spans.length / SPAN_FIELDS;
    const phases: Span[][] = [[], [], [], []];
    const mirrorPhases: Span[][] = [[], [], [], []];
    const budget = { decodes: DECODES_PER_FRAME };
    for (let span = 0; span < spanCount; span++) {
      const f = span * SPAN_FIELDS;
      const entry = this.entryFor(spans, span, budget);
      if (!entry) continue;
      const set = spans[f + SPAN.assetSet];
      const vehicle = set === ASSET_MODEL_BANK || set === ASSET_TRACK_MODEL_BANK_1;
      // A car fading out past the finish is drawn blended and casts no shadow.
      const fading = spans[f + SPAN.alpha] < 255;
      const phase = entry.transparent || fading ? 3 : spans[f + SPAN.depthDecal] ? 1 : vehicle ? 2 : 0;
      (span >= mainSpans ? mirrorPhases : phases)[phase].push({
        start: spans[f + SPAN.firstVertex], count: spans[f + SPAN.count], entry, vehicle: vehicle && !fading, blended: entry.transparent || fading });
    }
    const floats = this.rage.packedFloats;
    let written = 0;
    const layout = (list: Span[][], groups: Group[], casters: Group[] | null) => {
      for (const phase of list) {
        for (const span of phase) {
          this.vertexData.set(packed.subarray(span.start * floats, (span.start + span.count) * floats), written * floats);
          this.layerData.fill(span.entry.layer, written, written + span.count);
          addMerged(groups, written, span.count, span.blended ? TRANSPARENT : OPAQUE);
          if (casters && span.vehicle) addMerged(casters, written, span.count, 0);
          written += span.count;
        }
      }
    };
    const mainGroups: Group[] = [];
    const casterGroups: Group[] = [];
    const mirrorGroups: Group[] = [];
    layout(phases, mainGroups, casterGroups);
    const mainVertices = written;
    layout(mirrorPhases, mirrorGroups, null);
    this.vertexBuffer.clearUpdateRanges();
    this.vertexBuffer.addUpdateRange(0, written * floats);
    this.vertexBuffer.needsUpdate = true;
    this.layerAttribute.clearUpdateRanges();
    this.layerAttribute.addUpdateRange(0, written);
    this.layerAttribute.needsUpdate = true;

    this.geometry.clearGroups();
    for (const g of mainGroups) this.geometry.addGroup(g.start, g.count, g.material);
    this.shadowGeometry.clearGroups();
    for (const g of casterGroups) this.shadowGeometry.addGroup(g.start, g.count, g.material);
    this.mirrorGeometry.clearGroups();
    for (const g of mirrorGroups) this.mirrorGeometry.addGroup(g.start, g.count, g.material);
    this.geometry.setDrawRange(0, mainVertices);
    this.shadowGeometry.setDrawRange(0, mainVertices);
    this.mirrorGeometry.setDrawRange(0, written);

    this.mainView.camera.set(this.rage.camera());
    this.mainView.sky.set(this.rage.sky());
    this.mirrorPanelY = mirror ? mirror.panelY : null;
    if (mirror) {
      this.mirrorView.camera.set(this.rage.mirrorCamera());
      this.mirrorView.sky.set(this.rage.mirrorSky());
    }
    const s = this.shared;
    const l = this.rage.light();
    s.uLightDirection.value.fromArray(l, 0);
    s.uAmbient.value.fromArray(l, 4);
    s.uDiffuse.value.fromArray(l, 8);
    this.updatePanorama();
    const shadow = this.shadows ? this.rage.shadow() : null;
    s.uShadowEnabled.value = shadow ? 1 : 0;
    if (shadow) {
      s.uShadowPosition.value.fromArray(shadow, 0);
      s.uShadowRow0.value.fromArray(shadow, 4);
      s.uShadowRow1.value.fromArray(shadow, 8);
      s.uShadowRow2.value.fromArray(shadow, 12);
      s.uShadowProjection.value.fromArray(shadow, 16);
    }
  }

  resize(width: number, height: number) {
    this.webgl.setSize(width, height, false);
  }

  get aspect(): number {
    const canvas = this.webgl.domElement;
    return canvas.clientWidth / Math.max(1, canvas.clientHeight);
  }

  /* Points the shared camera and sky uniforms at one view; `height` is the
   * target's height in pixels, which the sky's screen-space grid scales by. */
  private applyView(view: View, height: number) {
    const s = this.shared;
    const c = view.camera;
    s.uCameraPosition.value.fromArray(c, 0);
    s.uViewRow0.value.fromArray(c, 4);
    s.uViewRow1.value.fromArray(c, 8);
    s.uViewRow2.value.fromArray(c, 12);
    s.uProjection.value.fromArray(c, 16);
    const k = this.sky;
    const sky = view.sky;
    k.uSkyTop.value.fromArray(sky, 0);
    k.uSkyMiddle.value.fromArray(sky, 4);
    k.uSkyHorizon.value.fromArray(sky, 8);
    k.uSkyBottom.value.fromArray(sky, 12);
    k.uSkyGridOrigin.value.fromArray(sky, 16);
    k.uSkyGridBasis.value.fromArray(sky, 20);
    k.uSkyGridParams.value.fromArray(sky, 24);
    k.uSkyGridParams.value.w = height;
  }

  /** Names above the cars close to the one being followed. Drawn with the
   *  world, so the depth test hides a plate behind nearer geometry. */
  setNameplates(plates: readonly Nameplate[]): void {
    const live = new Set<number>();
    for (const plate of plates) {
      live.add(plate.seat);
      let tag = this.nameTags.get(plate.seat);
      if (!tag) {
        tag = this.createNameTag();
        this.nameTags.set(plate.seat, tag);
      }
      if (tag.name !== plate.name || tag.texture === null) {
        tag.texture?.dispose();
        tag.texture = paintName(plate.name);
        tag.material.uniforms.uName.value = tag.texture;
        tag.name = plate.name;
      }
      tag.material.uniforms.uAnchor.value.set(plate.anchor[0], plate.anchor[1], plate.anchor[2]);
      const image = tag.texture.image as HTMLCanvasElement;
      const height = plate.length * NAMEPLATE_HEIGHT;
      tag.material.uniforms.uSize.value.set(height * (image.width / image.height), height);
      tag.material.uniforms.uAlpha.value = plate.alpha;
      tag.mesh.position.set(plate.anchor[0], plate.anchor[1], plate.anchor[2]);
      tag.mesh.visible = true;
    }
    for (const [seat, tag] of this.nameTags) if (!live.has(seat)) tag.mesh.visible = false;
  }

  private createNameTag(): NameTag {
    const material = new THREE.RawShaderMaterial({
      glslVersion: THREE.GLSL3,
      vertexShader: nameplateVertex,
      fragmentShader: nameplateFragment,
      uniforms: {
        uCameraPosition: this.shared.uCameraPosition,
        uViewRow0: this.shared.uViewRow0,
        uViewRow1: this.shared.uViewRow1,
        uViewRow2: this.shared.uViewRow2,
        uProjection: this.shared.uProjection,
        uAnchor: { value: new THREE.Vector3() },
        uSize: { value: new THREE.Vector2(1, 1) },
        uAlpha: { value: 1 },
        uName: { value: null },
      },
      transparent: true,
      depthTest: true,
      depthWrite: false,
      depthFunc: THREE.LessEqualDepth,
      side: THREE.DoubleSide,
      blending: THREE.NormalBlending,
    });
    const mesh = new THREE.Mesh(this.nameGeometry, material);
    mesh.frustumCulled = false;
    mesh.renderOrder = 2;
    mesh.visible = false;
    this.scene.add(mesh);
    return { mesh, material, texture: null, name: '' };
  }

  render() {
    if (this.shared.uShadowEnabled.value) {
      this.webgl.setRenderTarget(this.shadowTarget);
      this.webgl.clear(true, true, false);
      this.webgl.render(this.shadowScene, this.camera);
      this.webgl.setRenderTarget(null);
    }
    // gl_FragCoord is in drawing-buffer pixels.
    this.applyView(this.mainView, this.webgl.getContext().drawingBufferHeight);
    this.webgl.render(this.scene, this.camera);
    this.atlasFresh = false; // uploaded whole by the pass above
    if (this.mirrorPanelY !== null) this.renderMirror(this.mirrorPanelY);
  }

  /* modern_renderer.c: the mirror view renders into its own 148x36 target
   * (scaled like the 240-line screen), then ModernCompositeNativeMirror
   * blits it flipped into the panel, clipped to the screen while it slides
   * in from above. The black frame TILE of rear_view_mirror.c goes first. */
  private renderMirror(panelY: number) {
    const gl = this.webgl.getContext();
    const width = gl.drawingBufferWidth, height = gl.drawingBufferHeight;
    const scale = height / PAL_HEIGHT;
    const targetWidth = Math.max(2, Math.round(MIRROR_WIDTH * scale) & ~1);
    const targetHeight = Math.max(2, Math.round(MIRROR_HEIGHT * scale) & ~1);
    if (!this.mirrorTarget || this.mirrorTarget.width !== targetWidth || this.mirrorTarget.height !== targetHeight) {
      this.mirrorTarget?.dispose();
      this.mirrorTarget = new THREE.WebGLRenderTarget(targetWidth, targetHeight, {
        samples: 4, generateMipmaps: false, minFilter: THREE.LinearFilter, magFilter: THREE.LinearFilter,
      });
      this.compositeUniforms.uMirror.value = this.mirrorTarget.texture;
    }
    this.applyView(this.mirrorView, targetHeight);
    this.webgl.setRenderTarget(this.mirrorTarget);
    this.webgl.render(this.mirrorScene, this.camera);
    this.webgl.setRenderTarget(null);

    // Screen rectangles in drawing-buffer pixels (bottom-left origin), from
    // PAL coordinates on the centred 240-line screen.
    const ratio = this.webgl.getPixelRatio();
    const rect = (x: number, top: number, w: number, h: number): [number, number, number, number] => {
      const left = width * 0.5 + (x - PAL_WIDTH / 2) * scale;
      return [left / ratio, (height - (top + h) * scale) / ratio, (w * scale) / ratio, (h * scale) / ratio];
    };
    const autoClear = this.webgl.autoClear;
    this.webgl.autoClear = false;
    this.webgl.setScissorTest(true);
    this.webgl.setScissor(...rect(MIRROR_FRAME_X, panelY - 2, MIRROR_FRAME_WIDTH, MIRROR_FRAME_HEIGHT));
    this.webgl.setClearColor(0x000000, 1);
    this.webgl.clear(true, false, false);
    const panel = rect(MIRROR_X, panelY, MIRROR_WIDTH, MIRROR_HEIGHT);
    this.webgl.setScissor(...panel);
    this.webgl.setViewport(...panel);
    this.webgl.render(this.compositeScene, this.camera);
    this.webgl.setScissorTest(false);
    const size = this.webgl.getSize(new THREE.Vector2());
    this.webgl.setViewport(0, 0, size.x, size.y);
    this.webgl.autoClear = autoClear;
  }
}

interface NameTag {
  mesh: THREE.Mesh;
  material: THREE.RawShaderMaterial;
  texture: THREE.CanvasTexture | null;
  name: string;
}

function nameQuad(): THREE.BufferGeometry {
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('corner', new THREE.Float32BufferAttribute([-1, 0, 1, 0, 1, 1, -1, 1], 2));
  geometry.setIndex([0, 1, 2, 0, 2, 3]);
  geometry.boundingSphere = new THREE.Sphere(new THREE.Vector3(), Infinity);
  return geometry;
}

function paintName(name: string): THREE.CanvasTexture {
  const canvas = document.createElement('canvas');
  const ctx = canvas.getContext('2d');
  if (!ctx) throw new Error('name canvas unavailable');
  const font = '700 32px "Helvetica Neue", Arial, sans-serif';
  ctx.font = font;
  canvas.width = Math.max(4, Math.ceil(ctx.measureText(name).width) + 24);
  canvas.height = 48;
  ctx.font = font;
  ctx.textBaseline = 'middle';
  ctx.lineJoin = 'round';
  ctx.miterLimit = 2;
  ctx.lineWidth = 6;
  ctx.strokeStyle = 'rgba(0, 0, 0, 0.9)';
  ctx.fillStyle = '#ffffff';
  ctx.strokeText(name, 12, 26);
  ctx.fillText(name, 12, 26);
  const texture = new THREE.CanvasTexture(canvas);
  texture.colorSpace = THREE.NoColorSpace;
  texture.generateMipmaps = false;
  texture.minFilter = THREE.LinearFilter;
  texture.magFilter = THREE.LinearFilter;
  texture.needsUpdate = true;
  return texture;
}
