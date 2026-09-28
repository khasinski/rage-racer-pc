// The retail race tachometer (web/wasm/web_hud.c): the car's own dial face,
// the needle, the shift light and the speed and gear digits, drawn from the
// disc's HUD sprites over the 3D view. The bridge evaluates the packets as
// retail builds them each game frame; this only rasterises them on the PAL
// 320x240 screen, scaled to the canvas height and centred like the mirror,
// with nearest filtering so the pixel art stays crisp.
import { PAL_HEIGHT, PAL_WIDTH } from './constants';
import type { Rage } from './rage';

/* Atlas layout (web_hud.c): normal and dark faces side by side, then the
 * manual and automatic digit strips, 8x8 per digit. */
const DIGITS_Y = 96;
const DIGIT = 8;
/* Word layout of rw_tachometer (web_hud.c). */
const T = {
  visible: 0, needle: 1, needleColor: 9, face: 12, gear: 16, speed: 19, digitClut: 22, shiftLight: 23, faceSize: 26,
} as const;

export class Tachometer {
  private readonly context: CanvasRenderingContext2D;
  private atlas: HTMLCanvasElement | null = null;
  /* The face after its brightness modulation. */
  private readonly face = document.createElement('canvas');
  private readonly faceContext: CanvasRenderingContext2D;

  constructor(private readonly canvas: HTMLCanvasElement) {
    this.context = canvas.getContext('2d')!;
    this.faceContext = this.face.getContext('2d')!;
  }

  /** Takes the sprites of the race the bridge just prepared. */
  prepare(rage: Rage) {
    const image = rage.hudAtlas();
    this.atlas = null;
    this.clear();
    if (!image) return;
    const atlas = document.createElement('canvas');
    atlas.width = image.width;
    atlas.height = image.height;
    const pixels = new ImageData(image.width, image.height);
    pixels.data.set(image.data);
    atlas.getContext('2d')!.putImageData(pixels, 0, 0);
    this.atlas = atlas;
  }

  clear() {
    this.context.clearRect(0, 0, this.canvas.width, this.canvas.height);
  }

  /** Draws this game frame's tachometer, or nothing while retail hides it. */
  draw(rage: Rage) {
    const ratio = Math.min(window.devicePixelRatio, 2);
    const width = Math.max(1, Math.round(this.canvas.clientWidth * ratio));
    const height = Math.max(1, Math.round(this.canvas.clientHeight * ratio));
    if (this.canvas.width !== width || this.canvas.height !== height) {
      this.canvas.width = width;
      this.canvas.height = height;
    }
    const c = this.context;
    c.clearRect(0, 0, width, height);
    const t = rage.tachometer();
    if (!this.atlas || !t[T.visible]) return;
    c.imageSmoothingEnabled = false;

    // PAL coordinates on the centred 240-line screen. hud_config.c's
    // HudRightX pushes the right-hand furniture out to the edge of a wider
    // screen (53 PAL pixels at 16:9); the whole tachometer hangs off it.
    const scale = height / PAL_HEIGHT;
    const edge = Math.max(0, Math.floor((width / scale - PAL_WIDTH) / 2));
    const left = width / 2 - (PAL_WIDTH / 2) * scale;
    const px = (x: number) => Math.round(left + (x + edge) * scale);
    const py = (y: number) => Math.round(y * scale);
    const sprite = (sx: number, sy: number, w: number, h: number, x: number, y: number, source: CanvasImageSource) =>
      c.drawImage(source, sx, sy, w, h, px(x), py(y), px(x + w) - px(x), py(y + h) - py(y));

    // Packet order as the ordering table draws it (draw_tachometer.c): the
    // shift light TILE, the face, the speed and gear digits, the needle.
    c.fillStyle = `rgb(${t[T.shiftLight + 2]}, 32, 32)`;
    c.fillRect(px(t[T.shiftLight]), py(t[T.shiftLight + 1]),
               px(t[T.shiftLight] + 16) - px(t[T.shiftLight]), py(t[T.shiftLight + 1] + 16) - py(t[T.shiftLight + 1]));

    const faceWidth = t[T.faceSize], faceHeight = t[T.faceSize + 1];
    const brightness = t[T.face + 2];
    let face: CanvasImageSource = this.atlas;
    let faceX = t[T.face + 3] ? faceWidth : 0, faceY = 0;
    if (brightness !== 128) {
      // SetShadeTex off: texels are modulated by brightness / 128.
      const f = this.faceContext;
      if (this.face.width !== faceWidth || this.face.height !== faceHeight) {
        this.face.width = faceWidth;
        this.face.height = faceHeight;
      }
      f.globalCompositeOperation = 'copy';
      f.drawImage(this.atlas, faceX, 0, faceWidth, faceHeight, 0, 0, faceWidth, faceHeight);
      f.globalCompositeOperation = 'source-atop';
      f.fillStyle = `rgba(0, 0, 0, ${1 - Math.min(brightness, 128) / 128})`;
      f.fillRect(0, 0, faceWidth, faceHeight);
      face = this.face;
      faceX = faceY = 0;
    }
    sprite(faceX, faceY, faceWidth, faceHeight, t[T.face], t[T.face + 1], face);

    const digitsY = DIGITS_Y + t[T.digitClut] * DIGIT;
    const digit = (value: number, x: number, y: number) =>
      sprite(value * DIGIT, digitsY, DIGIT, DIGIT, x, y, this.atlas!);
    const speed = t[T.speed + 2];
    [Math.floor(speed / 100), Math.floor(speed / 10) % 10, speed % 10].forEach((value, i) =>
      digit(value, t[T.speed] + i * DIGIT, t[T.speed + 1]));
    digit(t[T.gear + 2], t[T.gear], t[T.gear + 1]);

    // The flat POLY_F4 needle: vertices 0, 1, 3, 2 go round its outline.
    c.fillStyle = `rgb(${t[T.needleColor]}, ${t[T.needleColor + 1]}, ${t[T.needleColor + 2]})`;
    c.beginPath();
    for (const v of [0, 1, 3, 2]) {
      const x = left + (t[T.needle + v * 2] + edge) * scale, y = t[T.needle + v * 2 + 1] * scale;
      if (v === 0) c.moveTo(x, y);
      else c.lineTo(x, y);
    }
    c.closePath();
    c.fill();
  }
}
