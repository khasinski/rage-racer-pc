// Selection rules for the names drawn above other players' cars.
import assert from 'node:assert/strict';
import { NAMEPLATE_FADE, NAMEPLATE_LENGTHS, selectNameplates } from '../src/nameplates.ts';

const car = (seat, origin, length = 200, name = `P${seat}`) => ({
  seat, name, origin, anchor: [origin[0], origin[1] + 80, origin[2]], length,
});

const solid = selectNameplates([car(0, [0, 0, 0]), car(1, [400, 0, 0])], 0);
assert.equal(solid.length, 1);
assert.equal(solid[0].seat, 1);
assert.equal(solid[0].alpha, 1);
assert.deepEqual(solid[0].anchor, [400, 80, 0]);

const far = 200 * NAMEPLATE_LENGTHS;
assert.deepEqual(selectNameplates([car(0, [0, 0, 0]), car(1, [far, 0, 0])], 0), []);

const range = 200 * NAMEPLATE_LENGTHS;
const distance = range - range * NAMEPLATE_FADE / 2;
const faded = selectNameplates([car(0, [0, 0, 0]), car(1, [distance, 0, 0])], 0);
assert.equal(faded.length, 1);
assert.ok(Math.abs(faded[0].alpha - 0.5) < 1e-9);

const hidden = { seat: 0, name: 'me', origin: [0, 0, 0], anchor: null, length: 0 };
const undrawn = { ...car(2, [100, 0, 0]), anchor: null, length: 0 };
assert.deepEqual(selectNameplates([hidden, car(1, [300, 0, 0]), undrawn], 0).map((plate) => plate.seat), [1]);

assert.deepEqual(selectNameplates([car(0, [0, 0, 0]), { ...car(1, [100, 0, 0]), name: '  ' }], 0), []);
assert.deepEqual(selectNameplates([car(1, [100, 0, 0])], 0), []);

console.log('nameplates ok');
