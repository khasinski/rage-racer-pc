// DOM for the lobby, rooms, room settings and results. Car and course rules
// come from the WebAssembly module (web_rules.c), the same code the server
// validates with.
import type {
  RaceResult, RecordRow, RoomSettings, RoomState, RoomSummary, UserInfo,
} from '../shared/protocol.ts';
import type { Rage } from './rage';

export const $ = <T extends HTMLElement>(id: string) => document.getElementById(id) as T;

export const CLASS_NAMES = ['Class 1', 'Class 2', 'Class 3', 'Class 4', 'Class 5', 'Extra class'];

export function formatTime(ms: number): string {
  if (ms < 0) return '--:--.--';
  const minutes = Math.floor(ms / 60000);
  const seconds = Math.floor(ms / 1000) % 60;
  const hundredths = Math.floor(ms / 10) % 100;
  return `${minutes}:${String(seconds).padStart(2, '0')}.${String(hundredths).padStart(2, '0')}`;
}

function el<K extends keyof HTMLElementTagNameMap>(tag: K, props: Partial<HTMLElementTagNameMap[K]> = {},
                                                   ...children: (Node | string)[]): HTMLElementTagNameMap[K] {
  const node = Object.assign(document.createElement(tag), props);
  node.append(...children);
  return node;
}

function row(cells: (Node | string)[], header = false): HTMLTableRowElement {
  return el('tr', {}, ...cells.map((cell) => el(header ? 'th' : 'td', {}, cell)));
}

export function courseLabel(rage: Rage, course: number, reverse: boolean): string {
  return `${rage.courseName(course)}${reverse ? ' · reverse' : ''}`;
}

export function carLabel(rage: Rage, variant: number): string {
  return variant < 0 ? '—' : `${rage.carName(variant)} ${'I'.repeat(rage.carGrade(variant) + 1)}`;
}

/** Models a class offers, as (variant, label). */
export function classCars(rage: Rage, classIndex: number): { variant: number; label: string }[] {
  const cars = [];
  for (let model = 0; model < rage.carModels(); model++) {
    const variant = rage.classCar(classIndex, model);
    if (variant >= 0) cars.push({ variant, label: carLabel(rage, variant) });
  }
  return cars;
}

export function fillClasses(select: HTMLSelectElement): void {
  select.replaceChildren(...CLASS_NAMES.map((name, index) => new Option(name, String(index))));
}

/** Course options with the ones a class does not race disabled. */
export function fillCourses(rage: Rage, select: HTMLSelectElement, classIndex: number): void {
  const current = Number(select.value || 0);
  select.replaceChildren(...[0, 1, 2, 3].map((course) => {
    const option = new Option(rage.courseName(course), String(course));
    option.disabled = !rage.courseAllowed(classIndex, course);
    return option;
  }));
  select.value = String(rage.courseAllowed(classIndex, current) ? current : 0);
}

export function renderRooms(rage: Rage, rooms: RoomSummary[], join: (id: number) => void): void {
  const list = $<HTMLUListElement>('room-list');
  if (!rooms.length) {
    list.replaceChildren(el('li', { className: 'empty' }, 'No rooms yet. Create one and invite your friends.'));
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

export function renderHistory(rage: Rage, history: Array<{ course: number; reverse: boolean; classIndex: number;
    place: number; entrants: number; timeMs: number; bestLapMs: number; status: string }>): void {
  const table = $<HTMLTableElement>('history');
  table.replaceChildren(row(['Course', 'Class', 'Place', 'Time', 'Best lap'], true),
    ...history.map((h) => row([courseLabel(rage, h.course, h.reverse), CLASS_NAMES[h.classIndex],
      h.status === 'retired' ? 'retired' : `${h.place}/${h.entrants}`,
      h.status === 'finished' ? formatTime(h.timeMs) : '—', formatTime(h.bestLapMs)])));
  if (!history.length) table.append(el('tr', {}, el('td', { colSpan: 5, className: 'dim' }, 'Your races will show up here.')));
}

export interface RoomHandlers {
  setCar(variant: number, manual: boolean): void;
}

export function renderRoom(rage: Rage, room: RoomState, me: UserInfo, automatic: boolean[]): void {
  const s = room.settings;
  const host = room.members.find((m) => m.host);
  const mine = room.members.find((m) => m.userId === me.id);
  const isHost = host?.userId === me.id;
  $('room-name').textContent = s.name;
  $('room-summary').textContent =
    `${courseLabel(rage, s.course, s.reverse)} · ${CLASS_NAMES[s.classIndex]} · ${s.laps} ${s.laps === 1 ? 'lap' : 'laps'}`;
  $<HTMLButtonElement>('room').querySelector<HTMLButtonElement>('[data-action=edit-room]')!.hidden = !isHost;
  $<HTMLButtonElement>('room').querySelector<HTMLButtonElement>('[data-action=close-room]')!.hidden = !isHost && !me.admin;

  const racers = room.members.filter((m) => !m.spectator);
  const watchers = room.members.filter((m) => m.spectator);
  const players = $<HTMLTableElement>('room-players');
  const ping = (m: { online: boolean; latencyMs: number | null }) =>
    el('span', { className: 'dim' }, !m.online ? 'offline' : m.latencyMs === null ? '' : `${m.latencyMs} ms`);
  players.replaceChildren(row(['#', 'Driver', 'Car', 'Gearbox', '', 'Ping'], true),
    ...racers.map((m, index) => row([String(index + 1), `${m.name}${m.host ? ' ★' : ''}`,
      carLabel(rage, m.variant), m.manual ? 'MT' : 'AT',
      el('span', { className: m.ready ? 'ok' : 'dim' }, m.ready ? 'ready' : 'choosing'), ping(m)])));
  if (watchers.length) {
    players.append(el('tr', {}, el('td', { colSpan: 6, className: 'dim' },
      `Watching: ${watchers.map((m) => `${m.name}${m.host ? ' ★' : ''}`).join(', ')}`)));
  }
  const free = s.maxPlayers - racers.length;
  $('room-fill').textContent = s.rivals
    ? `${free > 0 ? `${free} more ${free === 1 ? 'player' : 'players'} can join; ` : ''}the rest of the grid races the retail rivals.`
    : `${free > 0 ? `${free} more ${free === 1 ? 'player' : 'players'} can join. ` : ''}No rivals: only players race.`;

  // Car picker: the class's models; automatic only where the disc offers it.
  const model = $<HTMLSelectElement>('car-model');
  const transmission = $<HTMLSelectElement>('car-transmission');
  const cars = classCars(rage, s.classIndex);
  const key = cars.map((c) => c.variant).join(',');
  if (model.dataset.cars !== key) {
    model.replaceChildren(...cars.map((c) => new Option(c.label, String(c.variant))));
    model.dataset.cars = key;
  }
  if (mine && mine.variant >= 0) model.value = String(mine.variant);
  const autoOption = transmission.options[0];
  autoOption.disabled = automatic[Number(model.value)] === false;
  transmission.value = mine?.manual || autoOption.disabled ? 'manual' : 'auto';
  const racing = room.status !== 'lobby';
  const watching = mine?.spectator === true;
  model.disabled = transmission.disabled = racing || watching;
  $('car-picker').hidden = watching;
  const spectate = $<HTMLButtonElement>('spectate-button');
  spectate.textContent = watching ? 'Join the grid' : 'Watch only';
  spectate.disabled = racing || (watching && free <= 0);
  $<HTMLButtonElement>('watch-button').hidden = !racing;

  const ready = $<HTMLButtonElement>('ready-button');
  const start = $<HTMLButtonElement>('start-button');
  ready.hidden = isHost || watching;
  ready.textContent = mine?.ready ? 'Not ready' : 'Ready';
  ready.disabled = racing;
  start.hidden = !isHost;
  const waiting = racers.filter((m) => !m.ready).map((m) => m.name);
  start.disabled = racing || waiting.length > 0 || racers.length === 0;
  $('room-status').textContent = racing ? 'The race is on…'
    : waiting.length ? `Waiting for ${waiting.join(', ')}.` : isHost ? 'Everyone is ready.' : 'Waiting for the host to start.';
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

/** Opens the room settings dialog; resolves with the settings or null. */
export function editSettings(rage: Rage, initial: RoomSettings | null, minPlayers: number): Promise<RoomSettings | null> {
  const dialog = $<HTMLDialogElement>('settings-dialog');
  const form = $<HTMLFormElement>('settings-form');
  const field = <T extends HTMLElement>(name: string) => form.elements.namedItem(name) as unknown as T;
  const classSelect = field<HTMLSelectElement>('class');
  const course = field<HTMLSelectElement>('course');
  const reverse = field<HTMLSelectElement>('reverse');
  const players = field<HTMLSelectElement>('maxPlayers');
  $('settings-title').textContent = initial ? 'Room settings' : 'Create room';
  $('settings-submit').textContent = initial ? 'Save' : 'Create';
  fillClasses(classSelect);
  const s = initial ?? { name: '', classIndex: 2, course: 0, reverse: false, laps: 3, rivals: true, maxPlayers: 4 };
  field<HTMLInputElement>('name').value = s.name;
  classSelect.value = String(s.classIndex);
  course.value = String(s.course);
  fillCourses(rage, course, s.classIndex);
  course.value = String(rage.courseAllowed(s.classIndex, s.course) ? s.course : 0);
  reverse.value = s.reverse ? '1' : '0';
  field<HTMLSelectElement>('laps').value = String(s.laps);
  field<HTMLInputElement>('rivals').checked = s.rivals;

  const sync = () => {
    const classIndex = Number(classSelect.value);
    fillCourses(rage, course, classIndex);
    const most = Math.max(1, rage.maxHumans(classIndex, Number(course.value), reverse.value === '1'));
    const wanted = Number(players.value || s.maxPlayers);
    players.replaceChildren(...Array.from({ length: most - minPlayers + 1 }, (_, i) => new Option(String(minPlayers + i))));
    players.value = String(Math.min(most, Math.max(minPlayers, wanted)));
    $('settings-cars').textContent = `Cars in ${CLASS_NAMES[classIndex]}: ${classCars(rage, classIndex).map((c) => c.label).join(', ')}.`;
  };
  players.replaceChildren(new Option(String(s.maxPlayers)));
  players.value = String(s.maxPlayers);
  sync();
  classSelect.onchange = course.onchange = reverse.onchange = sync;

  dialog.showModal();
  field<HTMLInputElement>('name').focus();
  return new Promise((resolve) => {
    dialog.onclose = () => {
      if (dialog.returnValue !== 'ok') return resolve(null);
      resolve({
        name: field<HTMLInputElement>('name').value.trim(), classIndex: Number(classSelect.value),
        course: Number(course.value), reverse: reverse.value === '1', laps: Number(field<HTMLSelectElement>('laps').value),
        rivals: field<HTMLInputElement>('rivals').checked, maxPlayers: Number(players.value),
      });
    };
  });
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
