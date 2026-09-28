// The online side: the server connection, the lobby and room, and the
// network part of an online race (a player predicts their own car from the
// newest server frame; a spectator plays the frames through a jitter buffer).
import type { RaceEvent, RoomState, ServerMessage } from '../shared/protocol.ts';
import { app, session, setHint, show, toast } from './app';
import { TICK_MS } from './constants';
import { Connection, FrameBuffer, type Frame } from './net';
import { feed } from './overlay';
import { beginRace, race, racing, stopRace, type RaceNet } from './race';
import type { Rage } from './rage';
import {
  $, appendChat, editSettings, formatTime, readRoomCar, renderHistory, renderRecords, renderResults, renderRoom,
  renderRooms,
} from './views';

let connection: Connection | null = null;
let room: RoomState | null = null;
let reconnectDelay = 1000;
/* A spectator's frames. */
const frames = new FrameBuffer();
/* A player's prediction: the newest server frame not applied yet, whether
 * the local race runs ahead of the server yet, and the smoothed clock error
 * (ticks an input was used later than predicted). */
let latest: Frame | null = null;
let predicting = false;
let clockError = 0;

const results = $('results');
const onlineRace = () => racing && race.mode !== 'offline';

export function enterLobby(): void {
  connect();
  show(room ? 'room' : 'lobby');
  void refreshTables();
}

async function refreshTables() {
  const rage = app.rage;
  if (!rage) return;
  try {
    renderRecords(rage, (await session.records()).records);
    renderHistory(rage, (await session.history()).history);
  } catch { /* tables stay as they were */ }
}

function connect() {
  if (connection || !session.token) return;
  connection = new Connection(session.token);
  connection.onMessage = onMessage;
  connection.onFrame = (frame) => {
    if (!racing) return;
    if (race.mode === 'player') latest = frame;
    else if (race.mode === 'spectator') frames.push(frame);
  };
  connection.onClose = (reason) => {
    connection = null;
    if (!session.token) return;
    // The server keeps your place for a while: stay on the race or room and
    // reconnect; it puts you back (and back in your car) when you return.
    if (app.screen === 'race' && onlineRace()) setHint('Connection lost — reconnecting…');
    else toast(`${reason} Reconnecting…`);
    setTimeout(() => { if (!connection && session.token && app.discLoaded) connect(); }, reconnectDelay);
    reconnectDelay = Math.min(5000, reconnectDelay * 1.5);
  };
}

export function disconnect(): void {
  connection?.close();
  connection = null;
}

export function send(message: Parameters<Connection['send']>[0]): void {
  connection?.send(message);
}

function onMessage(message: ServerMessage) {
  const rage = app.rage!;
  switch (message.t) {
    case 'welcome':
      session.discId = message.discId;
      $('lobby-status').textContent = '';
      reconnectDelay = 1000;
      if (app.screen === 'race' && onlineRace()) setHint('');
      break;
    case 'latency':
      app.latency = message.latency;
      if (room && app.screen === 'room') {
        for (const m of room.members) m.online = app.latency[m.userId] !== null;
        renderRoom(rage, room, session.user!, app.automaticCars, app.latency);
      }
      break;
    case 'rooms':
      renderRooms(rage, message.rooms, (id) => send({ t: 'joinRoom', roomId: id }));
      break;
    case 'room':
      if (!message.room) {
        room = null;
        $('chat-log').replaceChildren();
        if (app.screen === 'room') show('lobby');
        void refreshTables();
        break;
      }
      if (!room || room.id !== message.room.id) $('chat-log').replaceChildren();
      room = message.room;
      renderRoom(rage, room, session.user!, app.automaticCars, app.latency);
      if (app.screen === 'lobby') show('room');
      break;
    case 'chat':
      appendChat(message.from, message.text, message.at);
      break;
    case 'raceStart':
      void startOnlineRace(message);
      break;
    case 'raceGo':
      setHint('');
      break;
    case 'finishDeadline':
      race.deadlineAt = performance.now() + message.remainingMs;
      break;
    case 'raceEvent':
      feed(describe(message.event));
      break;
    case 'results':
      renderResults(rage, message.results, session.user!);
      if (app.screen !== 'race') show('room');
      results.hidden = false;
      void refreshTables();
      break;
    case 'error':
      toast(message.message);
      break;
  }
}

// ---- room -------------------------------------------------------------------

$('room').addEventListener('click', async (event) => {
  const action = (event.target as HTMLElement).closest<HTMLElement>('[data-action]')?.dataset.action;
  if (action === 'leave-room') send({ t: 'leaveRoom' });
  else if (action === 'close-room' && room) send({ t: 'closeRoom', roomId: room.id });
  else if (action === 'edit-room' && room && app.rage) {
    const settings = await editSettings(app.rage, room.settings, room.members.length);
    if (settings) send({ t: 'updateRoom', settings });
  }
});

const sendCar = () => send({ t: 'setCar', ...readRoomCar(app.automaticCars) });
$('car-model').addEventListener('change', sendCar);
$('car-transmission').addEventListener('change', sendCar);
const mine = () => room?.members.find((m) => m.userId === session.user?.id);
$('ready-button').addEventListener('click', () => send({ t: 'setReady', ready: !mine()?.ready }));
$('start-button').addEventListener('click', () => send({ t: 'startRace' }));
$('spectate-button').addEventListener('click', () => send({ t: 'setSpectator', spectator: !mine()?.spectator }));
$('watch-button').addEventListener('click', () => send({ t: 'watchRace' }));
$<HTMLFormElement>('chat-form').addEventListener('submit', (event) => {
  event.preventDefault();
  const input = (event.target as HTMLFormElement).elements.namedItem('text') as HTMLInputElement;
  if (input.value.trim()) send({ t: 'chat', text: input.value });
  input.value = '';
});
results.querySelector('[data-action=results-done]')!.addEventListener('click', () => {
  results.hidden = true;
  if (racing) stopRace();
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

const leave = () => {
  send({ t: 'leaveRace' });
  show(room ? 'room' : 'lobby');
};

/* A player: this tick's controls drive the local race and go to the server. */
const playerNet: RaceNet = {
  sync: syncPrediction,
  tick(rage) {
    const words = rage.takeInput();
    connection?.sendInput(words, rage.inputSeq());
  },
  leave,
};

/* A spectator: the server's frames through the jitter buffer. */
const spectatorNet: RaceNet = {
  sync: () => 0,
  tick(rage) {
    const due = frames.next();
    if (due) rage.applyFrame(due.data);
  },
  leave,
};

async function startOnlineRace(message: Extract<ServerMessage, { t: 'raceStart' }>) {
  const rage = app.rage!;
  if (racing) stopRace();
  results.hidden = true;
  show('race');
  setHint('Preparing the course…');
  await new Promise(requestAnimationFrame);
  const players = message.seats.slice(0, message.humans);
  const ok = rage.startNetRace(message.settings, players.map(({ variant, manual }) => ({ variant, manual })),
                               message.localSeat);
  send({ t: 'loaded', ok });
  if (!ok) {
    show('room');
    toast('This race could not be prepared from your disc.');
    return;
  }
  const spectator = message.localSeat < 0;
  setHint(spectator ? 'Watching the race…' : 'Waiting for the other players…');
  frames.reset();
  latest = null;
  predicting = false;
  clockError = 0;
  beginRace(rage, {
    mode: spectator ? 'spectator' : 'player', names: message.seats.map((seat) => seat.name),
    humans: message.humans, userIds: players.map((seat) => seat.userId), localSeat: message.localSeat,
    deadlineAt: null,
  }, spectator ? spectatorNet : playerNet);
}

/* Applies the newest server frame (rewind and replay) and keeps the local
 * clock just far enough ahead that each input reaches the server a tick
 * before it is needed: late inputs speed the clock up, early ones slow it.
 * Returns the milliseconds to add to the local clock. */
function syncPrediction(rage: Rage): number {
  const frame = latest;
  const seat = race.localSeat;
  latest = null;
  if (!frame || seat * 2 + 1 >= frame.acks.length) return 0;
  const error = rage.applyPredicted(frame.data, frame.acks[seat * 2], frame.acks[seat * 2 + 1]);
  if (error === null) return 0;
  if (!predicting) {
    predicting = true; // the first frame only sets the starting point
    return 0;
  }
  const target = -1;
  if (error > target + 3) {
    // Far behind (start, or a network hiccup): catch up at once.
    clockError = target;
    return (error - target) * TICK_MS;
  }
  clockError = clockError * 0.9 + error * 0.1;
  if (clockError > target + 0.5) return TICK_MS * 0.05;
  if (clockError < target - 1.5) return -TICK_MS * 0.05;
  return 0;
}
