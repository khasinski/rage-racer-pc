// Typed wrapper over the WebAssembly bridge (web/wasm/rage_web.c). Every
// heap view is taken fresh: the module's memory can grow, which replaces the
// underlying ArrayBuffer.

interface FsStream { readonly fd: number }
interface EmscriptenFs {
  mkdir(path: string): void;
  open(path: string, flags: string): FsStream;
  write(stream: FsStream, buffer: Uint8Array, offset: number, length: number): number;
  close(stream: FsStream): void;
  unlink(path: string): void;
}
interface EmscriptenModule {
  FS: EmscriptenFs;
  HEAPU8: Uint8Array;
  ccall(name: string, returnType: 'number' | null, argTypes: ('number' | 'string')[], args: (number | string)[]): number;
  _malloc(size: number): number;
  _free(pointer: number): void;
}

export interface RaceOptions {
  classIndex: number; // 0..5
  course: number; // 0..3
  car: number; // 0..31
  manual: boolean;
  reverse: boolean;
  laps: number;
  rivals: boolean;
}

export interface DriverLevels {
  left: boolean;
  right: boolean;
  throttle: number; // 0..256
  brake: number; // 0..256
  shiftUp: boolean;
  shiftDown: boolean;
}

export interface Hud {
  phase: number; // 0 setup, 1 countdown, 2 racing, 3 finished
  countdown: number;
  lap: number;
  laps: number;
  place: number;
  entrants: number;
  timeMs: number;
  speed: number;
  gear: number;
  status: number;
  tick: number;
}

export interface TextureLevel { data: Uint8Array; width: number; height: number }
export interface DecodedTexture { levels: TextureLevel[]; transparent: boolean }

export const PHASE_COUNTDOWN = 1;
export const PHASE_RACING = 2;
export const PHASE_FINISHED = 3;
export const SPAN_FIELDS = 13;
export const TEXTURE_SIZE = 256;
export const NO_MATERIAL = 0xffffffff;
/* rmesh.h RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT: decoded through the
 * environment palette, so it changes with the time of day. */
export const MATERIAL_ENV_CLUT = 1 << 28;

const DISC_PATH = '/disc/data.bin';
const CHUNK = 16 * 1024 * 1024;

export class Rage {
  readonly packedFloats: number;
  private readonly textureScratch: number;

  private constructor(private readonly m: EmscriptenModule) {
    this.m.FS.mkdir('/disc');
    this.packedFloats = this.call('rw_packed_floats');
    this.textureScratch = this.m._malloc(TEXTURE_SIZE * TEXTURE_SIZE * 4);
  }

  static async load(): Promise<Rage> {
    const url = new URL('wasm/rage-web.mjs', document.baseURI).href;
    const factory = (await import(/* @vite-ignore */ url)) as {
      default: () => Promise<EmscriptenModule>;
    };
    return new Rage(await factory.default());
  }

  private call(name: string, types: ('number' | 'string')[] = [], args: (number | string)[] = []): number {
    return this.m.ccall(name, 'number', types, args);
  }

  /** Streams the data track into the module's memory, imports it, then
   *  frees the raw image: the imported race data no longer needs it. */
  async loadDisc(file: File, progress: (fraction: number) => void): Promise<boolean> {
    const stream = this.m.FS.open(DISC_PATH, 'w');
    try {
      for (let offset = 0; offset < file.size; offset += CHUNK) {
        const bytes = new Uint8Array(await file.slice(offset, offset + CHUNK).arrayBuffer());
        this.m.FS.write(stream, bytes, 0, bytes.length);
        progress(Math.min(1, (offset + bytes.length) / file.size));
      }
    } finally {
      this.m.FS.close(stream);
    }
    const ok = this.call('rw_load_disc', ['string'], [DISC_PATH]) === 1;
    this.m.FS.unlink(DISC_PATH);
    return ok;
  }

  startRace(o: RaceOptions): boolean {
    return this.call('rw_start_race', Array(7).fill('number'), [
      o.classIndex, o.course, o.car, +o.manual, +o.reverse, o.laps, +o.rivals]) === 1;
  }

  setInput(d: DriverLevels): void {
    this.m.ccall('rw_set_input', null, Array(6).fill('number'),
      [+d.left, +d.right, d.throttle, d.brake, +d.shiftUp, +d.shiftDown]);
  }

  /** One 50 Hz simulation tick; returns the race phase or -1. */
  tick(): number { return this.call('rw_tick'); }

  /** Builds the frame shown `t` (0..1) between the last two physics steps;
   *  returns the vertex count or -1. */
  buildFrame(aspect: number, t: number): number {
    return this.call('rw_build_frame', ['number', 'number'], [aspect, t]);
  }

  /** Whether the last tick advanced the field physics (a new snapshot). */
  lastTickStepped(): boolean { return this.call('rw_last_tick_stepped') === 1; }

  setDrawDistance(multiplier: number): void {
    this.m.ccall('rw_set_draw_distance', null, ['number'], [multiplier]);
  }

  /** Vehicle shadow camera for the last frame, or null. */
  shadow(): Float32Array | null {
    const pointer = this.call('rw_shadow');
    return pointer ? new Float32Array(this.m.HEAPU8.buffer, pointer, 20) : null;
  }

  shadowResolution(): number { return this.call('rw_shadow_resolution'); }

  packedVertices(count: number): Float32Array {
    const pointer = this.call('rw_pack_vertices');
    return new Float32Array(this.m.HEAPU8.buffer, pointer, count * this.packedFloats);
  }

  spans(): Uint32Array {
    const count = this.call('rw_span_count');
    return new Uint32Array(this.m.HEAPU8.buffer, this.call('rw_spans'), count * SPAN_FIELDS);
  }

  /** position, viewRow0..2, projection, fogColor, fogRange (vec4 each). */
  camera(): Float32Array { return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_camera'), 28); }

  /** light direction, ambient, diffuse, sky top, horizon, bottom. */
  light(): Float32Array { return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_light'), 24); }

  /** Decodes one span's 256x256 material; null when it has no image. */
  /** One span's material as the native backend uploads it: a premultiplied
   *  atlas mip chain (RAGE_TEXTURE_ATLAS_MIP_LEVELS levels), plus whether it
   *  has partial alpha (drawn blended, after everything opaque). */
  decodeTexture(spanIndex: number): DecodedTexture | null {
    const chain = this.call('rw_decode_texture_mips', ['number', 'number'], [spanIndex, this.textureScratch]);
    if (!chain) return null;
    const count = this.call('rw_texture_levels');
    const levels: TextureLevel[] = [];
    for (let level = 0, size = TEXTURE_SIZE; level < count; level++, size >>= 1) {
      const offset = chain + this.call('rw_texture_level_offset', ['number'], [level]);
      levels.push({ data: this.m.HEAPU8.slice(offset, offset + size * size * 4), width: size, height: size });
    }
    const base = levels[0].data;
    let transparent = false;
    for (let i = 3; i < base.length; i += 4) {
      if (base[i] !== 0 && base[i] !== 255) { transparent = true; break; }
    }
    return { levels, transparent };
  }

  paletteHash(): number { return this.call('rw_palette_hash') >>> 0; }

  /** Cycles the three retail chase-camera distances. */
  cycleCamera(): void { this.call('rw_cycle_camera'); }

  /** Per car variant: whether the disc offers it with an automatic gearbox. */
  carAutomatic(): boolean[] {
    const count = this.call('rw_car_variants');
    return Array.from({ length: count }, (_, variant) =>
      this.call('rw_car_automatic', ['number'], [variant]) === 1);
  }

  hud(): Hud {
    const h = new Int32Array(this.m.HEAPU8.buffer, this.call('rw_hud'), 16);
    return { phase: h[0], countdown: h[1], lap: h[2], laps: h[3], place: h[4], entrants: h[5],
             timeMs: h[6], speed: h[7], gear: h[8], status: h[9], tick: h[11] };
  }
}
