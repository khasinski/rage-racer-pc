// A team logo as the garage editor holds it, and the operations on it. The
// image is LOGO_SIZE squared palette indices; index 0 is transparent and the
// other fifteen entries are PlayStation 15-bit colours, as the game's own
// logo editor keeps them (see shared/protocol.ts for the stored form).
import { LOGO_BYTES, LOGO_COLORS, LOGO_PIXEL_BYTES, LOGO_SIZE } from '../shared/protocol.ts';

export interface Logo {
  pixels: Uint8Array; // LOGO_SIZE * LOGO_SIZE indices, row by row
  palette: number[]; // LOGO_COLORS 15-bit colours; entry 0 is not drawn
}

const BLACK = 0x8000; // opaque black: a plain 0 in a palette means "transparent"
/* The starting palette: white, black, the primaries and a few greys. */
export const DEFAULT_PALETTE = [
  0, 0x7fff, BLACK, 0x001f, 0x02bf, 0x03ff, 0x03e0, 0x7fe0, 0x7e00, 0x7c00, 0x7c1f, 0x5ef7, 0x39ce, 0x1ce7, 0x0d7f, 0x2d6b,
];

export const rgb15 = (r: number, g: number, b: number): number => {
  const word = (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
  return word === 0 ? BLACK : word;
};

/** A 15-bit colour as [r, g, b] (0..255). */
export function channels(color: number): [number, number, number] {
  const expand = (v: number) => (v << 3) | (v >> 2);
  return [expand(color & 31), expand((color >> 5) & 31), expand((color >> 10) & 31)];
}

export const css = (color: number): string => `rgb(${channels(color).join(' ')})`;

export function toHex(color: number): string {
  return `#${channels(color).map((v) => v.toString(16).padStart(2, '0')).join('')}`;
}

export function fromHex(hex: string): number {
  const value = parseInt(hex.replace('#', ''), 16);
  return rgb15((value >> 16) & 255, (value >> 8) & 255, value & 255);
}

export function blankLogo(): Logo {
  return { pixels: new Uint8Array(LOGO_SIZE * LOGO_SIZE), palette: [...DEFAULT_PALETTE] };
}

export const cloneLogo = (logo: Logo): Logo => ({ pixels: logo.pixels.slice(), palette: [...logo.palette] });

export function sameLogo(a: Logo | null, b: Logo | null): boolean {
  if (!a || !b) return a === b;
  return a.pixels.every((v, i) => v === b.pixels[i]) && a.palette.every((v, i) => v === b.palette[i]);
}

/** Whether nothing is drawn. */
export const isEmpty = (logo: Logo): boolean => logo.pixels.every((v) => v === 0);

/** The stored form: 4-bit pixels (left pixel in the low nibble), then the
 *  palette as little-endian words. */
export function encodeLogo(logo: Logo): Uint8Array {
  const bytes = new Uint8Array(LOGO_BYTES);
  for (let i = 0; i < logo.pixels.length; i += 2) bytes[i >> 1] = (logo.pixels[i] & 15) | ((logo.pixels[i + 1] & 15) << 4);
  logo.palette.forEach((color, i) => {
    bytes[LOGO_PIXEL_BYTES + i * 2] = color & 255;
    bytes[LOGO_PIXEL_BYTES + i * 2 + 1] = color >> 8;
  });
  return bytes;
}

export function decodeLogo(bytes: Uint8Array): Logo | null {
  if (bytes.length !== LOGO_BYTES) return null;
  const pixels = new Uint8Array(LOGO_SIZE * LOGO_SIZE);
  for (let i = 0; i < LOGO_PIXEL_BYTES; i++) {
    pixels[i * 2] = bytes[i] & 15;
    pixels[i * 2 + 1] = bytes[i] >> 4;
  }
  const palette = Array.from({ length: LOGO_COLORS }, (_, i) => bytes[LOGO_PIXEL_BYTES + i * 2] | (bytes[LOGO_PIXEL_BYTES + i * 2 + 1] << 8));
  return { pixels, palette };
}

export function toBase64(bytes: Uint8Array): string {
  let text = '';
  for (const byte of bytes) text += String.fromCharCode(byte);
  return btoa(text);
}

export const fromBase64 = (text: string): Uint8Array => Uint8Array.from(atob(text), (c) => c.charCodeAt(0));

/** Decodes a logo as the server sends it. */
export const logoFromBase64 = (text: string | null): Logo | null => (text ? decodeLogo(fromBase64(text)) : null);

// ---- editing --------------------------------------------------------------------

const at = (x: number, y: number) => y * LOGO_SIZE + x;
const inside = (x: number, y: number) => x >= 0 && y >= 0 && x < LOGO_SIZE && y < LOGO_SIZE;

/** A square brush of `size` pixels centred near (x, y). */
export function stamp(logo: Logo, x: number, y: number, size: number, index: number): void {
  const from = -Math.floor((size - 1) / 2);
  for (let dy = from; dy < from + size; dy++) {
    for (let dx = from; dx < from + size; dx++) if (inside(x + dx, y + dy)) logo.pixels[at(x + dx, y + dy)] = index;
  }
}

/** A straight stroke between two points. */
export function line(logo: Logo, x0: number, y0: number, x1: number, y1: number, size: number, index: number): void {
  const steps = Math.max(Math.abs(x1 - x0), Math.abs(y1 - y0), 1);
  for (let i = 0; i <= steps; i++) {
    stamp(logo, Math.round(x0 + ((x1 - x0) * i) / steps), Math.round(y0 + ((y1 - y0) * i) / steps), size, index);
  }
}

/** Paints the connected area of the same index. */
export function fill(logo: Logo, x: number, y: number, index: number): void {
  if (!inside(x, y)) return;
  const target = logo.pixels[at(x, y)];
  if (target === index) return;
  const pending = [at(x, y)];
  while (pending.length) {
    const i = pending.pop()!;
    if (logo.pixels[i] !== target) continue;
    logo.pixels[i] = index;
    const px = i % LOGO_SIZE, py = (i - px) / LOGO_SIZE;
    if (px > 0) pending.push(i - 1);
    if (px < LOGO_SIZE - 1) pending.push(i + 1);
    if (py > 0) pending.push(i - LOGO_SIZE);
    if (py < LOGO_SIZE - 1) pending.push(i + LOGO_SIZE);
  }
}

/** Rebuilds the image from a mapping of each new pixel's source. */
function remap(logo: Logo, source: (x: number, y: number) => [number, number]): void {
  const old = logo.pixels.slice();
  for (let y = 0; y < LOGO_SIZE; y++) {
    for (let x = 0; x < LOGO_SIZE; x++) {
      const [sx, sy] = source(x, y);
      logo.pixels[at(x, y)] = old[at(sx, sy)];
    }
  }
}

const last = LOGO_SIZE - 1;
export const flipHorizontal = (logo: Logo): void => remap(logo, (x, y) => [last - x, y]);
export const flipVertical = (logo: Logo): void => remap(logo, (x, y) => [x, last - y]);
export const rotateClockwise = (logo: Logo): void => remap(logo, (x, y) => [y, last - x]);
export const rotateCounterClockwise = (logo: Logo): void => remap(logo, (x, y) => [last - y, x]);
/** Moves the picture, wrapping round the edges as the game's editor does. */
export const shift = (logo: Logo, dx: number, dy: number): void =>
  remap(logo, (x, y) => [(x - dx + LOGO_SIZE * 8) % LOGO_SIZE, (y - dy + LOGO_SIZE * 8) % LOGO_SIZE]);

// ---- importing ------------------------------------------------------------------

/** Reduces an image (RGBA, LOGO_SIZE squared) to the logo's fifteen colours by
 *  median cut; pixels that are mostly transparent stay transparent. */
export function quantize(rgba: Uint8ClampedArray): Logo {
  const logo = blankLogo();
  const opaque: number[] = [];
  for (let i = 0; i < LOGO_SIZE * LOGO_SIZE; i++) if (rgba[i * 4 + 3] >= 128) opaque.push(i);
  if (!opaque.length) return logo;
  const rgb = (i: number): [number, number, number] => [rgba[i * 4] >> 3, rgba[i * 4 + 1] >> 3, rgba[i * 4 + 2] >> 3];
  let boxes: number[][] = [opaque];
  while (boxes.length < LOGO_COLORS - 1) {
    // Split the box whose widest colour channel spans the most.
    let best = -1, bestSpan = 0, bestAxis = 0;
    boxes.forEach((box, b) => {
      if (box.length < 2) return;
      for (let axis = 0; axis < 3; axis++) {
        let low = 31, high = 0;
        for (const i of box) { const v = rgb(i)[axis]; if (v < low) low = v; if (v > high) high = v; }
        if (high - low > bestSpan) { best = b; bestSpan = high - low; bestAxis = axis; }
      }
    });
    if (best < 0) break;
    const sorted = boxes[best].slice().sort((a, b) => rgb(a)[bestAxis] - rgb(b)[bestAxis]);
    const half = sorted.length >> 1;
    boxes.splice(best, 1, sorted.slice(0, half), sorted.slice(half));
  }
  boxes = boxes.filter((box) => box.length);
  boxes.forEach((box, b) => {
    const sum = [0, 0, 0];
    for (const i of box) rgb(i).forEach((v, axis) => { sum[axis] += v; });
    const mean = sum.map((v) => Math.round(v / box.length));
    logo.palette[b + 1] = rgb15(mean[0] << 3, mean[1] << 3, mean[2] << 3);
    for (const i of box) logo.pixels[i] = b + 1;
  });
  for (let b = boxes.length + 1; b < LOGO_COLORS; b++) logo.palette[b] = DEFAULT_PALETTE[b];
  return logo;
}

/** What the windscreen shows of a name: the game's font has capitals, digits
 *  and . - ! ? @ (a space and underscore are blanks), six characters at most.
 *  Mirrors CarCustomSetTag in engine/src/port/car_custom.c, which does the drawing. */
export function teamTag(name: string): string {
  let tag = '';
  for (const c of name.toUpperCase()) {
    if (tag.length === 6) break;
    if (/^[A-Z0-9.\-!?@ ]$/.test(c)) tag += c;
    else if (c === '_') tag += ' ';
  }
  return tag.trim(); // blanks at the ends only shift the name off centre
}
