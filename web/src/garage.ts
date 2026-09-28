// The garage: pick a car, paint it, give it a team logo and watch it turn on a
// showroom camera. Paint and the logo are presentation only; they are saved on
// the server (the paint per car model, the logo once) and drawn by everyone
// who races or watches you. The windscreen shows your name.
import { LOGO_SIZE, PAINTABLE_MODELS, type Paint } from '../shared/protocol.ts';
import { app, session, show } from './app';
import {
  blankLogo, channels, cloneLogo, css, encodeLogo, fill, flipHorizontal, flipVertical, fromHex, isEmpty, line, logoFromBase64,
  quantize, rotateClockwise, rotateCounterClockwise, sameLogo, shift, stamp, teamTag, toBase64, toHex, type Logo,
} from './logo';
import { Renderer } from './renderer';
import type { Rage } from './rage';
import { $, el } from './views';

const TURN_DEGREES_PER_SECOND = 14;
const DRAG_DEGREES_PER_PIXEL = 0.5;
const UNDO_LIMIT = 40;
const AUTOSAVE_MS = 600; // a quiet moment after the last change

const canvas = $<HTMLCanvasElement>('garage-view');
const carSelect = $<HTMLSelectElement>('garage-car');
const statusLine = $('garage-status');
const factoryButton = $<HTMLButtonElement>('garage-factory');
const rows = [$('paint-first'), $('paint-second')];
const logoCanvas = $<HTMLCanvasElement>('logo-canvas');
const paletteRow = $('logo-palette');
const colourInput = $<HTMLInputElement>('logo-colour');
const undoButton = $<HTMLButtonElement>('logo-undo');

let renderer: Renderer | null = null;
let open = false;
let angle = 30; // degrees round the car
let dragging = false;
let lastFrame = 0;
let shownSize = '';
let variant = -1;
let draft: Paint | null = null; // the colours on show, saved or not
let swatches: HTMLButtonElement[][] = [];
let draftLogo: Logo = blankLogo(); // the logo being edited, saved or not
let undo: Logo[] = [];
let tool: 'pen' | 'fill' | 'pick' = 'pen';
let colour = 1; // the palette entry drawn with
let previewStale = true; // the preview's logo needs sending to the module
let saveTimer: ReturnType<typeof setTimeout> | null = null;
let saving: Promise<void> = Promise.resolve();

const same = (a: Paint | null, b: Paint | null) => a === b || (!!a && !!b && a[0] === b[0] && a[1] === b[1]);
/** A logo as it is kept: nothing drawn means none. */
const kept = (logo: Logo): Logo | null => (isEmpty(logo) ? null : logo);

/** The player's saved paint for a car, for the races they start. */
export function paintOf(rage: Rage, carVariant: number): Paint | null {
  const model = rage.modelOf(carVariant);
  return model < PAINTABLE_MODELS ? app.garage[model] ?? null : null;
}

/** Fetches the saved paints and logo after logging in; a failure leaves the
 *  factory look. */
export async function loadGarage(): Promise<void> {
  try { app.garage = await session.garage(); } catch { app.garage = {}; }
  try { app.logo = logoFromBase64(await session.logo()); } catch { app.logo = null; }
}

/** Hands the player's own logo and name to the next race prepared (seat 0 offline). */
export function useOwnLook(rage: Rage, seat: number): void {
  rage.setLogo(seat, app.logo ? encodeLogo(app.logo) : null);
  rage.setTag(seat, session.user?.name ?? '');
}

const savedPaint = (rage: Rage) => paintOf(rage, variant);
const paintable = (rage: Rage) => rage.modelOf(variant) < PAINTABLE_MODELS;
const paintChanged = (rage: Rage) => paintable(rage) && !same(draft, savedPaint(rage));
const logoChanged = () => !sameLogo(kept(draftLogo), app.logo);

// ---- paint ----------------------------------------------------------------------

function buildSwatches(rage: Rage): void {
  swatches = rows.map((row, zone) => {
    const buttons = Array.from({ length: rage.paintCount() }, (_, color) => {
      const button = el('button', { type: 'button', className: 'swatch', title: `Colour ${color + 1}` });
      button.style.background = rage.paintSwatch(color);
      button.setAttribute('aria-label', `${zone === 0 ? 'First' : 'Second'} colour ${color + 1}`);
      button.onclick = () => choose(rage, zone, color);
      return button;
    });
    row.replaceChildren(...buttons);
    return buttons;
  });
}

function refresh(rage: Rage, message = ''): void {
  const canPaint = paintable(rage);
  if (!canPaint) draft = null;
  rage.setShowroomPaint(draft);
  for (const button of [...swatches.flat(), factoryButton]) button.disabled = !canPaint;
  // The special cars take neither paint, a logo nor a name (as in retail).
  $<HTMLButtonElement>('tab-logo').disabled = !canPaint;
  if (!canPaint) selectTab('paint');
  swatches.forEach((buttons, zone) => buttons.forEach((button, color) => {
    button.setAttribute('aria-pressed', String(draft?.[zone] === color));
  }));
  factoryButton.setAttribute('aria-pressed', String(draft === null));
  const changed = paintChanged(rage) || logoChanged();
  statusLine.textContent = changed ? 'Saving…'
    : !canPaint && !message ? 'This car keeps its factory look: no repaint, logo or name.' : message;
}

/** Saves what changed after a quiet moment (and at once when leaving). */
function scheduleSave(): void {
  if (saveTimer) clearTimeout(saveTimer);
  saveTimer = setTimeout(() => void flush(), AUTOSAVE_MS);
}

function flush(): Promise<void> {
  if (saveTimer) clearTimeout(saveTimer);
  saveTimer = null;
  saving = saving.then(saveChanges);
  return saving;
}

async function saveChanges(): Promise<void> {
  const rage = app.rage!;
  const model = rage.modelOf(variant);
  const paint = draft, logo = kept(draftLogo);
  const paintDirty = paintChanged(rage), logoDirty = logoChanged();
  if (!paintDirty && !logoDirty) {
    refresh(rage, 'Saved. Everyone in your races sees this.'); // changed back to what was saved
    return;
  }
  try {
    if (paintDirty) app.garage = await session.savePaint(model, paint);
    if (logoDirty) app.logo = logoFromBase64(await session.saveLogo(logo ? toBase64(encodeLogo(logo)) : null));
    refresh(rage, 'Saved. Everyone in your races sees this.');
  } catch (error) {
    refresh(rage, (error as Error).message);
  }
}

function choose(rage: Rage, zone: number, color: number): void {
  // From the factory colours, the first pick paints the whole body; the
  // second row then sets the other colour apart.
  const next: Paint = draft ? [...draft] : [color, color];
  next[zone] = color;
  draft = next;
  refresh(rage);
  scheduleSave();
}

function showCar(rage: Rage): void {
  variant = Number(carSelect.value);
  statusLine.textContent = 'Loading the car…';
  if (!rage.startShowroom(variant)) {
    statusLine.textContent = 'This car could not be prepared from your disc.';
    return;
  }
  const paint = savedPaint(rage);
  draft = paint ? [...paint] : null;
  previewStale = true;
  rage.setShowroomTag(session.user?.name ?? '');
  refresh(rage);
}

// ---- logo -----------------------------------------------------------------------

function drawLogo(): void {
  const context = logoCanvas.getContext('2d')!;
  const image = context.createImageData(LOGO_SIZE, LOGO_SIZE);
  draftLogo.pixels.forEach((index, i) => {
    if (index === 0) return;
    const [r, g, b] = channels(draftLogo.palette[index]);
    image.data.set([r, g, b, 255], i * 4);
  });
  context.putImageData(image, 0, 0);
}

function drawPalette(): void {
  paletteRow.replaceChildren(...draftLogo.palette.map((color, index) => {
    const button = el('button', { type: 'button', className: 'swatch', title: index ? `Colour ${index}` : 'Transparent' });
    button.dataset.index = String(index);
    if (index) button.style.background = css(color);
    button.setAttribute('aria-label', index ? `Logo colour ${index}` : 'Transparent');
    button.setAttribute('aria-pressed', String(index === colour));
    button.onclick = () => { colour = index; drawPalette(); };
    return button;
  }));
  colourInput.disabled = colour === 0;
  colourInput.value = toHex(draftLogo.palette[colour] || draftLogo.palette[1]);
}

/** The logo changed: redraw the editor and queue the preview. */
function logoEdited(rage: Rage | null = app.rage): void {
  drawLogo();
  previewStale = true;
  undoButton.disabled = undo.length === 0;
  if (rage) refresh(rage);
  scheduleSave();
}

function remember(): void {
  undo.push(cloneLogo(draftLogo));
  if (undo.length > UNDO_LIMIT) undo.shift();
}

function edit(change: (logo: Logo) => void): void {
  remember();
  change(draftLogo);
  logoEdited();
}

function pixelAt(event: PointerEvent): [number, number] {
  const box = logoCanvas.getBoundingClientRect();
  const clamp = (v: number) => Math.max(0, Math.min(LOGO_SIZE - 1, Math.floor(v)));
  return [clamp(((event.clientX - box.left) / box.width) * LOGO_SIZE), clamp(((event.clientY - box.top) / box.height) * LOGO_SIZE)];
}

let stroke: { x: number; y: number; index: number } | null = null;

logoCanvas.addEventListener('contextmenu', (event) => event.preventDefault());
logoCanvas.addEventListener('pointerdown', (event) => {
  const [x, y] = pixelAt(event);
  const brush = Number($<HTMLSelectElement>('logo-brush').value);
  if (tool === 'pick') {
    colour = draftLogo.pixels[y * LOGO_SIZE + x] || colour;
    drawPalette();
    return;
  }
  const index = event.button === 2 ? 0 : colour;
  remember();
  if (tool === 'fill') {
    fill(draftLogo, x, y, index);
    logoEdited();
    return;
  }
  logoCanvas.setPointerCapture(event.pointerId);
  stroke = { x, y, index };
  stamp(draftLogo, x, y, brush, index);
  logoEdited();
});
logoCanvas.addEventListener('pointermove', (event) => {
  if (!stroke) return;
  const [x, y] = pixelAt(event);
  if (x === stroke.x && y === stroke.y) return;
  line(draftLogo, stroke.x, stroke.y, x, y, Number($<HTMLSelectElement>('logo-brush').value), stroke.index);
  stroke.x = x;
  stroke.y = y;
  logoEdited();
});
for (const type of ['pointerup', 'pointercancel']) logoCanvas.addEventListener(type, () => { stroke = null; });

for (const name of ['pen', 'fill', 'pick'] as const) {
  $(`tool-${name}`).onclick = () => {
    tool = name;
    for (const other of ['pen', 'fill', 'pick']) $(`tool-${other}`).setAttribute('aria-pressed', String(other === name));
  };
}
colourInput.oninput = () => {
  if (colour === 0) return;
  draftLogo.palette[colour] = fromHex(colourInput.value);
  drawPalette();
  logoEdited();
};

const transforms: Record<string, (logo: Logo) => void> = {
  flipH: flipHorizontal, flipV: flipVertical, turnCw: rotateClockwise, turnCcw: rotateCounterClockwise,
  left: (logo) => shift(logo, -1, 0), right: (logo) => shift(logo, 1, 0), up: (logo) => shift(logo, 0, -1), down: (logo) => shift(logo, 0, 1),
};
for (const button of document.querySelectorAll<HTMLElement>('[data-logo]')) {
  button.onclick = () => edit(transforms[button.dataset.logo!]);
}
undoButton.onclick = () => {
  const previous = undo.pop();
  if (previous) draftLogo = previous;
  drawPalette();
  logoEdited();
};
$('logo-clear').onclick = () => edit((logo) => logo.pixels.fill(0));

/** A picture fitted into the logo's square, reduced to its fifteen colours. */
$<HTMLInputElement>('logo-import').onchange = async (event) => {
  const input = event.target as HTMLInputElement;
  const file = input.files?.[0];
  input.value = '';
  if (!file) return;
  try {
    const bitmap = await createImageBitmap(file);
    const scratch = document.createElement('canvas');
    scratch.width = scratch.height = LOGO_SIZE;
    const context = scratch.getContext('2d', { willReadFrequently: true })!;
    const scale = Math.min(LOGO_SIZE / bitmap.width, LOGO_SIZE / bitmap.height);
    const width = Math.max(1, Math.round(bitmap.width * scale)), height = Math.max(1, Math.round(bitmap.height * scale));
    context.drawImage(bitmap, Math.round((LOGO_SIZE - width) / 2), Math.round((LOGO_SIZE - height) / 2), width, height);
    remember();
    draftLogo = quantize(context.getImageData(0, 0, LOGO_SIZE, LOGO_SIZE).data);
    colour = 1;
    drawPalette();
    logoEdited();
  } catch {
    statusLine.textContent = 'That file is not a picture this browser can read.';
  }
};

// ---- tabs -----------------------------------------------------------------------

function selectTab(name: 'paint' | 'logo'): void {
  $('pane-paint').hidden = name !== 'paint';
  $('pane-logo').hidden = name !== 'logo';
  $('tab-paint').setAttribute('aria-selected', String(name === 'paint'));
  $('tab-logo').setAttribute('aria-selected', String(name === 'logo'));
}
$('tab-paint').onclick = () => selectTab('paint');
$('tab-logo').onclick = () => selectTab('logo');

// ---- the screen -----------------------------------------------------------------

function frame(now: number): void {
  if (!open) return;
  requestAnimationFrame(frame);
  const rage = app.rage;
  if (!rage || !renderer) return;
  if (previewStale) {
    previewStale = false;
    rage.setShowroomLogo(isEmpty(draftLogo) ? null : encodeLogo(draftLogo));
  }
  if (!dragging) angle = (angle + ((now - lastFrame) / 1000) * TURN_DEGREES_PER_SECOND) % 360;
  lastFrame = now;
  const size = `${canvas.clientWidth}x${canvas.clientHeight}`;
  if (size !== shownSize) {
    shownSize = size;
    renderer.resize(canvas.clientWidth, canvas.clientHeight);
  }
  const count = rage.buildShowroom(renderer.aspect, angle);
  if (count < 0) return;
  renderer.update(count);
  renderer.render();
}

export function enterGarage(): void {
  const rage = app.rage!;
  show('garage');
  renderer ??= new Renderer(canvas, rage);
  if (!carSelect.options.length) {
    carSelect.replaceChildren(...rage.garageVariants().map((v) =>
      new Option(`${rage.carName(v)} ${'I'.repeat(rage.carGrade(v) + 1)}`, String(v))));
    buildSwatches(rage);
  }
  // Start from the saved logo; leaving without saving drops the changes.
  draftLogo = app.logo ? cloneLogo(app.logo) : blankLogo();
  undo = [];
  colour = 1;
  $('logo-tag').textContent = teamTag(session.user?.name ?? '') || '(nothing the font can show)';
  drawPalette();
  drawLogo();
  undoButton.disabled = true;
  open = true;
  lastFrame = performance.now();
  showCar(rage);
  requestAnimationFrame(frame);
}

async function leave(): Promise<void> {
  await flush(); // the races started next read the saved look
  open = false;
  show('lobby');
}

// The automated checks turn the car themselves.
if (location.hash === '#e2e') Object.assign(window, { __garage: { turnTo: (degrees: number) => { dragging = true; angle = degrees; } } });

carSelect.onchange = async () => {
  await flush(); // the draft belongs to the car being left
  showCar(app.rage!);
};
factoryButton.onclick = () => {
  draft = null;
  refresh(app.rage!);
  scheduleSave();
};
$('garage').querySelector('[data-action=garage-back]')!.addEventListener('click', () => void leave());

canvas.addEventListener('pointerdown', (event) => { dragging = true; canvas.setPointerCapture(event.pointerId); });
canvas.addEventListener('pointerup', () => { dragging = false; });
canvas.addEventListener('pointercancel', () => { dragging = false; });
canvas.addEventListener('pointermove', (event) => { if (dragging) angle = (angle - event.movementX * DRAG_DEGREES_PER_PIXEL + 360) % 360; });
