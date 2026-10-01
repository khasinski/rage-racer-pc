// The garage's logo editor: a 64x64 canvas with a 16-colour palette, pen, fill
// and picker, transforms, undo and picture import, over the Logo operations in
// logo.ts. It owns the logo being edited and reports every change; the garage
// decides what to do with it (preview, saving).
import { LOGO_SIZE } from '../shared/protocol.ts';
import {
  blankLogo, channels, cloneLogo, css, fill, flipHorizontal, flipVertical, fromHex, line, quantize,
  rotateClockwise, rotateCounterClockwise, shift, stamp, toHex, type Logo,
} from './logo';
import { $, el } from './views';

const UNDO_LIMIT = 40;

type Tool = 'pen' | 'fill' | 'pick';
const TOOLS: Tool[] = ['pen', 'fill', 'pick'];

const TRANSFORMS: Record<string, (logo: Logo) => void> = {
  flipH: flipHorizontal, flipV: flipVertical, turnCw: rotateClockwise, turnCcw: rotateCounterClockwise,
  left: (logo) => shift(logo, -1, 0), right: (logo) => shift(logo, 1, 0),
  up: (logo) => shift(logo, 0, -1), down: (logo) => shift(logo, 0, 1),
};

/** A picture fitted into the logo's square (letterboxed), as RGBA. */
async function fitPicture(file: File): Promise<Uint8ClampedArray> {
  const bitmap = await createImageBitmap(file);
  try {
    const scratch = document.createElement('canvas');
    scratch.width = scratch.height = LOGO_SIZE;
    const context = scratch.getContext('2d', { willReadFrequently: true })!;
    const scale = Math.min(LOGO_SIZE / bitmap.width, LOGO_SIZE / bitmap.height);
    const width = Math.max(1, Math.round(bitmap.width * scale));
    const height = Math.max(1, Math.round(bitmap.height * scale));
    context.drawImage(bitmap, Math.round((LOGO_SIZE - width) / 2), Math.round((LOGO_SIZE - height) / 2), width, height);
    return context.getImageData(0, 0, LOGO_SIZE, LOGO_SIZE).data;
  } finally {
    bitmap.close();
  }
}

export class LogoEditor {
  private readonly canvas = $<HTMLCanvasElement>('logo-canvas');
  private readonly paletteRow = $('logo-palette');
  private readonly colourInput = $<HTMLInputElement>('logo-colour');
  private readonly undoButton = $<HTMLButtonElement>('logo-undo');
  private readonly brushSelect = $<HTMLSelectElement>('logo-brush');
  private current: Logo = blankLogo();
  private undo: Logo[] = [];
  private tool: Tool = 'pen';
  private colour = 1; // the palette entry drawn with
  private stroke: { x: number; y: number; index: number } | null = null;

  /** `onChange` runs after every edit; `onError` gets a message for the player. */
  constructor(private readonly onChange: () => void, private readonly onError: (message: string) => void) {
    this.canvas.addEventListener('contextmenu', (event) => event.preventDefault());
    this.canvas.addEventListener('pointerdown', (event) => this.press(event));
    this.canvas.addEventListener('pointermove', (event) => this.drag(event));
    for (const type of ['pointerup', 'pointercancel']) this.canvas.addEventListener(type, () => { this.stroke = null; });
    for (const tool of TOOLS) $(`tool-${tool}`).onclick = () => this.selectTool(tool);
    this.colourInput.oninput = () => {
      if (this.colour === 0) return;
      this.current.palette[this.colour] = fromHex(this.colourInput.value);
      this.drawPalette();
      this.changed();
    };
    for (const button of document.querySelectorAll<HTMLElement>('[data-logo]')) {
      button.onclick = () => this.edit(TRANSFORMS[button.dataset.logo!]);
    }
    this.undoButton.onclick = () => {
      const previous = this.undo.pop();
      if (previous) this.current = previous;
      this.drawPalette();
      this.changed();
    };
    $('logo-clear').onclick = () => this.edit((logo) => logo.pixels.fill(0));
    $<HTMLInputElement>('logo-import').onchange = (event) => void this.importPicture(event.target as HTMLInputElement);
  }

  /** The logo as edited so far. */
  get logo(): Logo { return this.current; }

  /** Starts editing a logo (a copy of it; null for a blank one), with no undo history. */
  load(logo: Logo | null): void {
    this.current = logo ? cloneLogo(logo) : blankLogo();
    this.undo = [];
    this.colour = 1;
    this.drawPalette();
    this.draw();
    this.undoButton.disabled = true;
  }

  private selectTool(tool: Tool): void {
    this.tool = tool;
    for (const other of TOOLS) $(`tool-${other}`).setAttribute('aria-pressed', String(other === tool));
  }

  private remember(): void {
    this.undo.push(cloneLogo(this.current));
    if (this.undo.length > UNDO_LIMIT) this.undo.shift();
  }

  private edit(change: (logo: Logo) => void): void {
    this.remember();
    change(this.current);
    this.changed();
  }

  private changed(): void {
    this.draw();
    this.undoButton.disabled = this.undo.length === 0;
    this.onChange();
  }

  private pixelAt(event: PointerEvent): [number, number] {
    const box = this.canvas.getBoundingClientRect();
    const clamp = (v: number) => Math.max(0, Math.min(LOGO_SIZE - 1, Math.floor(v)));
    return [clamp(((event.clientX - box.left) / box.width) * LOGO_SIZE), clamp(((event.clientY - box.top) / box.height) * LOGO_SIZE)];
  }

  private get brush(): number { return Number(this.brushSelect.value); }

  private press(event: PointerEvent): void {
    const [x, y] = this.pixelAt(event);
    if (this.tool === 'pick') {
      this.colour = this.current.pixels[y * LOGO_SIZE + x] || this.colour;
      this.drawPalette();
      return;
    }
    const index = event.button === 2 ? 0 : this.colour; // the right button rubs out
    this.remember();
    if (this.tool === 'fill') {
      fill(this.current, x, y, index);
    } else {
      this.canvas.setPointerCapture(event.pointerId);
      this.stroke = { x, y, index };
      stamp(this.current, x, y, this.brush, index);
    }
    this.changed();
  }

  private drag(event: PointerEvent): void {
    const stroke = this.stroke;
    if (!stroke) return;
    const [x, y] = this.pixelAt(event);
    if (x === stroke.x && y === stroke.y) return;
    line(this.current, stroke.x, stroke.y, x, y, this.brush, stroke.index);
    stroke.x = x;
    stroke.y = y;
    this.changed();
  }

  /** A picture, fitted into the square and reduced to the logo's fifteen colours. */
  private async importPicture(input: HTMLInputElement): Promise<void> {
    const file = input.files?.[0];
    input.value = '';
    if (!file) return;
    let rgba: Uint8ClampedArray;
    try {
      rgba = await fitPicture(file);
    } catch {
      this.onError('That file is not a picture this browser can read.');
      return;
    }
    this.remember();
    this.current = quantize(rgba);
    this.colour = 1;
    this.drawPalette();
    this.changed();
  }

  private draw(): void {
    const context = this.canvas.getContext('2d')!;
    const image = context.createImageData(LOGO_SIZE, LOGO_SIZE);
    this.current.pixels.forEach((index, i) => {
      if (index === 0) return;
      const [r, g, b] = channels(this.current.palette[index]);
      image.data.set([r, g, b, 255], i * 4);
    });
    context.putImageData(image, 0, 0);
  }

  private drawPalette(): void {
    this.paletteRow.replaceChildren(...this.current.palette.map((color, index) => {
      const button = el('button', { type: 'button', className: 'swatch', title: index ? `Colour ${index}` : 'Transparent' });
      button.dataset.index = String(index);
      if (index) button.style.background = css(color);
      button.setAttribute('aria-label', index ? `Logo colour ${index}` : 'Transparent');
      button.setAttribute('aria-pressed', String(index === this.colour));
      button.onclick = () => {
        this.colour = index;
        this.drawPalette();
      };
      return button;
    }));
    this.colourInput.disabled = this.colour === 0;
    this.colourInput.value = toHex(this.current.palette[this.colour] || this.current.palette[1]);
  }
}
