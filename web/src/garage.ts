// The garage: pick a car, choose its two body colours and watch it turn on a
// showroom camera. Paint is presentation only; it is saved per car model on
// the server and shown by everyone who races or watches you.
import type { Paint } from '../shared/protocol.ts';
import { app, session, show } from './app';
import { Renderer } from './renderer';
import type { Rage } from './rage';
import { $, el } from './views';

const TURN_DEGREES_PER_SECOND = 14;
const DRAG_DEGREES_PER_PIXEL = 0.5;

const canvas = $<HTMLCanvasElement>('garage-view');
const carSelect = $<HTMLSelectElement>('garage-car');
const statusLine = $('garage-status');
const saveButton = $<HTMLButtonElement>('garage-save');
const factoryButton = $<HTMLButtonElement>('garage-factory');
const rows = [$('paint-first'), $('paint-second')];

let renderer: Renderer | null = null;
let open = false;
let angle = 30; // degrees round the car
let dragging = false;
let lastFrame = 0;
let shownSize = '';
let variant = -1;
let draft: Paint | null = null; // the colours on show, saved or not
let swatches: HTMLButtonElement[][] = [];

const same = (a: Paint | null, b: Paint | null) => a === b || (!!a && !!b && a[0] === b[0] && a[1] === b[1]);

/** The player's saved paint for a car, for the races they start. */
export function paintOf(rage: Rage, carVariant: number): Paint | null {
  return app.garage[rage.modelOf(carVariant)] ?? null;
}

/** Fetches the saved paints after logging in; a failure leaves factory colours. */
export async function loadGarage(): Promise<void> {
  try { app.garage = await session.garage(); } catch { app.garage = {}; }
}

const saved = (rage: Rage) => paintOf(rage, variant);

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
  rage.setShowroomPaint(draft);
  swatches.forEach((buttons, zone) => buttons.forEach((button, color) => {
    button.setAttribute('aria-pressed', String(draft?.[zone] === color));
  }));
  factoryButton.setAttribute('aria-pressed', String(draft === null));
  const changed = !same(draft, saved(rage));
  saveButton.disabled = !changed;
  statusLine.textContent = changed ? 'Not saved yet.' : message;
}

function choose(rage: Rage, zone: number, color: number): void {
  // From the factory colours, the first pick paints the whole body; the
  // second row then sets the other colour apart.
  const next: Paint = draft ? [...draft] : [color, color];
  next[zone] = color;
  draft = next;
  refresh(rage);
}

function showCar(rage: Rage): void {
  variant = Number(carSelect.value);
  statusLine.textContent = 'Loading the car…';
  if (!rage.startShowroom(variant)) {
    statusLine.textContent = 'This car could not be prepared from your disc.';
    return;
  }
  const paint = saved(rage);
  draft = paint ? [...paint] : null;
  refresh(rage);
}

function frame(now: number): void {
  if (!open) return;
  requestAnimationFrame(frame);
  const rage = app.rage;
  if (!rage || !renderer) return;
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
  open = true;
  lastFrame = performance.now();
  showCar(rage);
  requestAnimationFrame(frame);
}

function leave(): void {
  open = false;
  show('lobby');
}

carSelect.onchange = () => showCar(app.rage!);
factoryButton.onclick = () => { draft = null; refresh(app.rage!); };
saveButton.onclick = async () => {
  const rage = app.rage!;
  saveButton.disabled = true;
  try {
    app.garage = await session.savePaint(rage.modelOf(variant), draft);
    refresh(rage, 'Saved. Everyone in your races sees this paint.');
  } catch (error) {
    refresh(rage, (error as Error).message);
  }
};
$('garage').querySelector('[data-action=garage-back]')!.addEventListener('click', leave);

canvas.addEventListener('pointerdown', (event) => { dragging = true; canvas.setPointerCapture(event.pointerId); });
canvas.addEventListener('pointerup', () => { dragging = false; });
canvas.addEventListener('pointercancel', () => { dragging = false; });
canvas.addEventListener('pointermove', (event) => { if (dragging) angle = (angle - event.movementX * DRAG_DEGREES_PER_PIXEL + 360) % 360; });
