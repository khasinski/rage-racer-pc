// The garage: pick a car, paint it, give it a team logo and watch it turn on a
// showroom camera. Paint and the logo are presentation only; they are saved on
// the server (the paint per car model, the logo once) and drawn by everyone
// who races or watches you. The windscreen shows your name.
import { PAINTABLE_MODELS, type Paint, type RaceSeat } from '../shared/protocol.ts';
import { app, rememberCar, session, show } from './app';
import { encodeLogo, fromBase64, isEmpty, logoFromBase64, sameLogo, teamTag, toBase64, type Logo } from './logo';
import { LogoEditor } from './logo-editor';
import { Renderer } from './renderer';
import type { Rage, SeatLook } from './rage';
import { $, el, setCarNote } from './views';

const TURN_DEGREES_PER_SECOND = 14;
const DRAG_DEGREES_PER_PIXEL = 0.5;
/* The front three-quarter view, which shows the bonnet (and the logo on it). */
const FRONT_ANGLE = 320;
const SETTLE_DEGREES_PER_SECOND = 150;
const AUTOSAVE_MS = 600; // a quiet moment after the last change

const canvas = $<HTMLCanvasElement>('garage-view');
const carSelect = $<HTMLSelectElement>('garage-car');
const statusLine = $('garage-status');
const factoryButton = $<HTMLButtonElement>('garage-factory');
const rows = [$('paint-first'), $('paint-second')];

let renderer: Renderer | null = null;
let open = false;
let angle = FRONT_ANGLE; // degrees round the car
/* While the logo is edited the camera settles on the bonnet instead of turning
 * (until the player turns the car themselves). */
let settleAt: number | null = null;
let dragging = false;
let lastFrame = 0;
let shownSize = '';
let variant = -1;
let draft: Paint | null = null; // the colours on show, saved or not
let swatches: HTMLButtonElement[][] = [];
let previewStale = true; // the preview's logo needs sending to the module
let saveTimer: ReturnType<typeof setTimeout> | null = null;
let saving: Promise<void> = Promise.resolve();

const same = (a: Paint | null, b: Paint | null) => a === b || (!!a && !!b && a[0] === b[0] && a[1] === b[1]);
/** A logo as it is kept: nothing drawn means none. */
const kept = (logo: Logo): Logo | null => (isEmpty(logo) ? null : logo);

/* The race forms mark painted cars too. */
setCarNote((v) => (app.rage && paintOf(app.rage, v) ? ' · painted' : ''));

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

/** The player's own car look for a car (offline practice). */
export function ownLook(rage: Rage, carVariant: number): SeatLook {
  return { paint: paintOf(rage, carVariant), logo: app.logo ? encodeLogo(app.logo) : null, name: session.user?.name ?? '' };
}

/** A seat's car look as the server announced it with the race. */
export function seatLook(seat: RaceSeat): SeatLook {
  return { paint: seat.paint, logo: seat.logo ? fromBase64(seat.logo) : null, name: seat.name };
}

const savedPaint = (rage: Rage) => paintOf(rage, variant);
const paintable = (rage: Rage) => rage.modelOf(variant) < PAINTABLE_MODELS;
const paintChanged = (rage: Rage) => paintable(rage) && !same(draft, savedPaint(rage));
const logoChanged = () => !sameLogo(kept(editor.logo), app.logo);

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

/** A car in the garage's list, marked when the player has painted it. */
function carOption(rage: Rage, v: number): string {
  return `${rage.carName(v)} ${'I'.repeat(rage.carGrade(v) + 1)}${paintOf(rage, v) ? ' · painted' : ''}`;
}

function markPaintedCars(rage: Rage): void {
  for (const option of carSelect.options) {
    const label = carOption(rage, Number(option.value));
    if (option.textContent !== label) option.textContent = label;
  }
}

function refresh(rage: Rage, message = ''): void {
  markPaintedCars(rage);
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
  const paint = draft, logo = kept(editor.logo);
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
  rememberCar(variant);
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

/** The logo changed: queue the preview and the save. */
function logoEdited(): void {
  previewStale = true;
  if (app.rage) refresh(app.rage);
  scheduleSave();
}

const editor = new LogoEditor(logoEdited, (message) => { statusLine.textContent = message; });

// ---- tabs -----------------------------------------------------------------------

function selectTab(name: 'paint' | 'logo'): void {
  settleAt = name === 'logo' ? FRONT_ANGLE : null;
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
    rage.setShowroomLogo(isEmpty(editor.logo) ? null : encodeLogo(editor.logo));
  }
  const seconds = (now - lastFrame) / 1000;
  if (dragging) {
    // the player turns the car
  } else if (settleAt !== null) {
    const towards = ((settleAt - angle + 540) % 360) - 180; // the short way round
    angle = (angle + Math.sign(towards) * Math.min(Math.abs(towards), seconds * SETTLE_DEGREES_PER_SECOND) + 360) % 360;
  } else {
    angle = (angle + seconds * TURN_DEGREES_PER_SECOND) % 360;
  }
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
    carSelect.replaceChildren(...rage.garageVariants().map((v) => new Option(carOption(rage, v), String(v))));
    buildSwatches(rage);
  }
  // Open on the car the player last chose (here, in practice or in a room).
  if ([...carSelect.options].some((o) => o.value === String(app.lastCar))) carSelect.value = String(app.lastCar);
  editor.load(app.logo); // the saved logo
  $('logo-tag').textContent = teamTag(session.user?.name ?? '') || '(nothing the font can show)';
  open = true;
  angle = FRONT_ANGLE;
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
if (location.hash === '#e2e') Object.assign(window, { __garage: {
  turnTo: (degrees: number) => { dragging = true; angle = degrees; },
  angle: () => angle,
} });

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

canvas.addEventListener('pointerdown', (event) => {
  dragging = true;
  settleAt = null;
  canvas.setPointerCapture(event.pointerId);
});
canvas.addEventListener('pointerup', () => { dragging = false; });
canvas.addEventListener('pointercancel', () => { dragging = false; });
canvas.addEventListener('pointermove', (event) => { if (dragging) angle = (angle - event.movementX * DRAG_DEGREES_PER_PIXEL + 360) % 360; });
