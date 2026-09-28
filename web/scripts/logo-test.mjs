// The logo editor's pure operations (src/logo.ts): storage form, geometry,
// filling and colour reduction.   node scripts/logo-test.mjs
import { LOGO_BYTES, LOGO_SIZE } from '../shared/protocol.ts';
import {
  blankLogo, cloneLogo, decodeLogo, encodeLogo, fill, flipHorizontal, flipVertical, fromBase64, isEmpty, line, quantize,
  rotateClockwise, rotateCounterClockwise, rgb15, sameLogo, shift, stamp, toBase64,
} from '../src/logo.ts';
import { checks } from './lib/harness.mjs';

const { check, report } = checks();
const random = () => {
  const logo = blankLogo();
  logo.pixels = Uint8Array.from({ length: LOGO_SIZE * LOGO_SIZE }, (_, i) => (i * 7 + (i >> 3) * 3) % 16);
  return logo;
};

const logo = random();
const bytes = encodeLogo(logo);
check(bytes.length === LOGO_BYTES, 'a stored logo is 2080 bytes');
check(sameLogo(decodeLogo(bytes), logo), 'storing and reading a logo changes nothing');
check(sameLogo(decodeLogo(fromBase64(toBase64(bytes))), logo), 'base64 round trip');
check(decodeLogo(bytes.slice(1)) === null, 'a short logo is refused');
check(bytes[0] === ((logo.pixels[0] & 15) | (logo.pixels[1] << 4)), 'the left pixel is the low nibble');

const spun = cloneLogo(logo);
for (let i = 0; i < 4; i++) rotateClockwise(spun);
check(sameLogo(spun, logo), 'four turns are a full turn');
const there = cloneLogo(logo); rotateClockwise(there); rotateCounterClockwise(there);
check(sameLogo(there, logo), 'clockwise then counter-clockwise is nothing');
const mirrored = cloneLogo(logo); flipHorizontal(mirrored); flipHorizontal(mirrored);
check(sameLogo(mirrored, logo), 'flipping twice is nothing');
const flipped = cloneLogo(logo); flipVertical(flipped);
check(flipped.pixels[0] === logo.pixels[63 * 64] && flipped.pixels[63 * 64] === logo.pixels[0], 'a vertical flip swaps top and bottom');
const moved = cloneLogo(logo); shift(moved, 5, -3); shift(moved, -5, 3);
check(sameLogo(moved, logo), 'shifting back and forth wraps round');
const cw = cloneLogo(logo); rotateClockwise(cw);
check(cw.pixels[0 * 64 + 63] === logo.pixels[0] && cw.pixels[0] === logo.pixels[63 * 64], 'a clockwise turn moves the top left to the top right');

const canvas = blankLogo();
stamp(canvas, 10, 10, 3, 5);
check(canvas.pixels.filter((v) => v === 5).length === 9, 'a 3-pixel brush marks nine pixels');
line(canvas, 0, 0, 63, 63, 1, 2);
check(canvas.pixels[0] === 2 && canvas.pixels[63 * 64 + 63] === 2, 'a line reaches both ends');
const empty = blankLogo();
fill(empty, 3, 3, 4);
check(empty.pixels.every((v) => v === 4), 'filling an empty picture covers it');
const boxed = blankLogo();
line(boxed, 0, 32, 63, 32, 1, 1);
fill(boxed, 5, 5, 6);
check(boxed.pixels[5 * 64 + 5] === 6 && boxed.pixels[50 * 64 + 5] === 0 && isEmpty(blankLogo()), 'a fill stops at a line');

const rgba = new Uint8ClampedArray(64 * 64 * 4);
for (let i = 0; i < 64 * 64; i++) {
  const left = (i % 64) < 32;
  rgba.set(left ? [255, 0, 0, 255] : [0, 0, 255, 255], i * 4);
}
rgba.fill(0, 0, 4 * 64 * 4); // a transparent first four rows
const small = quantize(rgba);
check(small.pixels[0] === 0 && small.pixels[10 * 64 + 5] !== small.pixels[10 * 64 + 60], 'a two-colour picture keeps two colours and its transparency');
check(small.palette[small.pixels[10 * 64 + 5]] === rgb15(248, 0, 0) && small.palette[small.pixels[10 * 64 + 60]] === rgb15(0, 0, 248), 'the colours are found');
check(small.palette.length === 16 && small.palette.every((c, i) => i === 0 || c !== 0), 'no palette entry is a bare zero (that would be transparent)');
report('logo ok');
