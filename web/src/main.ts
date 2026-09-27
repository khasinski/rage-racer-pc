import './style.css';
import { chooseDataTrack, droppedFiles } from './disc';
import { consumeKey, sampleDriver } from './input';
import { PHASE_COUNTDOWN, PHASE_FINISHED, Rage, type Hud, type RaceOptions } from './rage';
import { Renderer } from './renderer';

const TICK_MS = 1000 / 50; // the simulation's fixed 50 Hz clock

const $ = <T extends HTMLElement>(id: string) => document.getElementById(id) as T;
const disc = $('disc');
const status = $('disc-status');
const picker = $<HTMLInputElement>('disc-input');
const setup = $<HTMLFormElement>('setup');
const hud = $('hud');
const canvas = $<HTMLCanvasElement>('view');

const ragePromise = Rage.load();
let renderer: Renderer | null = null;
let racing = false;

function show(screen: 'disc' | 'setup' | 'race') {
  disc.hidden = screen !== 'disc';
  setup.hidden = screen !== 'setup';
  hud.hidden = screen !== 'race';
  canvas.hidden = screen !== 'race';
  racing = screen === 'race';
}

async function useFiles(files: File[]) {
  const choice = await chooseDataTrack(files);
  if ('error' in choice) {
    status.textContent = choice.error;
    disc.dataset.state = 'error';
    return;
  }
  disc.dataset.state = 'busy';
  status.textContent = `Reading ${choice.file.name}…`;
  const rage = await ragePromise;
  const ok = await rage.loadDisc(choice.file, (fraction) => {
    status.textContent = `Reading ${choice.file.name}… ${Math.round(fraction * 100)}%`;
  });
  if (!ok) {
    disc.dataset.state = 'error';
    status.textContent = `${choice.file.name} is not a Rage Racer disc image this build can read.`;
    return;
  }
  disc.dataset.state = '';
  fillCars(rage.carAutomatic());
  show('setup');
  $('start').focus();
}

/* Car variants in disc order (model, then grade); a manual-only variant
 * cannot be driven with the automatic gearbox, as in the native menu. */
let automaticCars: boolean[] = [];
function fillCars(automatic: boolean[]) {
  automaticCars = automatic;
  const cars = $<HTMLSelectElement>('car');
  cars.replaceChildren(...automatic.map((auto, variant) =>
    new Option(`Car ${variant + 1}${auto ? '' : ' (manual only)'}`, String(variant))));
  syncTransmission();
}
function syncTransmission() {
  const car = Number($<HTMLSelectElement>('car').value);
  const transmission = setup.elements.namedItem('transmission') as HTMLSelectElement;
  const automatic = transmission.options[0];
  automatic.disabled = automaticCars[car] === false;
  if (automatic.disabled) transmission.value = '1';
}
$('car').addEventListener('change', syncTransmission);

picker.addEventListener('change', () => { if (picker.files?.length) void useFiles(Array.from(picker.files)); });
addEventListener('dragover', (event) => { event.preventDefault(); disc.dataset.drag = 'on'; });
addEventListener('dragleave', () => { disc.dataset.drag = ''; });
addEventListener('drop', (event) => {
  event.preventDefault();
  disc.dataset.drag = '';
  if (!disc.hidden && event.dataTransfer) void droppedFiles(event.dataTransfer).then(useFiles);
});

function readOptions(): RaceOptions {
  const value = (name: string) => Number((setup.elements.namedItem(name) as HTMLSelectElement).value);
  const checked = (name: string) => (setup.elements.namedItem(name) as HTMLInputElement).checked;
  return {
    classIndex: value('class'), course: value('course'), car: value('car'),
    manual: value('transmission') === 1, reverse: checked('reverse'),
    laps: value('laps'), rivals: checked('rivals'),
  };
}

setup.addEventListener('submit', async (event) => {
  event.preventDefault();
  const rage = await ragePromise;
  $('setup-status').textContent = 'Preparing the course and cars…';
  await new Promise(requestAnimationFrame);
  if (!rage.startRace(readOptions())) {
    $('setup-status').textContent = 'This course could not be prepared from the disc.';
    return;
  }
  $('setup-status').textContent = '';
  renderer ??= new Renderer(canvas, rage);
  rage.setDrawDistance(Number((setup.elements.namedItem('drawDistance') as HTMLSelectElement).value));
  renderer.shadows = (setup.elements.namedItem('shadows') as HTMLInputElement).checked;
  show('race');
  resize();
  last = performance.now();
  accumulator = simTime = previousStep = currentStep = 0;
});

function resize() {
  renderer?.resize(canvas.clientWidth, canvas.clientHeight);
}
addEventListener('resize', resize);

function formatTime(ms: number): string {
  if (ms < 0) return '--:--.--';
  const minutes = Math.floor(ms / 60000);
  const seconds = Math.floor(ms / 1000) % 60;
  const hundredths = Math.floor(ms / 10) % 100;
  return `${minutes}:${String(seconds).padStart(2, '0')}.${String(hundredths).padStart(2, '0')}`;
}

function drawHud(h: Hud) {
  $('hud-place').textContent = h.place > 0 ? `${h.place}/${h.entrants}` : `–/${h.entrants}`;
  $('hud-lap').textContent = `${Math.max(1, Math.min(h.lap || 1, h.laps))}/${h.laps}`;
  $('hud-time').textContent = formatTime(h.timeMs);
  $('hud-speed').textContent = String(h.speed);
  $('hud-gear').textContent = String(h.gear);
  const banner = $('hud-banner');
  if (h.phase === PHASE_COUNTDOWN) banner.textContent = String(Math.ceil(h.countdown / 50) || 'GO');
  else if (h.phase === PHASE_FINISHED || h.status === 2) banner.textContent = 'FINISH';
  else banner.textContent = '';
}

let last = 0;
let accumulator = 0;
/* Simulated time (ms) and the times of the last two physics steps: frames
 * are drawn between those two snapshots, one step behind the simulation. */
let simTime = 0;
let previousStep = 0;
let currentStep = 0;
function frame(now: number) {
  requestAnimationFrame(frame);
  if (!racing || !renderer) return;
  if (consumeKey('KeyC')) rageSync?.cycleCamera();
  if (consumeKey('Escape')) {
    show('setup');
    $('start').focus();
    return;
  }
  accumulator = Math.min(accumulator + (now - last), 250);
  last = now;
  const rage = rageSync;
  if (!rage) return;
  while (accumulator >= TICK_MS) {
    rage.setInput(sampleDriver());
    if (rage.tick() < 0) {
      $('setup-status').textContent = 'The race stopped unexpectedly.';
      show('setup');
      return;
    }
    accumulator -= TICK_MS;
    simTime += TICK_MS;
    if (rage.lastTickStepped()) {
      previousStep = currentStep;
      currentStep = simTime;
    }
  }
  const interval = Math.max(TICK_MS, currentStep - previousStep);
  const t = Math.min(1, Math.max(0, (simTime + accumulator - currentStep) / interval));
  const vertices = rage.buildFrame(renderer.aspect, t);
  if (vertices >= 0) renderer.update(vertices);
  drawHud(rage.hud());
  renderer.render();
}

let rageSync: Rage | null = null;
void ragePromise.then((rage) => { rageSync = rage; status.dataset.ready = '1'; });
show('disc');
requestAnimationFrame(frame);
