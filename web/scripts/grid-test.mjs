// Starting-grid checks for online races:  node scripts/grid-test.mjs <disc>
// For every class, course and direction, eight players must start together
// behind the retail player start (before the line), apart from each other and
// from the rivals, and stay put through the countdown.
import { realpathSync } from 'node:fs';
import { basename, dirname } from 'node:path';
import createRaceServerModule from '../server/wasm/rage-server.mjs';

const disc = process.argv[2] && realpathSync(process.argv[2]);
if (!disc) { console.error('usage: grid-test.mjs <disc>'); process.exit(2); }
const m = await createRaceServerModule();
m.FS.mkdir('/disc');
m.FS.mount(m.NODEFS, { root: dirname(disc) }, '/disc');
const call = (name, args = []) => m.ccall(name, 'number', args.map(() => 'number'), args);
if (!m.ccall('rs_load_disc', 'number', ['string'], [`/disc/${basename(disc)}`])) throw new Error('disc');

const PLAYERS = 8;
const failures = [];
const seats = m._malloc(PLAYERS * 8);
let races = 0;
for (let classIndex = 0; classIndex < 6; classIndex++) {
  const variant = call('rw_class_car', [classIndex, 3]); // Esperanza, offered in every class
  for (let course = 0; course < 4; course++) {
    if (!call('rw_course_allowed', [classIndex, course])) continue;
    for (const reverse of [0, 1]) {
      const words = new Int32Array(m.HEAPU8.buffer, seats, PLAYERS * 2);
      for (let seat = 0; seat < PLAYERS; seat++) { words[seat * 2] = variant; words[seat * 2 + 1] = 1; }
      const race = call('rs_create_race', [classIndex, course, reverse, 3, 1, PLAYERS, seats]);
      const where = `class ${classIndex + 1} course ${course} ${reverse ? 'reverse' : 'forward'}`;
      if (!race) { failures.push(`${where}: no race`); continue; }
      races++;
      const field = [];
      for (let seat = 0; seat < 12; seat++) {
        if (!call('rs_seat_status', [race, seat])) continue;
        field.push({ seat, x: call('rs_seat_x', [race, seat]), z: call('rs_seat_z', [race, seat]),
                     progress: call('rs_seat_progress', [race, seat]) });
      }
      const players = field.slice(0, PLAYERS);
      const pole = players[0].progress;
      for (const p of players.slice(1)) {
        if (p.progress > pole + 1200 || p.progress < pole - 6000) failures.push(`${where}: player ${p.seat} is not on the grid (progress ${p.progress - pole})`);
      }
      for (let a = 0; a < field.length; a++) for (let b = a + 1; b < field.length; b++) {
        const d = Math.hypot(field[a].x - field[b].x, field[a].z - field[b].z);
        if (d < 250) failures.push(`${where}: seats ${field[a].seat} and ${field[b].seat} overlap (${Math.round(d)})`);
      }
      call('rs_start', [race]);
      for (let tick = 0; tick < 140; tick++) call('rs_tick', [race]); // countdown, no input
      for (const p of players) {
        const moved = Math.hypot(call('rs_seat_x', [race, p.seat]) - p.x, call('rs_seat_z', [race, p.seat]) - p.z);
        if (moved > 40) failures.push(`${where}: player ${p.seat} moved ${Math.round(moved)} during the countdown`);
      }
      call('rs_free', [race]);
    }
  }
}
if (failures.length) { console.error(failures.join('\n')); process.exit(1); }
console.log(`ok — ${races} course variants, ${PLAYERS} players each`);
