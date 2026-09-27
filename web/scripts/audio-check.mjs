// Headless check of the browser race audio (web/wasm/web_audio.c, web_spu.c)
// and CD music lookup (web/src/cdda.ts):
//   node web/scripts/audio-check.mjs "<Rage Racer .cue>" [--wav out.wav]
// Loads the disc into the WebAssembly module, runs the countdown and ten
// seconds of full throttle, and checks that the engine is audible and that
// its pitch follows the rpm; then checks that the race tunes are found on the
// disc's CD audio tracks and hold music.
import { openAsBlob, readFileSync, writeFileSync } from 'node:fs';
import { basename, dirname, join } from 'node:path';
import createRageModule from '../public/wasm/rage-web.mjs';
import { BGM_TRACK_COUNT, bgmCdTrack, cueAudioTracks, readTrackPcm } from '../src/cdda.ts';

const [cuePath, ...rest] = process.argv.slice(2);
const wavPath = rest[0] === '--wav' ? rest[1] : null;
if (!cuePath) {
  console.error('usage: node web/scripts/audio-check.mjs <disc.cue> [--wav out.wav]');
  process.exit(2);
}
const failures = [];
const expect = (ok, message) => { console.log(`${ok ? 'ok  ' : 'FAIL'} ${message}`); if (!ok) failures.push(message); };

// ---- the disc -------------------------------------------------------------
const cue = readFileSync(cuePath, 'utf8');
const names = [...cue.matchAll(/^\s*FILE\s+"([^"]+)"/gim)].map((m) => m[1]);
const files = await Promise.all(names.map(async (name) => {
  const blob = await openAsBlob(join(dirname(cuePath), name));
  return { name, size: blob.size, slice: (a, b) => blob.slice(a, b) };
}));
const dataTrack = join(dirname(cuePath), names[0]);

const rage = await createRageModule();
const call = (name, types = [], args = []) => rage.ccall(name, 'number', types, args);
rage.FS.mkdir('/disc');
rage.FS.writeFile('/disc/data.bin', readFileSync(dataTrack));
expect(call('rw_load_disc', ['string'], ['/disc/data.bin']) === 1, `loads ${basename(dataTrack)}`);
expect(call('rw_start_race', Array(7).fill('number'), [0, 0, 9, 0, 0, 3, 1]) === 1, 'starts a race with the retail field');
call('rw_audio_enable', ['number'], [1]);

// ---- the race -------------------------------------------------------------
const RATE = 44100;
const all = [];
const windows = [];
let pcm = [];
let rpm = [];
const flush = (label) => {
  const samples = Int16Array.from(pcm);
  windows.push({ label, samples, rpm: rpm.reduce((a, b) => a + b, 0) / rpm.length });
  all.push(samples);
  pcm = [];
  rpm = [];
};
for (let tick = 1; tick <= 50 * 14; tick++) {
  const phase = call('rw_hud') && new Int32Array(rage.HEAPU8.buffer, call('rw_hud'), 16)[0];
  const throttle = phase === 2 ? 256 : 0;
  rage.ccall('rw_set_input', null, Array(6).fill('number'), [0, 0, throttle, 0, 0, 0]);
  if (call('rw_tick') < 0) { expect(false, 'race ticks'); break; }
  const frames = call('rw_audio_take');
  const data = new Int16Array(rage.HEAPU8.buffer, call('rw_audio_data'), frames * 2);
  for (let i = 0; i < frames; i++) pcm.push((data[i * 2] + data[i * 2 + 1]) >> 1);
  rpm.push(call('rw_audio_engine_rpm'));
  if (tick % 25 === 0) flush(`${(tick / 50).toFixed(1)}s ${phase === 1 ? 'countdown' : 'racing'}`);
}

const rms = (s) => Math.sqrt(s.reduce((a, v) => a + v * v, 0) / Math.max(1, s.length));
/* Engine pitch: the strongest autocorrelation lag between 1.5 ms and 25 ms
 * (40..660 Hz fundamental), after removing the window's mean. */
function pitch(samples) {
  const n = Math.min(samples.length, 8192);
  const x = new Float64Array(n);
  let mean = 0;
  for (let i = 0; i < n; i++) mean += samples[i];
  mean /= n;
  for (let i = 0; i < n; i++) x[i] = samples[i] - mean;
  let best = 0, bestLag = 0;
  for (let lag = Math.floor(RATE / 660); lag <= Math.floor(RATE / 40); lag++) {
    let sum = 0;
    for (let i = 0; i + lag < n; i++) sum += x[i] * x[i + lag];
    sum /= n - lag;
    if (sum > best) { best = sum; bestLag = lag; }
  }
  return bestLag ? RATE / bestLag : 0;
}
/* Spectral centroid from a coarse DFT of 2048 samples. */
function centroid(samples) {
  const n = 2048;
  let weighted = 0, total = 0;
  for (let k = 1; k < n / 2; k += 2) {
    let re = 0, im = 0;
    for (let i = 0; i < n; i++) {
      const w = 0.5 - 0.5 * Math.cos((2 * Math.PI * i) / n);
      re += samples[i] * w * Math.cos((2 * Math.PI * k * i) / n);
      im -= samples[i] * w * Math.sin((2 * Math.PI * k * i) / n);
    }
    const magnitude = Math.hypot(re, im);
    weighted += magnitude * (k * RATE / n);
    total += magnitude;
  }
  return total ? weighted / total : 0;
}
function correlation(a, b) {
  const ma = a.reduce((x, y) => x + y, 0) / a.length, mb = b.reduce((x, y) => x + y, 0) / b.length;
  let num = 0, da = 0, db = 0;
  for (let i = 0; i < a.length; i++) { num += (a[i] - ma) * (b[i] - mb); da += (a[i] - ma) ** 2; db += (b[i] - mb) ** 2; }
  return num / Math.sqrt(da * db);
}

console.log('window              rpm    rms   pitch(Hz) centroid(Hz)');
for (const w of windows) {
  w.rms = rms(w.samples);
  w.pitch = pitch(w.samples.subarray(w.samples.length - 8192));
  w.centroid = centroid(w.samples.subarray(w.samples.length - 2048));
  console.log(`${w.label.padEnd(18)} ${w.rpm.toFixed(0).padStart(5)} ${w.rms.toFixed(0).padStart(6)} ${w.pitch.toFixed(1).padStart(9)} ${w.centroid.toFixed(0).padStart(10)}`);
}
const countdown = windows.filter((w) => w.label.endsWith('countdown'));
const racing = windows.filter((w) => w.label.endsWith('racing'));
expect(countdown.length > 0 && countdown.every((w) => w.rms > 100), 'countdown is audible (idling engine and announcer)');
expect(racing.length > 10 && racing.every((w) => w.rms > 100), 'racing at full throttle is audible');
const rpms = racing.map((w) => w.rpm);
expect(Math.max(...rpms) - Math.min(...rpms) > 2000, `rpm sweeps (${Math.min(...rpms).toFixed(0)}..${Math.max(...rpms).toFixed(0)})`);
const r = correlation(rpms, racing.map((w) => w.centroid));
expect(r > 0.5, `spectral centroid follows rpm (r=${r.toFixed(2)})`);
const low = racing.reduce((a, w) => (w.rpm < a.rpm ? w : a), racing[0] ?? { rpm: 0, centroid: 0 });
const high = racing.reduce((a, w) => (w.rpm > a.rpm ? w : a), low);
expect(high.centroid > low.centroid * 1.2, `higher rpm sounds higher (${low.centroid.toFixed(0)} Hz at ${low.rpm.toFixed(0)} rpm, ${high.centroid.toFixed(0)} Hz at ${high.rpm.toFixed(0)} rpm)`);

if (wavPath) {
  const total = all.reduce((a, s) => a + s.length, 0);
  const wav = Buffer.alloc(44 + total * 2);
  wav.write('RIFF', 0); wav.writeUInt32LE(36 + total * 2, 4); wav.write('WAVEfmt ', 8);
  wav.writeUInt32LE(16, 16); wav.writeUInt16LE(1, 20); wav.writeUInt16LE(1, 22);
  wav.writeUInt32LE(RATE, 24); wav.writeUInt32LE(RATE * 2, 28); wav.writeUInt16LE(2, 32); wav.writeUInt16LE(16, 34);
  wav.write('data', 36); wav.writeUInt32LE(total * 2, 40);
  let offset = 44;
  for (const s of all) for (const v of s) { wav.writeInt16LE(v, offset); offset += 2; }
  writeFileSync(wavPath, wav);
  console.log(`wrote ${wavPath}`);
}

// ---- the music --------------------------------------------------------------
const tracks = cueAudioTracks(cue, files);
const tunes = Array.from({ length: BGM_TRACK_COUNT }, (_, i) => bgmCdTrack(i));
expect(tunes.every((n) => tracks.has(n)), `all ${BGM_TRACK_COUNT} race tunes found (CD tracks ${tunes.join(', ')})`);
for (const n of tunes.filter((n) => tracks.has(n))) {
  const track = tracks.get(n);
  const second = await readTrackPcm(track, 30 * RATE * 4, RATE * 4);
  const level = rms(second);
  console.log(`track ${String(n).padStart(2)}: ${(track.length / RATE / 4).toFixed(1)} s, rms at 0:30 ${level.toFixed(0)}`);
  expect(level > 500, `track ${n} holds music`);
}

if (failures.length) {
  console.error(`${failures.length} check(s) failed`);
  process.exit(1);
}
console.log('audio check passed');
