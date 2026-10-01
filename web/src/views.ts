// DOM for the lobby, rooms, the race forms and results. Car and course rules
// come from the WebAssembly module (web_rules.c), the same code the server
// validates with.
import type {
  RaceResult, RecordRow, RoomSettings, RoomState, RoomSummary, Transport, UserInfo,
} from '../shared/protocol.ts';
import type { HistoryRow } from './net';
import type { Rage, RaceOptions } from './rage';

export const $ = <T extends HTMLElement>(id: string) => document.getElementById(id) as T;

const CLASS_NAMES = ['Class 1', 'Class 2', 'Class 3', 'Class 4', 'Class 5', 'Extra class'];

/** What a new room and the practice form start with. */
const DEFAULT_SETTINGS: Readonly<RoomSettings> = {
  name: '', classIndex: 2, course: 0, reverse: false, laps: 3, rivals: true, maxPlayers: 4,
};

export function formatTime(ms: number): string {
  if (ms < 0) return '--:--.--';
  const minutes = Math.floor(ms / 60000);
  const seconds = Math.floor(ms / 1000) % 60;
  const hundredths = Math.floor(ms / 10) % 100;
  return `${minutes}:${String(seconds).padStart(2, '0')}.${String(hundredths).padStart(2, '0')}`;
}

export function el<K extends keyof HTMLElementTagNameMap>(tag: K, props: Partial<HTMLElementTagNameMap[K]> = {},
                                                   ...children: (Node | string)[]): HTMLElementTagNameMap[K] {
  const node = Object.assign(document.createElement(tag), props);
  node.append(...children);
  return node;
}

/** Sets an element's text only when it changes (for per-frame updates). */
export function textWriter(node: HTMLElement): (text: string) => void {
  let shown = node.textContent ?? '';
  return (text) => {
    if (text !== shown) node.textContent = shown = text;
  };
}

function row(cells: (Node | string)[], header = false): HTMLTableRowElement {
  return el('tr', {}, ...cells.map((cell) => el(header ? 'th' : 'td', {}, cell)));
}

function courseLabel(rage: Rage, course: number, reverse: boolean): string {
  return `${rage.courseName(course)}${reverse ? ' · reverse' : ''}`;
}

function carLabel(rage: Rage, variant: number): string {
  return variant < 0 ? '—' : `${rage.carName(variant)} ${'I'.repeat(rage.carGrade(variant) + 1)}`;
}

/* What the car pickers add to a car's name: the garage marks the cars the
 * player painted (set by garage.ts, which knows the paints). */
let carNote: (variant: number) => string = () => '';
export function setCarNote(note: (variant: number) => string): void { carNote = note; }

/** The class that offers a car variant, or -1. */
export function classOf(rage: Rage, variant: number): number {
  for (let classIndex = 0; classIndex < CLASS_NAMES.length; classIndex++) {
    for (let model = 0; model < rage.carModels(); model++) if (rage.classCar(classIndex, model) === variant) return classIndex;
  }
  return -1;
}

/** Models a class offers, as (variant, label). */
function classCars(rage: Rage, classIndex: number): { variant: number; label: string }[] {
  const cars = [];
  for (let model = 0; model < rage.carModels(); model++) {
    const variant = rage.classCar(classIndex, model);
    if (variant >= 0) cars.push({ variant, label: carLabel(rage, variant) });
  }
  return cars;
}

// ---- race forms (practice and room settings) --------------------------------

const field = <T>(form: HTMLFormElement, name: string) => form.elements.namedItem(name) as unknown as T;

/** Course options with the ones a class does not race disabled; selects
 *  `wanted` when the class races it. */
function fillCourses(rage: Rage, select: HTMLSelectElement, classIndex: number, wanted: number): void {
  select.replaceChildren(...[0, 1, 2, 3].map((course) => {
    const option = new Option(rage.courseName(course), String(course));
    option.disabled = !rage.courseAllowed(classIndex, course);
    return option;
  }));
  select.value = String(rage.courseAllowed(classIndex, wanted) ? wanted : 0);
}

/** The class's models (marked as the garage says); the selection stays when
 *  the new list has it. */
function fillCars(rage: Rage, select: HTMLSelectElement, classIndex: number): void {
  const cars = classCars(rage, classIndex).map((c) => ({ ...c, label: c.label + carNote(c.variant) }));
  const key = cars.map((c) => `${c.variant}${c.label}`).join(',');
  if (select.dataset.cars === key) return;
  const current = select.value;
  select.replaceChildren(...cars.map((c) => new Option(c.label, String(c.variant))));
  select.dataset.cars = key;
  if (cars.some((c) => String(c.variant) === current)) select.value = current;
}

/** Automatic (the first option) only where the disc offers it for the car. */
function syncTransmission(select: HTMLSelectElement, automatic: boolean): void {
  select.options[0].disabled = !automatic;
  if (!automatic) select.value = 'manual';
}

/** Puts settings into the fields both race forms share. */
function writeRaceFields(rage: Rage, form: HTMLFormElement, s: Readonly<RoomSettings>): void {
  const classSelect = field<HTMLSelectElement>(form, 'class');
  classSelect.replaceChildren(...CLASS_NAMES.map((name, index) => new Option(name, String(index))));
  classSelect.value = String(s.classIndex);
  fillCourses(rage, field(form, 'course'), s.classIndex, s.course);
  field<HTMLSelectElement>(form, 'reverse').value = s.reverse ? '1' : '0';
  field<HTMLSelectElement>(form, 'laps').value = String(s.laps);
  field<HTMLInputElement>(form, 'rivals').checked = s.rivals;
}

function readRaceFields(form: HTMLFormElement): Omit<RoomSettings, 'name' | 'maxPlayers'> {
  const value = (name: string) => field<HTMLSelectElement>(form, name).value;
  return {
    classIndex: Number(value('class')), course: Number(value('course')), reverse: value('reverse') === '1',
    laps: Number(value('laps')), rivals: field<HTMLInputElement>(form, 'rivals').checked,
  };
}

/** Offline practice against the retail field. */
export function fillPractice(rage: Rage, form: HTMLFormElement, automatic: boolean[]): void {
  const classSelect = field<HTMLSelectElement>(form, 'class');
  const course = field<HTMLSelectElement>(form, 'course');
  const car = field<HTMLSelectElement>(form, 'car');
  const syncCar = () => syncTransmission(field(form, 'transmission'), automatic[Number(car.value)] !== false);
  const syncClass = () => {
    const classIndex = Number(classSelect.value);
    fillCourses(rage, course, classIndex, Number(course.value));
    fillCars(rage, car, classIndex);
    syncCar();
  };
  writeRaceFields(rage, form, DEFAULT_SETTINGS);
  classSelect.onchange = syncClass;
  car.onchange = syncCar;
  syncClass();
}

/** Opens the practice form on a car: its class, and the car itself. */
export function choosePracticeCar(rage: Rage, form: HTMLFormElement, variant: number): void {
  const classSelect = field<HTMLSelectElement>(form, 'class');
  const classIndex = variant >= 0 ? classOf(rage, variant) : -1;
  if (classIndex >= 0 && Number(classSelect.value) !== classIndex) classSelect.value = String(classIndex);
  classSelect.onchange?.(new Event('change')); // refills courses and cars (and their marks)
  const car = field<HTMLSelectElement>(form, 'car');
  if (classIndex >= 0) car.value = String(variant);
  car.onchange?.(new Event('change'));
}

export function readPractice(form: HTMLFormElement): RaceOptions & { shadows: boolean } {
  return {
    ...readRaceFields(form),
    car: Number(field<HTMLSelectElement>(form, 'car').value),
    manual: field<HTMLSelectElement>(form, 'transmission').value === 'manual',
    shadows: field<HTMLInputElement>(form, 'shadows').checked,
  };
}

/** Opens the room settings dialog; resolves with the settings or null. */
export function editSettings(rage: Rage, initial: RoomSettings | null, minPlayers: number): Promise<RoomSettings | null> {
  const dialog = $<HTMLDialogElement>('settings-dialog');
  const form = $<HTMLFormElement>('settings-form');
  const classSelect = field<HTMLSelectElement>(form, 'class');
  const course = field<HTMLSelectElement>(form, 'course');
  const reverse = field<HTMLSelectElement>(form, 'reverse');
  const players = field<HTMLSelectElement>(form, 'maxPlayers');
  const name = field<HTMLInputElement>(form, 'name');
  $('settings-title').textContent = initial ? 'Room settings' : 'Create room';
  $('settings-submit').textContent = initial ? 'Save' : 'Create';
  const s = initial ?? DEFAULT_SETTINGS;
  name.value = s.name;
  writeRaceFields(rage, form, s);

  const sync = (wantedPlayers = Number(players.value)) => {
    const classIndex = Number(classSelect.value);
    fillCourses(rage, course, classIndex, Number(course.value));
    const most = Math.max(1, rage.maxHumans(classIndex, Number(course.value), reverse.value === '1'));
    players.replaceChildren(...Array.from({ length: most - minPlayers + 1 }, (_, i) => new Option(String(minPlayers + i))));
    players.value = String(Math.min(most, Math.max(minPlayers, wantedPlayers)));
    $('settings-cars').textContent = `Cars in ${CLASS_NAMES[classIndex]}: ${classCars(rage, classIndex).map((c) => c.label).join(', ')}.`;
  };
  sync(s.maxPlayers);
  classSelect.onchange = course.onchange = reverse.onchange = () => sync();

  dialog.showModal();
  name.focus();
  return new Promise((resolve) => {
    dialog.onclose = () => {
      if (dialog.returnValue !== 'ok') return resolve(null);
      resolve({ ...readRaceFields(form), name: name.value.trim(), maxPlayers: Number(players.value) });
    };
  });
}

// ---- lobby, room and results --------------------------------------------------

export function renderRooms(rage: Rage, rooms: RoomSummary[], join: (id: number) => void): void {
  const list = $<HTMLUListElement>('room-list');
  if (!rooms.length) {
    list.replaceChildren(el('li', { className: 'empty' }, 'No rooms yet. Create one, or start a duel and send the link.'));
    return;
  }
  list.replaceChildren(...rooms.map((room) => {
    const s = room.settings;
    // A full or racing room is joined as a spectator.
    const open = room.status === 'lobby' && room.players < s.maxPlayers;
    const button = el('button', { type: 'button', textContent: open ? 'Join' : 'Watch' });
    if (!open) button.className = 'secondary';
    button.addEventListener('click', () => join(room.id));
    return el('li', {},
      el('div', { className: 'room-main' },
        el('b', {}, s.name),
        el('span', { className: 'dim' }, `${courseLabel(rage, s.course, s.reverse)} · ${CLASS_NAMES[s.classIndex]} · ${s.laps} ${s.laps === 1 ? 'lap' : 'laps'}${s.rivals ? ' · rivals' : ''}`)),
      el('span', { className: 'dim' }, `${room.players}/${s.maxPlayers}${room.spectators ? ` +${room.spectators} watching` : ''} · ${room.host}`),
      button);
  }));
}

export function renderRecords(rage: Rage, records: RecordRow[]): void {
  const table = $<HTMLTableElement>('records');
  table.replaceChildren(row(['Course', 'Class', 'Best lap', 'Driver', 'Car'], true),
    ...records.map((r) => row([courseLabel(rage, r.course, r.reverse), CLASS_NAMES[r.classIndex],
                               formatTime(r.bestLapMs), r.name, carLabel(rage, r.variant)])));
  if (!records.length) table.append(el('tr', {}, el('td', { colSpan: 5, className: 'dim' }, 'No laps recorded yet.')));
}

export function renderHistory(rage: Rage, history: HistoryRow[]): void {
  const table = $<HTMLTableElement>('history');
  table.replaceChildren(row(['Course', 'Class', 'Place', 'Time', 'Best lap'], true),
    ...history.map((h) => row([courseLabel(rage, h.course, h.reverse), CLASS_NAMES[h.classIndex],
      h.status === 'retired' ? 'retired' : `${h.place}/${h.entrants}`,
      h.status === 'finished' ? formatTime(h.timeMs) : '—', formatTime(h.bestLapMs)])));
  if (!history.length) table.append(el('tr', {}, el('td', { colSpan: 5, className: 'dim' }, 'Your races will show up here.')));
}

/** A round trip for the ping columns (null while reconnecting, undefined
 *  before the first measurement), marked when the player races over the
 *  WebSocket fallback instead of the data channel. */
export function pingCell(ms: number | null | undefined, online = ms !== null, transport?: Transport): HTMLElement {
  const fallback = online && ms != null && transport === 'ws';
  const cell = el('span', { className: fallback ? 'ping fallback' : 'ping dim' },
    !online ? 'offline' : ms == null ? '' : `${ms} ms${fallback ? ' ⚠' : ''}`);
  if (fallback) cell.title = 'On the fallback connection: a lost packet holds up the ones behind it, so the race may stutter.';
  return cell;
}

export function renderRoom(rage: Rage, room: RoomState, me: UserInfo, automatic: boolean[],
                           latency: Record<number, number | null>, transport: Record<number, Transport> = {}): void {
  const s = room.settings;
  const host = room.members.find((m) => m.host);
  const mine = room.members.find((m) => m.userId === me.id);
  const isHost = host?.userId === me.id;
  $('room-name').textContent = s.name;
  $('room-summary').textContent =
    `${courseLabel(rage, s.course, s.reverse)} · ${CLASS_NAMES[s.classIndex]} · ${s.laps} ${s.laps === 1 ? 'lap' : 'laps'}`;
  $<HTMLButtonElement>('room').querySelector<HTMLButtonElement>('[data-action=edit-room]')!.hidden = !isHost || room.duel !== null;
  $<HTMLButtonElement>('room').querySelector<HTMLButtonElement>('[data-action=close-room]')!.hidden = !isHost && !me.admin;

  const racers = room.members.filter((m) => !m.spectator);
  const watchers = room.members.filter((m) => m.spectator);
  const players = $<HTMLTableElement>('room-players');
  const duel = room.duel !== null;
  const headers = duel
    ? ['#', 'Driver', 'Car', 'Gearbox', 'Tires', '', 'Ping']
    : ['#', 'Driver', 'Car', 'Gearbox', '', 'Ping'];
  players.replaceChildren(row(headers, true),
    ...racers.map((m, index) => row([String(index + 1), `${m.name}${m.host ? ' ★' : ''}`,
      carLabel(rage, m.variant), m.manual ? 'MT' : 'AT',
      ...(duel ? [String(m.tire + 1)] : []),
      el('span', { className: m.ready ? 'ok' : 'dim' }, m.ready ? 'ready' : 'choosing'),
      pingCell(latency[m.userId], m.online, transport[m.userId])])));
  if (watchers.length) {
    players.append(el('tr', {}, el('td', { colSpan: headers.length, className: 'dim' },
      `Watching: ${watchers.map((m) => `${m.name}${m.host ? ' ★' : ''}`).join(', ')}`)));
  }
  const free = s.maxPlayers - racers.length;
  $('room-fill').textContent = room.duel
    ? 'Same car for both of you. Choose your gearbox and tires, then both press Ready.'
    : s.rivals
      ? `${free > 0 ? `${free} more ${free === 1 ? 'player' : 'players'} can join; ` : ''}the rest of the grid races the retail rivals.`
      : `${free > 0 ? `${free} more ${free === 1 ? 'player' : 'players'} can join. ` : ''}No rivals: only players race.`;
  const invite = $('duel-invite');
  if (room.duel) {
    invite.hidden = false;
    $('duel-car').textContent = `Both drivers: ${carLabel(rage, room.duel.variant)}.`;
    const link = $<HTMLInputElement>('duel-link');
    const url = duelUrl(room.duel.token);
    if (link.value !== url) link.value = url;
  } else {
    invite.hidden = true;
  }

  // Car picker: the class's models; automatic only where the disc offers it.
  const model = $<HTMLSelectElement>('car-model');
  const transmission = $<HTMLSelectElement>('car-transmission');
  const tires = $<HTMLSelectElement>('car-tires');
  fillCars(rage, model, s.classIndex);
  if (mine && mine.variant >= 0) model.value = String(mine.variant);
  transmission.value = mine?.manual ? 'manual' : 'auto';
  syncTransmission(transmission, automatic[Number(model.value)] !== false);
  tires.value = String(mine?.tire ?? 0);
  const racing = room.status !== 'lobby';
  const watching = mine?.spectator === true;
  model.disabled = racing || watching || duel;
  transmission.disabled = tires.disabled = racing || watching;
  $('car-picker').hidden = watching;
  $('car-model-field').hidden = duel;
  $('car-tires-field').hidden = !duel;
  const spectate = $<HTMLButtonElement>('spectate-button');
  spectate.hidden = duel;
  spectate.textContent = watching ? 'Join the grid' : 'Watch only';
  spectate.disabled = racing || (watching && free <= 0);
  $<HTMLButtonElement>('watch-button').hidden = !racing;

  const ready = $<HTMLButtonElement>('ready-button');
  const start = $<HTMLButtonElement>('start-button');
  ready.hidden = watching || (isHost && !duel);
  ready.textContent = mine?.ready ? 'Not ready' : 'Ready';
  ready.disabled = racing;
  start.hidden = !isHost;
  const waiting = racers.filter((m) => !m.ready).map((m) => m.name);
  const alone = duel && racers.length < 2;
  start.disabled = racing || waiting.length > 0 || racers.length === 0 || alone;
  $('room-status').textContent = racing ? 'The race is on…'
    : alone ? 'Send the link. The duel starts when the other driver is here and ready.'
    : waiting.length ? `Waiting for ${waiting.join(', ')}.` : isHost ? 'Everyone is ready.' : 'Waiting for the host to start.';
}

/** The page link that puts the other driver in this duel. */
export function duelUrl(token: string): string {
  const url = new URL(location.href);
  url.hash = '';
  url.search = '';
  url.searchParams.set('duel', token);
  return url.href;
}

/** The room's car picker changed: the choice to send, Automatic only where offered. */
export function readRoomCar(automatic: boolean[]): { variant: number; manual: boolean; tire: number } {
  const variant = Number($<HTMLSelectElement>('car-model').value);
  const transmission = $<HTMLSelectElement>('car-transmission');
  syncTransmission(transmission, automatic[variant] !== false);
  const tire = Number($<HTMLSelectElement>('car-tires').value);
  return { variant, manual: transmission.value === 'manual', tire: Number.isInteger(tire) ? tire : 0 };
}

export function appendChat(from: string, text: string, at: number): void {
  const log = $<HTMLOListElement>('chat-log');
  const time = new Date(at).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  log.append(from
    ? el('li', {}, el('span', { className: 'dim' }, `${time} `), el('b', {}, `${from}: `), text)
    : el('li', { className: 'system' }, `${time} ${text}`));
  while (log.children.length > 200) log.firstElementChild!.remove();
  log.scrollTop = log.scrollHeight;
}

export function renderResults(rage: Rage, results: RaceResult[], me: UserInfo): void {
  $<HTMLTableElement>('results-table').replaceChildren(row(['Place', 'Driver', 'Car', 'Time', 'Best lap'], true),
    ...results.map((r) => {
      const tr = row([r.status === 'retired' ? 'RET' : r.place ? String(r.place) : '—', r.name,
        r.variant >= 0 ? carLabel(rage, r.variant) : 'rival',
        r.status === 'finished' ? formatTime(r.timeMs) : r.status === 'retired' ? 'retired' : 'did not finish',
        formatTime(r.bestLapMs)]);
      if (r.userId === me.id) tr.className = 'me';
      else if (r.userId === null) tr.className = 'dim';
      return tr;
    }));
}
