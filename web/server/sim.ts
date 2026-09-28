// The authoritative race simulation: the C engine compiled to WebAssembly
// (web/wasm/rage_server.c), shared by every room. See npm run wasm.
import { realpathSync } from 'node:fs';
import { basename, dirname } from 'node:path';
import { INPUT_WORDS } from '../shared/protocol.ts';

interface ServerModule {
  FS: { mkdir(path: string): void; mount(type: unknown, options: { root: string }, path: string): void };
  NODEFS: unknown;
  HEAPU8: Uint8Array;
  ccall(name: string, returnType: 'number' | 'string' | null, argTypes: string[], args: (number | string)[]): number & string;
  _malloc(size: number): number;
}

export const PHASE_FINISHED = 3;
export const STATUS_EMPTY = 0;
export const STATUS_DRIVING = 1;
export const STATUS_FINISHED = 2;
export const STATUS_RETIRED = 3;

interface CarChoice { variant: number; manual: boolean }
interface SeatState { status: number; place: number; timeMs: number; bestLapMs: number; lap: number }

export class Simulation {
  readonly discId: string;
  private readonly frameSize: number;
  private readonly scratch: number;
  private readonly m: ServerModule;

  private constructor(m: ServerModule) {
    this.m = m;
    this.discId = this.m.ccall('rw_disc_id', 'string', [], []);
    this.frameSize = this.num('rs_frame_size');
    this.scratch = this.m._malloc(4 * 64);
  }

  /** Imports the server operator's disc (CUE or Track 01 BIN). */
  static async load(discPath: string): Promise<Simulation> {
    const url = new URL('./wasm/rage-server.mjs', import.meta.url).href;
    const factory = (await import(url)) as { default: () => Promise<ServerModule> };
    const m = await factory.default();
    const real = realpathSync(discPath);
    m.FS.mkdir('/disc');
    m.FS.mount(m.NODEFS, { root: dirname(real) }, '/disc');
    if (m.ccall('rs_load_disc', 'number', ['string'], [`/disc/${basename(real)}`]) !== 1) {
      throw new Error(`${discPath} is not a Rage Racer disc image the server can read`);
    }
    return new Simulation(m);
  }

  private num(name: string, args: number[] = []): number {
    return this.m.ccall(name, 'number', args.map(() => 'number'), args);
  }

  // Shared race rules (web/wasm/web_rules.c), the same code the browser runs.
  carModels(): number { return this.num('rw_car_models'); }
  classCar(classIndex: number, model: number): number { return this.num('rw_class_car', [classIndex, model]); }
  /** The model (the base car) a variant belongs to. */
  carModel(variant: number): number { return this.num('rw_car_model', [variant]); }
  carAllowed(classIndex: number, variant: number): boolean { return this.num('rw_car_allowed', [classIndex, variant]) === 1; }
  carAutomatic(variant: number): boolean { return this.num('rw_car_automatic', [variant]) === 1; }
  courseAllowed(classIndex: number, course: number): boolean { return this.num('rw_course_allowed', [classIndex, course]) === 1; }
  maxHumans(classIndex: number, course: number, reverse: boolean): number {
    return this.num('rw_max_humans', [classIndex, course, +reverse]);
  }

  /** Builds a race; returns its handle, or 0 when the field is invalid. */
  createRace(classIndex: number, course: number, reverse: boolean, laps: number, rivals: boolean,
             cars: CarChoice[]): number {
    const words = new Int32Array(this.m.HEAPU8.buffer, this.scratch, cars.length * 2);
    cars.forEach((car, seat) => { words[seat * 2] = car.variant; words[seat * 2 + 1] = +car.manual; });
    return this.num('rs_create_race', [classIndex, course, +reverse, laps, +rivals, cars.length, this.scratch]);
  }

  start(race: number): boolean { return this.num('rs_start', [race]) === 1; }
  free(race: number): void { this.num('rs_free', [race]); }
  retire(race: number, seat: number): boolean { return this.num('rs_retire', [race, seat]) === 1; }

  /** Latest controls for a human seat; false when the command is invalid. */
  setInput(race: number, seat: number, words: Int32Array): boolean {
    if (words.length !== INPUT_WORDS) return false;
    new Int32Array(this.m.HEAPU8.buffer, this.scratch, INPUT_WORDS).set(words);
    return this.num('rs_set_input', [race, seat, this.scratch]) === 1;
  }

  /** One 50 Hz tick; returns the race phase. */
  tick(race: number): number { return this.num('rs_tick', [race]); }
  simTick(race: number): number { return this.num('rs_sim_tick', [race]) >>> 0; }

  /** Copy of the race's complete RaceFrame wire state. */
  frame(race: number): Uint8Array | null {
    const pointer = this.num('rs_frame', [race]);
    return pointer ? this.m.HEAPU8.slice(pointer, pointer + this.frameSize) : null;
  }

  seat(race: number, seat: number): SeatState {
    return {
      status: this.num('rs_seat_status', [race, seat]),
      place: this.num('rs_seat_place', [race, seat]),
      timeMs: this.num('rs_seat_time', [race, seat]),
      bestLapMs: this.num('rs_seat_best_lap', [race, seat]),
      lap: this.num('rs_seat_lap', [race, seat]),
    };
  }

  /** Time of a completed lap (0-based), or -1. */
  lapTime(race: number, seat: number, lap: number): number { return this.num('rs_seat_lap_time', [race, seat, lap]); }
}
