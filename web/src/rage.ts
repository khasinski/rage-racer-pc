// Typed wrapper over the WebAssembly bridge (web/wasm/rage_web.c). Every
// heap view is taken fresh: the module's memory can grow, which replaces the
// underlying ArrayBuffer.
import type { Paint } from '../shared/protocol.ts';
import type { PadSample } from './input';

const CLASS_COUNT = 6;

/** How a player's car looks (presentation only): paint (null for the factory
 *  colours), team logo (LOGO_BYTES, or null) and the name on the windscreen. */
export interface SeatLook {
  paint: Paint | null;
  logo: Uint8Array | null;
  name: string;
}

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
  ccall(name: string, returnType: 'number' | 'string' | null, argTypes: ('number' | 'string')[], args: (number | string)[]): number & string;
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
export const SPAN_FIELDS = 15;
const TEXTURE_SIZE = 256;
export const NO_MATERIAL = 0xffffffff;
/* rmesh.h RAGE_RUNTIME_MATERIAL_TERRAIN_ENV_CLUT: decoded through the
 * environment palette, so it changes with the time of day. */
export const MATERIAL_ENV_CLUT = 1 << 28;
/* render_world.h RenderAssetSet: the two ordinary model banks of a track. */
export const ASSET_TRACK_MODEL_BANK_1 = 3;
export const ASSET_TRACK_MODEL_BANK_2 = 4;

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

  /** One pad sample; the bridge applies the desktop button presets. */
  setPad(p: PadSample): void {
    this.m.ccall('rw_set_pad', null, Array(5).fill('number'),
      [p.held, p.stickX, p.rightTrigger, p.leftTrigger, +p.gamepad]);
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

  /** The main view's spans, then the mirror's (`mainSpans` of them are main). */
  spans(): { fields: Uint32Array; mainSpans: number } {
    const main = this.call('rw_span_count');
    const count = main + this.call('rw_mirror_span_count');
    return { fields: new Uint32Array(this.m.HEAPU8.buffer, this.call('rw_spans'), count * SPAN_FIELDS), mainSpans: main };
  }

  /** position, viewRow0..2, projection, fogColor, fogRange (vec4 each). */
  camera(): Float32Array { return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_camera'), 28); }

  /** The rear-view mirror of the last frame, or null when it is not drawn:
   *  its panel top in PAL lines and its vertices after the main view's. */
  mirror(): { panelY: number; firstVertex: number; vertexCount: number } | null {
    const m = new Float32Array(this.m.HEAPU8.buffer, this.call('rw_mirror'), 4);
    return m[0] ? { panelY: m[1], firstVertex: m[2], vertexCount: m[3] } : null;
  }

  /** The mirror camera, laid out as camera(). */
  mirrorCamera(): Float32Array { return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_mirror_camera'), 28); }

  /** The mirror's sky uniform block, laid out as sky(). */
  mirrorSky(): Float32Array { return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_mirror_sky'), 28); }

  /** light direction, ambient, diffuse, sky top, horizon, bottom. */
  light(): Float32Array { return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_light'), 24); }

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

  /** The native sky uniform block for the last frame: top, middle, horizon,
   *  bottom colours, then grid origin, basis and parameters (vec4 each). */
  sky(): Float32Array { return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_sky'), 28); }

  /** Changes whenever the cloud panorama must be decoded again. */
  skyRevision(): number { return this.call('rw_sky_revision') >>> 0; }

  /** The last frame's cloud panorama (RGBA), or null when it has none. */
  decodeSky(): TextureLevel | null {
    const width = this.call('rw_sky_width'), height = this.call('rw_sky_height');
    const scratch = this.m._malloc(width * height * 4);
    try {
      if (!this.call('rw_decode_sky', ['number'], [scratch])) return null;
      return { data: this.m.HEAPU8.slice(scratch, scratch + width * height * 4), width, height };
    } finally {
      this.m._free(scratch);
    }
  }

  paletteHash(): number { return this.call('rw_palette_hash') >>> 0; }

  /** Track texture page (0/1) the last frame was built with. */
  texturePage(): number { return this.call('rw_texture_page'); }

  /** Per car variant: whether the disc offers it with an automatic gearbox. */
  carAutomatic(): boolean[] {
    const count = this.call('rw_car_variants');
    return Array.from({ length: count }, (_, variant) =>
      this.call('rw_car_automatic', ['number'], [variant]) === 1);
  }

  // ---- shared race rules (web/wasm/web_rules.c, also run by the server) ----
  private num(name: string, args: number[]): number {
    return this.call(name, args.map(() => 'number' as const), args);
  }
  private str(name: string, args: number[]): string {
    return this.m.ccall(name, 'string', args.map(() => 'number' as const), args);
  }
  carModels(): number { return this.num('rw_car_models', []); }
  /** The variant a class offers for a model, or -1. */
  classCar(classIndex: number, model: number): number { return this.num('rw_class_car', [classIndex, model]); }
  private carModel(variant: number): number { return this.num('rw_car_model', [variant]); }
  carGrade(variant: number): number { return this.num('rw_car_grade', [variant]); }
  carName(variant: number): string { return this.str('rw_car_name', [this.carModel(variant)]); }
  courseName(course: number): string { return this.str('rw_course_name', [course]); }
  courseAllowed(classIndex: number, course: number): boolean { return this.num('rw_course_allowed', [classIndex, course]) === 1; }
  maxHumans(classIndex: number, course: number, reverse: boolean): number {
    return this.num('rw_max_humans', [classIndex, course, +reverse]);
  }
  /** Boot serial and archive fingerprint of the loaded disc. */
  discId(): string { return this.str('rw_disc_id', []); }

  // ---- paint and the garage preview ---------------------------------------
  /** Every car variant some class offers, by model, then grade. */
  garageVariants(): number[] {
    const variants = new Set<number>();
    for (let classIndex = 0; classIndex < CLASS_COUNT; classIndex++) {
      for (let model = 0; model < this.carModels(); model++) {
        const variant = this.classCar(classIndex, model);
        if (variant >= 0) variants.add(variant);
      }
    }
    return [...variants].sort((a, b) => this.carModel(a) - this.carModel(b) || this.carGrade(a) - this.carGrade(b));
  }

  /** The base car a variant belongs to; paint is kept per model. */
  modelOf(variant: number): number { return this.carModel(variant); }

  /** The number of colours in the paint catalogue. */
  paintCount(): number { return this.call('rw_paint_count'); }

  /** A catalogue colour as CSS, for a swatch. */
  paintSwatch(color: number): string {
    const pointer = this.call('rw_paint_swatch', ['number'], [color]);
    if (!pointer) return 'transparent';
    const [r, g, b] = this.m.HEAPU8.subarray(pointer, pointer + 3);
    return `rgb(${r} ${g} ${b})`;
  }

  /** How a human seat's car looks in the next race prepared. */
  setLook(seat: number, look: SeatLook): void {
    this.withBytes(look.logo, (logo) => this.m.ccall('rw_set_look', null,
      ['number', 'number', 'number', 'number', 'string'],
      [seat, look.paint?.[0] ?? -1, look.paint?.[1] ?? -1, logo, look.name]));
  }

  setShowroomLogo(logo: Uint8Array | null): void {
    this.withBytes(logo, (pointer) => this.call('rw_set_showroom_logo', ['number'], [pointer]));
  }

  setShowroomTag(text: string): void {
    this.m.ccall('rw_set_showroom_tag', null, ['string'], [text]);
  }

  /** Runs `use` with the bytes on the module's heap (0 for none). */
  private withBytes(bytes: Uint8Array | null, use: (pointer: number) => void): void {
    if (!bytes) return use(0);
    const pointer = this.m._malloc(bytes.length);
    this.m.HEAPU8.set(bytes, pointer);
    try { use(pointer); } finally { this.m._free(pointer); }
  }

  /** Bonnet logo triangles found in the last built frame. */
  logoQuads(): number { return this.call('rw_logo_quads'); }

  /** Prepares the garage preview of a car: alone on the grid of a course its
   *  class races. False when no class offers that variant. */
  startShowroom(variant: number): boolean {
    for (let classIndex = 0; classIndex < CLASS_COUNT; classIndex++) {
      const course = [0, 1, 2, 3].find((c) => this.courseAllowed(classIndex, c));
      if (course === undefined || !this.num('rw_car_allowed', [classIndex, variant])) continue;
      return this.startRace({ classIndex, course, car: variant, manual: true, reverse: false, laps: 1, rivals: false });
    }
    return false;
  }

  setShowroomPaint(paint: Paint | null): void {
    this.call('rw_set_showroom_paint', ['number', 'number'], [paint?.[0] ?? -1, paint?.[1] ?? -1]);
  }

  /** Builds the preview seen from `angle` degrees round the car; the vertex
   *  count or -1. */
  buildShowroom(aspect: number, angle: number): number {
    return this.call('rw_build_showroom', ['number', 'number'], [aspect, angle]);
  }

  // ---- networked race -----------------------------------------------------
  /** Prepares the race the server announced; humans in seat order. */
  startNetRace(o: { classIndex: number; course: number; reverse: boolean; laps: number; rivals: boolean },
               humans: { variant: number; manual: boolean; tire?: number }[], localSeat: number): boolean {
    const seats = this.m._malloc(humans.length * 12);
    const words = new Int32Array(this.m.HEAPU8.buffer, seats, humans.length * 3);
    humans.forEach((h, seat) => {
      words[seat * 3] = h.variant;
      words[seat * 3 + 1] = +h.manual;
      words[seat * 3 + 2] = h.tire ?? 0;
    });
    const ok = this.num('rw_start_net_race', [o.classIndex, o.course, +o.reverse, o.laps, +o.rivals,
                                                humans.length, seats, localSeat]) === 1;
    this.m._free(seats);
    return ok;
  }

  private frameBuffer = 0;
  private frameBufferSize = 0;
  /** Hands one server RaceFrame to rw_apply_frame; its result, or null when
   *  the frame does not fit the race. */
  private applyServerFrame(frame: Uint8Array, ackSeq: number, arrivalTick: number): number | null {
    if (frame.length > this.frameBufferSize) {
      this.m._free(this.frameBuffer);
      this.frameBuffer = this.m._malloc(frame.length);
      this.frameBufferSize = frame.length;
    }
    this.m.HEAPU8.set(frame, this.frameBuffer);
    const result = this.num('rw_apply_frame', [this.frameBuffer, frame.length, ackSeq, arrivalTick]);
    return result === -2147483648 ? null : result;
  }

  /** A spectator restores one authoritative RaceFrame from the server. */
  applyFrame(frame: Uint8Array): boolean { return this.applyServerFrame(frame, 0, 0) !== null; }

  /** A player rewinds to a server frame and replays the unconsumed controls;
   *  returns how many ticks late the acknowledged input was used, or null. */
  applyPredicted(frame: Uint8Array, ackSeq: number, arrivalTick: number): number | null {
    return this.applyServerFrame(frame, ackSeq, arrivalTick);
  }

  // ---- spectating ---------------------------------------------------------
  /** The seat the camera and HUD follow. */
  viewSeat(): number { return this.num('rw_view_seat', []); }
  /** Follows another car; false when the seat is empty. */
  setViewSeat(seat: number): boolean { return this.num('rw_set_view_seat', [seat]) === 1; }
  /** Whether a seat's car has left the picture (retired, or finished and removed). */
  seatGone(seat: number): boolean { return this.num('rw_seat_gone', [seat]) === 1; }

  // ---- network diagnostics (scripts/net-check.mjs) -------------------------
  /** Server-frame corrections: frames, own mean/max, others mean/max, mean replayed ticks. */
  netStats(): number[] { return Array.from(new Float32Array(this.m.HEAPU8.buffer, this.call('rw_net_stats'), 6)); }
  resetNetStats(): void { this.call('rw_net_stats_reset'); }
  /** Where a seat's car was drawn in the last frame (x, y, z). */
  presented(seat: number): number[] {
    return Array.from(new Float32Array(this.m.HEAPU8.buffer, this.num('rw_presented', [seat]), 3));
  }

  /** Per seat: roof point x, y, z and the body's longer horizontal side.
   *  The length is 0 when that human car was not drawn. */
  nameplates(): Float32Array {
    const seats = this.num('rw_nameplate_seats', []);
    return new Float32Array(this.m.HEAPU8.buffer, this.call('rw_nameplates'), seats * 4).slice();
  }

  /** Place (0 when out of the race), lap and SimDriverStatus of a seat. */
  standing(seat: number): { place: number; lap: number; status: number } {
    return { place: this.num('rw_seat_place', [seat]), lap: this.num('rw_seat_lap', [seat]),
             status: this.num('rw_seat_status', [seat]) };
  }

  /** Sequence number of the controls rw_take_input just handed out. */
  inputSeq(): number { return this.num('rw_input_seq', []) >>> 0; }
  /** The race tick the latest taken input is predicted for. */
  inputTick(): number { return this.num('rw_input_tick', []) >>> 0; }

  /** The controls to send this tick (8 input words; gear edges consumed). */
  takeInput(): Int32Array {
    return new Int32Array(this.m.HEAPU8.buffer, this.call('rw_take_input'), 8).slice();
  }

  // ---- race audio (web/wasm/web_audio.c) -----------------------------------
  /** Starts or stops rendering the race's 44.1 kHz stereo output. */
  audioEnable(enabled: boolean): void { this.m.ccall('rw_audio_enable', null, ['number'], [+enabled]); }

  /** Interleaved stereo frames rendered by the ticks since the last call. */
  audioTake(): Int16Array {
    const frames = this.call('rw_audio_take');
    if (!frames) return new Int16Array(0);
    return new Int16Array(this.m.HEAPU8.buffer, this.call('rw_audio_data'), frames * 2).slice();
  }

  hud(): Hud {
    const h = new Int32Array(this.m.HEAPU8.buffer, this.call('rw_hud'), 16);
    return { phase: h[0], countdown: h[1], lap: h[2], laps: h[3], place: h[4], entrants: h[5],
             timeMs: h[6], speed: h[7], gear: h[8], status: h[9], tick: h[11] };
  }

  /** The retail tachometer's sprites for the prepared race (web_hud.c). */
  hudAtlas(): TextureLevel | null {
    const pointer = this.call('rw_hud_atlas');
    const width = this.call('rw_hud_atlas_width'), height = this.call('rw_hud_atlas_height');
    return pointer ? { data: this.m.HEAPU8.slice(pointer, pointer + width * height * 4), width, height } : null;
  }

  /** This game frame's tachometer packet fields (web_hud.c layout). */
  tachometer(): Int32Array {
    return new Int32Array(this.m.HEAPU8.buffer, this.call('rw_tachometer'), this.call('rw_tachometer_words')).slice();
  }
}
