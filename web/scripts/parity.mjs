// WebAssembly half of the physics parity check (see web/wasm/parity_main.c):
//   node web/scripts/parity.mjs <Track 01 BIN> [ticks]
import { readFileSync } from 'node:fs';
import createRageModule from '../../build/web/rage-web.mjs';

const [binPath, ticksArg] = process.argv.slice(2);
const ticks = Number(ticksArg ?? 1000);
const rage = await createRageModule();
const call = (name, ret, types, args) => rage.ccall(name, ret, types, args);
rage.FS.mkdir('/disc');
rage.FS.writeFile('/disc/track01.bin', readFileSync(binPath));
if (!call('rw_load_disc', 'number', ['string'], ['/disc/track01.bin']) ||
    !call('rw_start_race', 'number', Array(7).fill('number'), [0, 0, 9, 0, 0, 3, 1])) process.exit(1);

// Keep in step with ScriptedInput in web/wasm/parity_main.c.
const scripted = (tick) => {
  const left = tick >= 300 && tick < 340 ? 1 : 0;
  const right = (tick >= 400 && tick < 460) || (tick >= 700 && tick < 720) ? 1 : 0;
  const brake = tick >= 520 && tick < 545 ? 256 : 0;
  return [left, right, brake ? 0 : 256, brake];
};
const lines = [];
for (let tick = 1; tick <= ticks; tick++) {
  const [left, right, throttle, brake] = scripted(tick);
  call('rw_set_input', null, Array(6).fill('number'), [left, right, throttle, brake, 0, 0]);
  if (call('rw_tick', 'number', [], []) < 0) process.exit(1);
  if (tick % 10 === 0) {
    const h = new Int32Array(rage.HEAPU8.buffer, call('rw_hud', 'number', [], []), 16);
    lines.push(`${tick} phase=${h[0]} lap=${h[2]} x=${h[12]} y=${h[13]} z=${h[14]} yaw=${h[15]} ` +
               `speed=${h[7]} gear=${h[8]} time=${h[6]}`);
  }
}
process.stdout.write(lines.join('\n') + '\n');
