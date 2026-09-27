import './style.css';
import type { RaceEvent, RoomState, RoomSummary, ServerMessage } from '../shared/protocol.ts';
import { RaceAudio } from './audio';
import { chooseDataTrack, droppedFiles } from './disc';
import { clearKeyEdges, consumeKey, PAD, samplePad } from './input';
import { Connection, FrameBuffer, Session } from './net';
import { PHASE_COUNTDOWN, PHASE_FINISHED, PHASE_RACING, Rage, type Hud, type RaceOptions } from './rage';
import { Renderer } from './renderer';
import { Tachometer } from './hud';
import {
  $, appendChat, classCars, editSettings, fillClasses, fillCourses, formatTime, renderHistory,
  renderRecords, renderResults, renderRoom, renderRooms,
} from './views';

const TICK_MS = 1000 / 50; // the simulation's fixed 50 Hz clock

type Screen = 'auth' | 'disc' | 'lobby' | 'room' | 'setup' | 'race';
const SCREENS: Screen[] = ['auth', 'disc', 'lobby', 'room', 'setup'];

const discStatus = $('disc-status');
const picker = $<HTMLInputElement>('disc-input');
const setup = $<HTMLFormElement>('setup');
const hud = $('hud');
const canvas = $<HTMLCanvasElement>('view');
const results = $('results');
const tachometer = new Tachometer($<HTMLCanvasElement>('tacho'));

const ragePromise = Rage.load();
const session = new Session();
let rageSync: Rage | null = null;
let renderer: Renderer | null = null;
let audio: RaceAudio | null = null;
let screen: Screen = 'auth';
let discLoaded = false;
let automaticCars: boolean[] = [];

// Online state.
let connection: Connection | null = null;
let rooms: RoomSummary[] = [];
let room: RoomState | null = null;
let online = false; // the current race is driven by the server
/* Online: the players' seats and names, for the standings in the HUD. */
let players: { seat: number; name: string }[] = [];
let localSeat = 0;
let standingsAt = 0;
/* Every seat's name (players, then rivals), for spectating. */
let seatNames: string[] = [];
/* performance.now() when the server closes the race, once somebody finished. */
let deadlineAt: number | null = null;
/* Prediction: whether the local race runs ahead of the server yet, and the
 * smoothed clock error (ticks an input was used later than predicted). */
let predicting = false;
let clockError = 0;
const frames = new FrameBuffer();

function show(next: Screen) {
  screen = next;
  for (const id of SCREENS) $(id).hidden = id !== next;
  hud.hidden = canvas.hidden = next !== 'race';
  if (next !== 'race') results.hidden = true;
}

function toast(message: string) {
  const target = screen === 'room' ? 'room-status' : screen === 'setup' ? 'setup-status'
    : screen === 'auth' ? 'auth-status' : screen === 'disc' ? 'disc-status' : 'lobby-status';
  $(target).textContent = message;
  if (screen === 'race') feed(message);
}

// ---- accounts ---------------------------------------------------------------

$<HTMLFormElement>('auth').addEventListener('submit', async (event) => {
  event.preventDefault();
  const form = event.target as HTMLFormElement;
  const action = (event.submitter as HTMLButtonElement | null)?.value;
  const register = action === 'register';
  const value = (name: string) => (form.elements.namedItem(name) as HTMLInputElement).value;
  $('auth-status').textContent = action === 'guest' ? 'Joining as a guest…' : register ? 'Creating your account…' : 'Logging in…';
  try {
    if (action === 'guest') await session.guest();
    else await session.login(value('name').trim(), value('password'), register);
    $('auth-status').textContent = '';
    afterLogin();
  } catch (error) {
    $('auth-status').textContent = (error as Error).message;
  }
});

function afterLogin() {
  $('lobby').querySelector('.who')!.textContent = `${session.user!.name}${session.user!.admin ? ' (admin)' : ''}`;
  if (!discLoaded) {
    show('disc');
    return;
  }
  enterLobby();
}

// ---- disc -------------------------------------------------------------------

async function useFiles(files: File[]) {
  const disc = $('disc');
  const choice = await chooseDataTrack(files);
  if ('error' in choice) {
    discStatus.textContent = choice.error;
    disc.dataset.state = 'error';
    return;
  }
  disc.dataset.state = 'busy';
  discStatus.textContent = `Reading ${choice.file.name}…`;
  const rage = await ragePromise;
  const ok = await rage.loadDisc(choice.file, (fraction) => {
    discStatus.textContent = `Reading ${choice.file.name}… ${Math.round(fraction * 100)}%`;
  });
  if (!ok) {
    disc.dataset.state = 'error';
    discStatus.textContent = `${choice.file.name} is not a Rage Racer disc image this build can read.`;
    return;
  }
  if (session.discId && rage.discId() !== session.discId) {
    disc.dataset.state = 'error';
    discStatus.textContent = `This disc (${rage.discId()}) differs from the server's (${session.discId}). ` +
      'Online races need the same release; practice still works offline.';
  } else {
    disc.dataset.state = '';
  }
  discLoaded = true;
  void audio?.useDisc(files);
  automaticCars = rage.carAutomatic();
  fillPractice(rage);
  if (disc.dataset.state === 'error') {
    setTimeout(() => show('setup'), 2500);
    return;
  }
  enterLobby();
}

picker.addEventListener('change', () => { if (picker.files?.length) void useFiles(Array.from(picker.files)); });
addEventListener('dragover', (event) => { event.preventDefault(); $('disc').dataset.drag = 'on'; });
addEventListener('dragleave', () => { $('disc').dataset.drag = ''; });
addEventListener('drop', (event) => {
  event.preventDefault();
  $('disc').dataset.drag = '';
  if (screen === 'disc' && event.dataTransfer) void droppedFiles(event.dataTransfer).then(useFiles);
});

// ---- lobby ------------------------------------------------------------------

function enterLobby() {
  connect();
  show(room ? 'room' : 'lobby');
  void refreshTables();
}

async function refreshTables() {
  const rage = rageSync;
  if (!rage) return;
  try {
    renderRecords(rage, (await session.records()).records);
    const history = await fetch(new URL('api/history', document.baseURI),
      { headers: { authorization: `Bearer ${session.token}` } }).then((r) => r.json());
    renderHistory(rage, history.history ?? []);
  } catch { /* tables stay as they were */ }
}

function connect() {
  if (connection || !session.token) return;
  connection = new Connection(session.token);
  connection.onMessage = onMessage;
  connection.onFrame = (frame) => { if (online) frames.push(frame); };
  connection.onClose = (reason) => {
    connection = null;
    room = null;
    if (online) stopRace();
    if (!session.token) return;
    toast(`${reason} Reconnecting…`);
    if (screen === 'room') show('lobby');
    setTimeout(() => { if (!connection && session.token && discLoaded) connect(); }, 3000);
  };
}

function send(message: Parameters<Connection['send']>[0]) {
  connection?.send(message);
}

function onMessage(message: ServerMessage) {
  const rage = rageSync!;
  switch (message.t) {
    case 'welcome':
      session.discId = message.discId;
      $('lobby-status').textContent = '';
      break;
    case 'rooms':
      rooms = message.rooms;
      renderRooms(rage, rooms, (id) => send({ t: 'joinRoom', roomId: id }));
      break;
    case 'room':
      if (!message.room) {
        room = null;
        $('chat-log').replaceChildren();
        if (screen === 'room') show('lobby');
        void refreshTables();
        break;
      }
      if (!room || room.id !== message.room.id) $('chat-log').replaceChildren();
      room = message.room;
      renderRoom(rage, room, session.user!, automaticCars);
      if (screen === 'lobby') show('room');
      break;
    case 'chat':
      appendChat(message.from, message.text, message.at);
      break;
    case 'raceStart':
      void startOnlineRace(message);
      break;
    case 'raceGo':
      $('hud-hint').textContent = '';
      break;
    case 'finishDeadline':
      deadlineAt = performance.now() + message.remainingMs;
      break;
    case 'raceEvent':
      feed(describe(message.event));
      break;
    case 'results':
      renderResults(rage, message.results, session.user!);
      results.hidden = false;
      if (screen !== 'race') show('room');
      results.hidden = false;
      void refreshTables();
      break;
    case 'error':
      toast(message.message);
      break;
  }
}

$('lobby').addEventListener('click', async (event) => {
  const action = (event.target as HTMLElement).closest<HTMLElement>('[data-action]')?.dataset.action;
  if (action === 'logout') {
    connection?.close();
    connection = null;
    await session.logout();
    show('auth');
  } else if (action === 'practice') {
    show('setup');
  } else if (action === 'create-room' && rageSync) {
    const settings = await editSettings(rageSync, null, 1);
    if (settings) send({ t: 'createRoom', settings });
  }
});

// ---- room -------------------------------------------------------------------

$('room').addEventListener('click', async (event) => {
  const action = (event.target as HTMLElement).closest<HTMLElement>('[data-action]')?.dataset.action;
  if (action === 'leave-room') send({ t: 'leaveRoom' });
  else if (action === 'close-room' && room) send({ t: 'closeRoom', roomId: room.id });
  else if (action === 'edit-room' && room && rageSync) {
    const settings = await editSettings(rageSync, room.settings, room.members.length);
    if (settings) send({ t: 'updateRoom', settings });
  }
});

function sendCar() {
  const variant = Number($<HTMLSelectElement>('car-model').value);
  const transmission = $<HTMLSelectElement>('car-transmission');
  const automatic = automaticCars[variant] !== false;
  transmission.options[0].disabled = !automatic;
  if (!automatic) transmission.value = 'manual';
  send({ t: 'setCar', variant, manual: transmission.value === 'manual' });
}
$('car-model').addEventListener('change', sendCar);
$('car-transmission').addEventListener('change', sendCar);
$('ready-button').addEventListener('click', () => {
  const mine = room?.members.find((m) => m.userId === session.user?.id);
  send({ t: 'setReady', ready: !mine?.ready });
});
$('start-button').addEventListener('click', () => send({ t: 'startRace' }));
$('spectate-button').addEventListener('click', () => {
  const mine = room?.members.find((m) => m.userId === session.user?.id);
  send({ t: 'setSpectator', spectator: !mine?.spectator });
});
$('watch-button').addEventListener('click', () => send({ t: 'watchRace' }));
$<HTMLFormElement>('chat-form').addEventListener('submit', (event) => {
  event.preventDefault();
  const input = (event.target as HTMLFormElement).elements.namedItem('text') as HTMLInputElement;
  if (input.value.trim()) send({ t: 'chat', text: input.value });
  input.value = '';
});
results.querySelector('[data-action=results-done]')!.addEventListener('click', () => {
  results.hidden = true;
  if (online || screen === 'race') stopRace();
  show(room ? 'room' : 'lobby');
});

// ---- race -------------------------------------------------------------------

function describe(event: RaceEvent): string {
  switch (event.kind) {
    case 'lap': return `${event.name} · lap ${event.lap} ${formatTime(event.lapMs)}`;
    case 'finish': return `${event.name} finished ${ordinal(event.place)} · ${formatTime(event.timeMs)}`;
    case 'retire': return `${event.name} ${event.reason}`;
  }
}

function ordinal(n: number): string {
  const suffix = n % 100 >= 11 && n % 100 <= 13 ? 'th' : ['th', 'st', 'nd', 'rd'][n % 10] ?? 'th';
  return `${n}${suffix}`;
}

function feed(text: string) {
  const list = $<HTMLOListElement>('feed');
  const item = document.createElement('li');
  item.textContent = text;
  list.append(item);
  setTimeout(() => item.remove(), 6000);
  while (list.children.length > 5) list.firstElementChild!.remove();
}

async function startOnlineRace(message: Extract<ServerMessage, { t: 'raceStart' }>) {
  const rage = rageSync!;
  const s = message.settings;
  results.hidden = true;
  show('race');
  $('hud-hint').textContent = 'Preparing the course…';
  await new Promise(requestAnimationFrame);
  const humans = message.seats.slice(0, message.humans).map((seat) => ({ variant: seat.variant, manual: seat.manual }));
  const ok = rage.startNetRace({ classIndex: s.classIndex, course: s.course, reverse: s.reverse, laps: s.laps,
                                 rivals: s.rivals }, humans, message.localSeat);
  send({ t: 'loaded', ok });
  if (!ok) {
    show('room');
    toast('This race could not be prepared from your disc.');
    return;
  }
  $('hud-hint').textContent = 'Waiting for the other players…';
  players = message.seats.slice(0, message.humans).map((seat, index) => ({ seat: index, name: seat.name }));
  seatNames = message.seats.map((seat) => seat.name);
  localSeat = message.localSeat;
  if (localSeat < 0) $('hud-hint').textContent = 'Watching the race…';
  online = true;
  frames.reset();
  predicting = false;
  clockError = 0;
  beginRace(rage);
}

function beginRace(rage: Rage) {
  renderer ??= new Renderer(canvas, rage);
  resize();
  tachometer.prepare(rage);
  last = performance.now();
  accumulator = simTime = previousStep = currentStep = 0;
  paused = startHeld = false;
  phase = 0;
  clearKeyEdges();
  $('feed').replaceChildren();
  $('standings').replaceChildren();
  $('standings').hidden = !online;
  $('spectating').hidden = true;
  $('deadline').hidden = true;
  if (!online) {
    deadlineAt = null;
    localSeat = 0;
    seatNames = ['You', ...Array.from({ length: 11 }, (_, i) => `Rival ${i + 1}`)];
  }
  racing = true;
  audio?.startRace();
}

function stopRace() {
  racing = false;
  deadlineAt = null;
  audio?.stopRace();
  online = false;
  frames.reset();
}

// Offline practice against the retail field.
function fillPractice(rage: Rage) {
  const classSelect = setup.elements.namedItem('class') as HTMLSelectElement;
  const course = setup.elements.namedItem('course') as HTMLSelectElement;
  fillClasses(classSelect);
  classSelect.value = '2';
  const sync = () => {
    const classIndex = Number(classSelect.value);
    fillCourses(rage, course, classIndex);
    const cars = $<HTMLSelectElement>('car');
    const current = cars.value;
    cars.replaceChildren(...classCars(rage, classIndex).map((c) => new Option(c.label, String(c.variant))));
    if ([...cars.options].some((o) => o.value === current)) cars.value = current;
    syncTransmission();
  };
  classSelect.onchange = sync;
  sync();
}
function syncTransmission() {
  const car = Number($<HTMLSelectElement>('car').value);
  const transmission = setup.elements.namedItem('transmission') as HTMLSelectElement;
  const automatic = transmission.options[0];
  automatic.disabled = automaticCars[car] === false;
  if (automatic.disabled) transmission.value = '1';
}
$('car').addEventListener('change', syncTransmission);
setup.querySelector('[data-action=back-to-lobby]')!.addEventListener('click', () => {
  if (session.user) enterLobby();
  else show('auth');
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
  $('hud-hint').textContent = '';
  online = false;
  show('race');
  renderer ??= new Renderer(canvas, rage);
  renderer.shadows = (setup.elements.namedItem('shadows') as HTMLInputElement).checked;
  beginRace(rage);
});

function resize() {
  renderer?.resize(canvas.clientWidth, canvas.clientHeight);
}
addEventListener('resize', resize);

function drawHud(h: Hud) {
  $('hud-place').textContent = h.place > 0 ? `${h.place}/${h.entrants}` : `–/${h.entrants}`;
  $('hud-lap').textContent = `${Math.max(1, Math.min(h.lap || 1, h.laps))}/${h.laps}`;
  $('hud-time').textContent = formatTime(h.timeMs);
  $('hud-speed').textContent = String(h.speed);
  $('hud-gear').textContent = String(h.gear);
  const banner = $('hud-banner');
  if (h.phase === PHASE_COUNTDOWN) banner.textContent = String(Math.ceil(h.countdown / 50) || 'GO');
  else if (h.phase === PHASE_FINISHED || h.status === 2) banner.textContent = 'FINISH';
  else if (paused) banner.textContent = 'PAUSE';
  else banner.textContent = '';
}

let racing = false;
let last = 0;
let accumulator = 0;
/* Simulated time (ms) and the times of the last two physics steps: frames
 * are drawn between those two snapshots, one step behind the simulation. */
let simTime = 0;
let previousStep = 0;
let currentStep = 0;
/* Offline, Start pauses during the countdown and the race (race_scene_rules.c
 * CanPauseRace); online the server's clock does not stop. */
let paused = false;
let startHeld = false;
let phase = 0;
function frame(now: number) {
  requestAnimationFrame(frame);
  if (!racing || !renderer || screen !== 'race') return;
  const rage = rageSync;
  if (!rage) return;
  if (consumeKey('Escape')) {
    if (online) {
      send({ t: 'leaveRace' });
      stopRace();
      show(room ? 'room' : 'lobby');
    } else {
      racing = false;
      audio?.stopRace();
      show('setup');
      $('start').focus();
    }
    return;
  }
  if (consumeKey('KeyM')) audio?.toggleMute();
  const pad = samplePad();
  const start = (pad.held & PAD.START) !== 0;
  if (!online && start && !startHeld && (phase === PHASE_COUNTDOWN || phase === PHASE_RACING)) paused = !paused;
  startHeld = start;
  accumulator = paused ? 0 : Math.min(accumulator + (now - last), 250);
  last = now;
  rage.setPad(pad);
  if (online && localSeat >= 0) syncPrediction(rage);
  while (accumulator >= TICK_MS) {
    if (online) {
      if (localSeat >= 0) {
        // A player predicts: this tick's controls drive the local race and go to the server.
        const words = rage.takeInput();
        connection?.sendInput(words, rage.inputSeq());
      } else {
        // A spectator shows the server's frames through the jitter buffer.
        const due = frames.next();
        if (due) rage.applyFrame(due.data);
      }
    }
    phase = rage.tick();
    if (phase < 0) {
      stopRace();
      toast('The race stopped unexpectedly.');
      show(online ? 'room' : 'setup');
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
  const h = rage.hud();
  if (online && h.phase >= PHASE_RACING && $('hud-hint').textContent) $('hud-hint').textContent = '';
  audio?.update(h, paused);
  drawHud(h);
  if (now - standingsAt > 250) {
    standingsAt = now;
    if (online) drawStandings(rage);
    followRace(rage);
  }
  spectateKeys(rage);
  drawDeadline(now);
  renderer.render();
  tachometer.draw(rage);
}

// ---- prediction ------------------------------------------------------------------

/* Applies the newest server frame (rewind and replay) and keeps the local
 * clock just far enough ahead that each input reaches the server a tick
 * before it is needed: late inputs speed the clock up, early ones slow it. */
function syncPrediction(rage: Rage) {
  const latest = frames.takeLatest();
  if (!latest || localSeat * 2 + 1 >= latest.acks.length) return;
  const error = rage.applyPredicted(latest.data, latest.acks[localSeat * 2], latest.acks[localSeat * 2 + 1]);
  if (error === null) return;
  if (!predicting) {
    predicting = true; // the first frame only sets the starting point
    return;
  }
  const target = -1;
  if (error > target + 3) {
    // Far behind (start, or a network hiccup): catch up at once.
    accumulator += (error - target) * TICK_MS;
    clockError = target;
    return;
  }
  clockError = clockError * 0.9 + error * 0.1;
  if (clockError > target + 0.5) accumulator += TICK_MS * 0.05;
  else if (clockError < target - 1.5) accumulator -= TICK_MS * 0.05;
}

// ---- spectating ----------------------------------------------------------------

/* Cars still racing, in race order: players first when `playersFirst`. */
function racingSeats(rage: Rage, playersFirst: boolean): { seat: number; place: number; player: boolean }[] {
  const seats = [];
  for (let seat = 0; seat < seatNames.length; seat++) {
    const standing = rage.standing(seat);
    if (standing.status !== 1 || rage.seatGone(seat)) continue;
    seats.push({ seat, place: standing.place || 99, player: players.some((p) => p.seat === seat) || seat === localSeat });
  }
  return seats.sort((a, b) => (playersFirst ? Number(b.player) - Number(a.player) : 0) || a.place - b.place);
}

/* Once your car has finished and faded out (or retired, or you only watch),
 * the camera follows the player nearest to you in the race; with no player
 * left, the nearest rival. A followed car that leaves the picture hands over
 * the same way. */
function followRace(rage: Rage) {
  const view = rage.viewSeat();
  const own = localSeat >= 0 && !rage.seatGone(localSeat);
  if (own) {
    if (view !== localSeat) rage.setViewSeat(localSeat);
  } else if (view === localSeat || rage.seatGone(view)) {
    const reference = rage.standing(localSeat >= 0 ? localSeat : view).place || 1;
    const candidates = racingSeats(rage, true);
    const pool = candidates.some((c) => c.player) ? candidates.filter((c) => c.player) : candidates;
    pool.sort((a, b) => Math.abs(a.place - reference) - Math.abs(b.place - reference));
    if (pool.length) rage.setViewSeat(pool[0].seat);
  }
  const spectating = rage.viewSeat() !== localSeat;
  const label = $('spectating');
  label.hidden = !spectating;
  if (spectating) {
    const name = seatNames[rage.viewSeat()] ?? '';
    label.replaceChildren(`Watching ${name}`, Object.assign(document.createElement('span'), { textContent: '  ← → switch car' }));
  }
}

/* ←/→ (or the D-pad) pick the previous/next car in race order. */
function spectateKeys(rage: Rage) {
  const previous = consumeKey('ArrowLeft');
  const next = consumeKey('ArrowRight');
  if ((!previous && !next) || rage.viewSeat() === localSeat) return;
  const order = racingSeats(rage, false);
  if (!order.length) return;
  const at = order.findIndex((c) => c.seat === rage.viewSeat());
  const step = next ? 1 : -1;
  rage.setViewSeat(order[(Math.max(at, 0) + step + order.length) % order.length].seat);
  standingsAt = 0;
}

/* The race closes this long after the first car finished (bottom left). */
function drawDeadline(now: number) {
  const element = $('deadline');
  element.hidden = deadlineAt === null;
  if (deadlineAt !== null) element.textContent = `Race closes in ${formatTime(Math.max(0, deadlineAt - now)).slice(0, -3)}`;
}

/* The online players in race order, styled like the event feed. */
function drawStandings(rage: Rage) {
  const list = $<HTMLOListElement>('standings');
  const rows = players.map((p) => ({ ...p, ...rage.standing(p.seat) }))
    .sort((a, b) => (a.place || 99) - (b.place || 99) || a.seat - b.seat);
  list.replaceChildren(...rows.map((row) => {
    const item = document.createElement('li');
    if (row.seat === localSeat) item.className = 'me';
    const place = document.createElement('b');
    place.textContent = row.status === 3 ? 'RET' : row.place ? String(row.place) : '–';
    const name = document.createElement('span');
    name.textContent = row.name;
    const state = document.createElement('span');
    state.className = 'dim';
    state.textContent = row.status === 2 ? 'finished' : row.status === 3 ? '' : `lap ${Math.max(1, row.lap)}`;
    item.append(place, name, state);
    return item;
  }));
}

// ---- start ------------------------------------------------------------------

void ragePromise.then(async (rage) => {
  rageSync = rage;
  // The automated browser checks (scripts/e2e.mjs) inspect the race.
  if (location.hash === '#e2e') Object.assign(window, { __race: { rage, names: () => seatNames } });
  audio = new RaceAudio(rage);
  $('disc-status').dataset.ready = '1';
  if (await session.resume()) afterLogin();
  else show('auth');
});
show('auth');
$('auth').hidden = true; // until the stored session has been checked
requestAnimationFrame(frame);
