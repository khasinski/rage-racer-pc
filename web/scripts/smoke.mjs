// Headless check of the WebAssembly build against a real disc:
//   node web/scripts/smoke.mjs <Track 01 BIN> [ticks]
// Loads the disc through the same bridge the browser uses, starts a race with
// the retail AI field, holds full throttle and prints the local car's state.
import { readFileSync } from 'node:fs';
import createRageModule from '../../build/web/rage-web.mjs';

const [binPath, ticksArg] = process.argv.slice(2);
if (!binPath) { console.error('usage: smoke.mjs <Track 01 BIN> [ticks]'); process.exit(2); }
const ticks = Number(ticksArg ?? 600);

const rage = await createRageModule();
rage.FS.mkdir('/disc');
rage.FS.writeFile('/disc/track01.bin', readFileSync(binPath));
const call = (name, ret, types, args) => rage.ccall(name, ret, types, args);

if (!call('rw_load_disc', 'number', ['string'], ['/disc/track01.bin'])) throw new Error('disc did not load');
rage.FS.unlink('/disc/track01.bin');
if (!call('rw_start_race', 'number', Array(7).fill('number'), [0, 0, 0, 0, 0, 3, 1])) throw new Error('race did not start');

let phase = 0;
for (let tick = 1; tick <= ticks; tick++) {
  call('rw_set_input', null, Array(6).fill('number'), [0, 0, 256, 0, 0, 0]);
  phase = call('rw_tick', 'number', [], []);
  if (phase < 0) throw new Error(`tick ${tick} failed`);
  if (tick % 100 === 0) {
    const hud = new Int32Array(rage.HEAPU8.buffer, call('rw_hud', 'number', [], []), 12);
    console.log(`tick=${hud[11]} phase=${hud[0]} lap=${hud[2]}/${hud[3]} place=${hud[4]}/${hud[5]} ` +
                `speed=${hud[7]} gear=${hud[8]} time=${hud[6]}ms`);
  }
}
const vertices = call('rw_build_frame', 'number', ['number'], [16 / 9]);
const spans = call('rw_span_count', 'number', [], []);
if (vertices <= 0 || spans <= 0) throw new Error(`empty frame: ${vertices} vertices, ${spans} spans`);
const rgba = rage._malloc(256 * 256 * 4);
let decoded = 0;
const fields = call('rw_span_fields', 'number', [], []);
const spanData = new Uint32Array(rage.HEAPU8.buffer, call('rw_spans', 'number', [], []), spans * fields);
for (let i = 0; i < spans; i++) {
  if (spanData[i * fields + 2] === 0xffffffff) continue;
  if (call('rw_decode_texture', 'number', ['number', 'number'], [i, rgba])) decoded++;
}
rage._free(rgba);
console.log(`frame: ${vertices} vertices, ${spans} spans, ${decoded} textured spans decoded`);
