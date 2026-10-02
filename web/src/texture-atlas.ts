// Every material the frame draws lives in one layer of a texture array, with
// its mip chain packed beside it (shaders.ts ATLAS_LEVEL_ORIGINS). The array
// grows by doubling; once at its limit, the material drawn longest ago gives
// up its layer. Decoding is spread over frames: a material still waiting
// keeps showing what it showed last, as retail keeps drawing while it swaps
// VRAM rows.
import * as THREE from 'three';
import {
  ASSET_TRACK_MODEL_BANK_1, ASSET_TRACK_MODEL_BANK_2, MATERIAL_ENV_CLUT, NO_MATERIAL, Rage, SPAN_FIELDS, type DecodedTexture,
} from './rage';
import { ATLAS_LAYER_HEIGHT, ATLAS_LAYER_WIDTH, ATLAS_LEVEL_ORIGINS } from './shaders';

const INITIAL_LAYERS = 64;
const MAX_LAYERS = 512;
const LAYER_BYTES = ATLAS_LAYER_WIDTH * ATLAS_LAYER_HEIGHT * 4;
/* Decoding is the expensive part of a palette change; spread it out. */
const DECODES_PER_FRAME = 48;

/* Field offsets within one span of Rage.spans() (SPAN_FIELDS each). */
export const SPAN = {
  firstVertex: 0, count: 1, material: 2, assetSet: 3, assetSource: 4, assetKey: 5, materialVariant: 6,
  hasCarPaint: 7, paint1: 8, paint2: 9, instanceFlags: 10, materialFlags: 11, depthDecal: 12, alpha: 13, custom: 14,
} as const;

export interface MaterialEntry {
  layer: number; // texture array layer, -1 when untextured
  transparent: boolean;
  lastUsed: number; // frame number
}

export class TextureAtlas {
  /** The sampler uniform; its texture changes when the array grows. */
  readonly uniform: { value: THREE.DataArrayTexture | null } = { value: null };
  private atlas!: THREE.DataArrayTexture;
  /* A new array uploads whole; per-layer updates only after that (three.js
   * would otherwise upload just the queued layers of a fresh array). */
  private fresh = false;
  private readonly freeLayers: number[] = [];
  private readonly entries = new Map<string, MaterialEntry>();
  /* Last entry each material was drawn with, shown while a new page decodes. */
  private readonly shown = new Map<string, MaterialEntry>();
  private readonly untextured: MaterialEntry = { layer: -1, transparent: false, lastUsed: 0 };
  private stale = new Set<string>();
  private frameNumber = 0;
  private page = 0;
  private paletteHash = 0;
  private decodes = 0;

  constructor(private readonly rage: Rage) {
    this.grow(INITIAL_LAYERS);
  }

  /** Starts a frame: the track texture page and palette it was built with. */
  beginFrame(): void {
    this.frameNumber++;
    this.decodes = DECODES_PER_FRAME;
    this.page = this.rage.texturePage();
    const hash = this.rage.paletteHash();
    if (hash !== this.paletteHash) {
      this.paletteHash = hash;
      this.stale = new Set(this.entries.keys());
    }
  }

  /** The array has been uploaded whole; later changes upload per layer. */
  uploaded(): void {
    this.fresh = false;
  }

  /** The layer a span draws with, or null when it has nothing to show yet. */
  entryFor(spans: Uint32Array, span: number): MaterialEntry | null {
    const f = span * SPAN_FIELDS;
    if (spans[f + SPAN.material] === NO_MATERIAL) return this.untextured;
    const key = TextureAtlas.keyOf(spans, f, this.page);
    const identity = TextureAtlas.identityOf(spans, f);
    const existing = this.entries.get(key);
    const palette = (spans[f + SPAN.materialFlags] & MATERIAL_ENV_CLUT) !== 0;
    if (existing && !(palette && this.stale.has(key))) return this.show(identity, existing);
    // Out of budget: keep what this material showed last (the previous page
    // or variant).
    if (this.decodes <= 0) return existing ?? this.shown.get(identity) ?? null;
    this.decodes--;
    const decoded = this.rage.decodeTexture(span);
    this.stale.delete(key);
    if (!decoded) {
      this.entries.set(key, this.untextured);
      return this.show(identity, this.untextured);
    }
    if (existing && existing.layer >= 0) {
      // Palette change: redraw the image in place, in the same layer.
      this.write(existing.layer, decoded);
      existing.transparent = decoded.transparent;
      return this.show(identity, existing);
    }
    const layer = this.allocate();
    if (layer < 0) return this.show(identity, this.untextured);
    this.write(layer, decoded);
    const entry: MaterialEntry = { layer, transparent: decoded.transparent, lastUsed: this.frameNumber };
    this.entries.set(key, entry);
    return this.show(identity, entry);
  }

  private show(identity: string, entry: MaterialEntry): MaterialEntry {
    entry.lastUsed = this.frameNumber;
    this.shown.set(identity, entry);
    return entry;
  }

  /* Track model banks decode against the current track texture page, which
   * retail swaps by track section; terrain and course already carry the page
   * in their material variant. */
  private static keyOf(spans: Uint32Array, f: number, page: number): string {
    const set = spans[f + SPAN.assetSet];
    const paged = set === ASSET_TRACK_MODEL_BANK_1 || set === ASSET_TRACK_MODEL_BANK_2;
    return `${TextureAtlas.identityOf(spans, f)}:${spans[f + SPAN.materialVariant]}:${paged ? page : 0}`;
  }

  /** The material regardless of page and variant. */
  private static identityOf(spans: Uint32Array, f: number): string {
    return `${spans[f + SPAN.assetSet]}:${spans[f + SPAN.assetSource]}:${spans[f + SPAN.assetKey]}:` +
           `${spans[f + SPAN.material]}:${spans[f + SPAN.hasCarPaint]}:` +
           `${spans[f + SPAN.paint1]}:${spans[f + SPAN.paint2]}:${spans[f + SPAN.custom]}`;
  }

  /** Recreates the texture array with room for `layers`, keeping its contents. */
  private grow(layers: number) {
    const previous = this.uniform.value;
    const data = new Uint8Array(layers * LAYER_BYTES);
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
    this.fresh = true;
    this.uniform.value = atlas;
    for (let layer = layers - 1; layer >= oldLayers; layer--) this.freeLayers.push(layer);
    previous?.dispose();
  }

  /** A layer for a new material: a free one, a larger array, or the layer of
   *  the material drawn longest ago (never one drawn this frame). */
  private allocate(): number {
    if (!this.freeLayers.length) {
      const layers = this.atlas.image.depth;
      if (layers < MAX_LAYERS) this.grow(Math.min(MAX_LAYERS, layers * 2));
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
  private write(layer: number, decoded: DecodedTexture) {
    const data = this.atlas.image.data as Uint8Array;
    const base = layer * LAYER_BYTES;
    decoded.levels.forEach((level, index) => {
      const [ox, oy] = ATLAS_LEVEL_ORIGINS[index];
      for (let y = 0; y < level.height; y++) {
        data.set(level.data.subarray(y * level.width * 4, (y + 1) * level.width * 4),
                 base + ((oy + y) * ATLAS_LAYER_WIDTH + ox) * 4);
      }
    });
    if (!this.fresh) this.atlas.addLayerUpdate(layer);
    this.atlas.needsUpdate = true;
  }
}
