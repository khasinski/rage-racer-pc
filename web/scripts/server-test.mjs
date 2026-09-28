// Protocol and race-management checks for the multiplayer server, without a
// browser:  node scripts/server-test.mjs <Track 01 BIN or CUE>
// Starts the server on a scratch database and checks accounts, room and car
// rules, host hand-over, chat, and a complete race run by the server to the
// finish (rivals finish, players who do not are closed out), with events,
// results and their storage; then short races for leaving before the start,
// reconnecting before it and the load timeout, and the offline grace period.
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { join, resolve } from 'node:path';
import { DatabaseSync } from 'node:sqlite';
import { checks, startServer, until, wait } from './lib/harness.mjs';

const disc = process.argv[2] && resolve(process.argv[2]);
if (!disc) { console.error('usage: server-test.mjs <disc>'); process.exit(2); }
const scratch = mkdtempSync(join(tmpdir(), 'rage-server-test-'));
const db = join(scratch, 'test.db');
const port = 4181;
const LOAD_TIMEOUT_MS = 5000;
const OFFLINE_GRACE_MS = 3000;
const { base, stop } = await startServer({ disc, port, db, env: { RAGE_FINISH_GRACE_MS: '3000', RAGE_KEEPALIVE_MS: '500',
  RAGE_LOAD_TIMEOUT_MS: String(LOAD_TIMEOUT_MS), RAGE_OFFLINE_GRACE_MS: String(OFFLINE_GRACE_MS) } });
const { check, failures, report } = checks();

const post = async (path, body, token) => {
  const response = await fetch(base + path, { method: 'POST', body: JSON.stringify(body),
    headers: token ? { authorization: `Bearer ${token}` } : {} });
  return { status: response.status, body: await response.json() };
};

/* Opens a new connection with a player's session (the old one closed). */
async function reconnect(p) {
  p.ws.close();
  const ws = new WebSocket(`ws://localhost:${port}/ws?token=${p.token}`);
  ws.binaryType = 'arraybuffer';
  p.ws = ws;
  p.messages.length = 0;
  ws.onmessage = p.handler;
  await new Promise((r) => { ws.onopen = r; });
}

async function connect(name, password) {
  const { body } = await post('/api/login', { name, password });
  const ws = new WebSocket(`ws://localhost:${port}/ws?token=${body.token}`);
  ws.binaryType = 'arraybuffer';
  const p = { name, ws, token: body.token, user: body.user, messages: [], frames: 0, lastTick: 0 };
  p.handler = ws.onmessage = (event) => {
    if (typeof event.data === 'string') p.messages.push(JSON.parse(event.data));
    else {
      const view = new DataView(event.data);
      p.frames++;
      p.lastTick = view.getUint32(1, true);
      p.acks = Array.from({ length: view.getUint8(5) * 2 }, (_, i) => view.getUint32(6 + i * 4, true));
    }
  };
  await new Promise((r) => { ws.onopen = r; });
  p.send = (message) => p.ws.send(JSON.stringify(message));
  p.last = (type) => p.messages.filter((m) => m.t === type).at(-1);
  // Messages of a type that arrived after p.mark() returned `mark`.
  p.mark = () => p.messages.length;
  p.since = (mark, type) => p.messages.slice(mark).filter((m) => m.t === type);
  p.errors = () => p.messages.filter((m) => m.t === 'error').map((m) => m.message);
  p.sequence = 0;
  p.input = (throttle) => {
    const buffer = new ArrayBuffer(37);
    const view = new DataView(buffer);
    view.setUint8(0, 1);
    view.setUint32(1, ++p.sequence, true);
    [1, 0, 0, 0, throttle, 0, 0, 0].forEach((word, i) => view.setInt32(5 + i * 4, word, true));
    p.ws.send(buffer);
  };
  return p;
}

try {
  // Accounts.
  check((await post('/api/login', { name: 'admin', password: 'wrong' })).status === 401, 'a wrong password is refused');
  const created = await post('/api/register', { name: 'newbie', password: 'secret' });
  check(created.status === 201 && created.body.token, 'a new account can be registered');
  check((await post('/api/register', { name: 'NEWBIE', password: 'other' })).status === 409, 'names are unique regardless of case');
  check((await post('/api/register', { name: 'x', password: 'secret' })).status === 400, 'too short names are refused');
  const unauthorized = await fetch(`${base}/api/me`);
  check(unauthorized.status === 401, 'the API needs a session');

  // Guests: numbered accounts without a name or password.
  const guestA = await post('/api/guest', {});
  const guestB = await post('/api/guest', {});
  check(guestA.status === 201 && guestA.body.user.name === 'Guest #1' && guestB.body.user.name === 'Guest #2',
        `guests get the next number (${guestA.body.user?.name}, ${guestB.body.user?.name})`);
  const guestWs = new WebSocket(`ws://localhost:${port}/ws?token=${guestB.body.token}`);
  const guestWelcome = await new Promise((r) => { guestWs.onmessage = (e) => r(JSON.parse(e.data)); setTimeout(() => r(null), 5000); });
  check(guestWelcome?.t === 'welcome' && guestWelcome.user.name === 'Guest #2', 'a guest joins the lobby');
  guestWs.close();
  check((await post('/api/register', { name: 'Guest #3', password: 'secret' })).status === 400, 'nobody can register a guest name');

  const admin = await connect('admin', 'admin');
  const rage = await connect('rage', 'racer');
  const newbie = await connect('newbie', 'secret');
  check(await until(() => admin.last('welcome')?.user.admin === true && rage.last('welcome')?.user.admin === false), 'welcome carries the account and admin flag');

  // Room rules come from the same compiled code the browser runs.
  const settings = { name: 'Test', classIndex: 0, course: 3, reverse: false, laps: 1, rivals: true, maxPlayers: 2 };
  admin.send({ t: 'createRoom', settings });
  check(await until(() => admin.errors().some((e) => e.includes('not raced in this class'))), 'the Extreme Oval is refused below class 3');
  admin.send({ t: 'createRoom', settings: { ...settings, course: 0, maxPlayers: 40 } });
  check(await until(() => admin.errors().some((e) => e.includes('starting places'))), 'more players than grid places are refused');
  admin.send({ t: 'createRoom', settings: { ...settings, course: 0, classIndex: 2 } });
  await until(() => admin.last('room')?.room);
  const room = admin.last('room')?.room;
  check(room && room.members.length === 1 && room.members[0].host, 'the creator hosts the new room');
  check(await until(() => rage.last('rooms')?.rooms.some((r) => r.id === room.id)), 'the room list reaches other players');

  rage.send({ t: 'joinRoom', roomId: room.id });
  newbie.send({ t: 'joinRoom', roomId: room.id });
  check(await until(() => newbie.last('room')?.room?.members.find((m) => m.name === 'newbie')?.spectator === true),
        'a full room takes a newcomer as a spectator');
  newbie.send({ t: 'setSpectator', spectator: false });
  check(await until(() => newbie.errors().some((e) => e.includes('grid is full'))), 'a spectator cannot take a place on a full grid');
  newbie.send({ t: 'leaveRoom' });
  await until(() => newbie.last('room')?.room === null);
  rage.send({ t: 'setCar', variant: 9, manual: false }); // Esperanza I is a class 1 car
  check(await until(() => rage.errors().some((e) => e.includes('not available'))), 'a car from another class is refused');
  rage.send({ t: 'setCar', variant: 11, manual: false }); // Esperanza III, class 3
  admin.send({ t: 'startRace' });
  check(await until(() => admin.errors().some((e) => e.includes('Waiting for rage'))), 'the host waits for everyone to be ready');
  rage.send({ t: 'chat', text: 'ready when you are' });
  rage.send({ t: 'setReady', ready: true });
  check(await until(() => admin.messages.some((m) => m.t === 'chat' && m.from === 'rage')), 'chat reaches the room');

  // Keep-alive: pings every 0.5 s here; players answering them stay put.
  await wait(2000);
  check(admin.ws.readyState === WebSocket.OPEN && rage.last('room')?.room.members.length === 2,
        'connections answering the keep-alive stay in their room');

  // Hand-over: the host leaves and comes back; rage now hosts.
  admin.send({ t: 'leaveRoom' });
  check(await until(() => rage.last('room')?.room.members.find((m) => m.name === 'rage')?.host), 'the host role passes on when the host leaves');
  admin.send({ t: 'joinRoom', roomId: room.id });
  await until(() => rage.last('room')?.room.members.length === 2);
  admin.send({ t: 'setReady', ready: true });
  await until(() => rage.last('room')?.room.members.every((m) => m.ready));

  // A one-lap race on class 3 Mythical Coast with the retail rivals. The
  // players only hold the throttle; the rivals finish and the server closes
  // the race after the grace period.
  rage.send({ t: 'startRace' });
  await until(() => rage.last('raceStart') && admin.last('raceStart'));
  const start = rage.last('raceStart');
  check(start && start.seats.length === 12 && start.humans === 2, 'the race starts with both players and ten rivals');
  const seatsTaken = [start?.localSeat, admin.last('raceStart')?.localSeat].sort().join(',');
  check(seatsTaken === '0,1' && start.seats[start.localSeat].name === 'rage', 'each player learns its own seat');
  admin.send({ t: 'loaded', ok: true });
  rage.send({ t: 'loaded', ok: true });
  // A spectator joins the running race: no seat, the same stream.
  await until(() => admin.frames > 10);
  newbie.send({ t: 'joinRoom', roomId: room.id });
  check(await until(() => newbie.last('raceStart')?.localSeat === -1), 'a spectator joins the running race without a seat');
  check(await until(() => newbie.frames > 20), 'the spectator receives the race stream');
  // A player who drops keeps the seat and gets back into the running race.
  const seatBefore = rage.last('raceStart').localSeat;
  rage.ws.close();
  check(await until(() => admin.last('room')?.room.members.find((m) => m.name === 'rage')?.online === false),
        'a dropped player stays in the room, marked offline');
  await reconnect(rage);
  check(await until(() => rage.last('raceStart')?.localSeat === seatBefore), 'reconnecting puts the player back in their car');
  const framesBack = rage.frames;
  check(await until(() => rage.frames > framesBack + 10), 'the race stream resumes after reconnecting');
  check(await until(() => typeof admin.last('latency')?.latency[rage.user.id] === 'number'), 'rooms hear every player latency');
  const t0 = Date.now();
  while (!admin.last('results') && Date.now() - t0 < 240_000) {
    admin.input(256);
    rage.input(256);
    await wait(20);
  }
  const results = admin.last('results');
  const events = admin.messages.filter((m) => m.t === 'raceEvent').map((m) => m.event);
  const seatOfRage = rage.last('raceStart').localSeat;
  check(rage.acks?.[seatOfRage * 2] > 100 && rage.acks[seatOfRage * 2] <= rage.sequence && rage.acks[seatOfRage * 2 + 1] > 0,
        `frames acknowledge each player's inputs (${rage.acks?.[seatOfRage * 2]} of ${rage.sequence})`);
  check(admin.frames > 100 && rage.frames > 100, `both players received the race stream (${admin.frames} frames)`);
  check(events.some((e) => e.kind === 'finish' && e.place === 1), 'the winner\'s finish is announced');
  const deadline = rage.last('finishDeadline');
  check(deadline?.remainingMs > 0 && newbie.last('finishDeadline'), `the finish deadline is announced to players and spectators (${deadline?.remainingMs} ms)`);
  check(newbie.last('results')?.results.length === 12, 'the spectator gets the results');
  check(results?.results.length === 12, 'results cover the whole field');
  const winner = results?.results[0];
  check(winner?.place === 1 && winner.status === 'finished' && winner.timeMs > 0, `the winner has a time (${winner?.name} ${winner?.timeMs} ms)`);
  check(results?.results.filter((r) => r.userId !== null).every((r) => r.status === 'dnf'), 'players still on track are closed out as not finished');
  const places = results?.results.filter((r) => r.status === 'finished').map((r) => r.place) ?? [];
  check(places.every((place, i) => place === i + 1), 'finishers are in order');
  const stored = new DatabaseSync(db).prepare('SELECT COUNT(*) AS n FROM race_results').get();
  check(stored.n === 12, 'results are stored in SQLite');
  const history = await (await fetch(`${base}/api/history`, { headers: { authorization: `Bearer ${rage.token}` } })).json();
  check(history.history?.[0]?.entrants === 12, 'the race shows in the player\'s history');
  check(rage.last('room')?.room.status === 'lobby', 'the room returns to its lobby');

  // Lap records keep each course variant's fastest player lap. The bots
  // above never complete a lap, so store two laps directly.
  const store = new DatabaseSync(db);
  const raceId = store.prepare('SELECT MAX(id) AS id FROM races').get().id;
  store.prepare('UPDATE race_results SET best_lap_ms = ? WHERE race_id = ? AND name = ?').run(61234, raceId, 'rage');
  store.prepare('UPDATE race_results SET best_lap_ms = ? WHERE race_id = ? AND name = ?').run(65000, raceId, 'admin');
  const records = (await (await fetch(`${base}/api/records`)).json()).records;
  check(records.length === 1 && records[0].name === 'rage' && records[0].bestLapMs === 61234 && records[0].classIndex === 2,
        'lap records keep the fastest player per course variant and class');

  /* Starts a race in the room: the other racer gets ready, the host starts,
   * and both players get their seats. */
  const startRaceWith = async (host, other) => {
    other.send({ t: 'setReady', ready: true });
    await until(() => host.last('room')?.room.members.every((m) => m.ready || m.spectator));
    const marks = [host.mark(), other.mark()];
    host.send({ t: 'startRace' });
    if (!(await until(() => host.since(marks[0], 'raceStart').length && other.since(marks[1], 'raceStart').length))) {
      throw new Error(`the race did not start: ${host.errors().at(-1)}`);
    }
  };
  const retires = (p, mark, name) => p.since(mark, 'raceEvent').filter((m) => m.event.kind === 'retire' && m.event.name === name);
  const hostAndOther = () => admin.last('room')?.room.members.find((m) => m.host)?.name === 'rage' ? [rage, admin] : [admin, rage];

  // Escape while the race loads: the car is retired when the race starts
  // instead of holding the race open on the grid.
  let mark = admin.mark();
  await startRaceWith(...hostAndOther());
  admin.send({ t: 'loaded', ok: true });
  rage.send({ t: 'leaveRace' });
  check(await until(() => admin.since(mark, 'raceGo').length === 1), 'the race starts without a player who left while it loaded');
  check(retires(admin, mark, 'rage').length === 1, 'leaving before the start announces the retirement');
  admin.send({ t: 'leaveRace' });
  check(await until(() => admin.since(mark, 'results').length === 1, 8000), 'the race ends once no player is left driving');
  check(admin.last('results')?.results.find((r) => r.name === 'rage')?.status === 'retired',
        'a player who left before the start is retired at the start');

  // A player who drops before the start is awaited again: the race starts
  // once they reloaded it, not as soon as they are back.
  mark = admin.mark();
  await startRaceWith(...hostAndOther());
  admin.send({ t: 'loaded', ok: true });
  await reconnect(rage);
  check(await until(() => rage.last('raceStart')), 'a player back before the start gets the race again');
  await wait(1000);
  check(admin.since(mark, 'raceGo').length === 0, 'the start waits for a player who reconnected before it');
  rage.send({ t: 'loaded', ok: true });
  check(await until(() => admin.since(mark, 'raceGo').length === 1), 'the race starts once the reconnected player loaded it');
  // A car that is out of the race already is not retired again on leaving.
  rage.send({ t: 'leaveRace' });
  await until(() => retires(admin, mark, 'rage').length === 1);
  rage.send({ t: 'leaveRoom' });
  await until(() => admin.last('room')?.room.members.every((m) => m.name !== 'rage'));
  await wait(300);
  check(retires(admin, mark, 'rage').length === 1,
        `leaving the room after retiring announces nothing more (${retires(admin, mark, 'rage').length} retirements)`);
  admin.send({ t: 'leaveRace' });
  check(await until(() => admin.since(mark, 'results').length === 1, 8000), 'the race ends once both players are out');

  // The load timeout: a player who never loads leaves the room and the race
  // starts without them.
  rage.send({ t: 'joinRoom', roomId: room.id });
  await until(() => admin.last('room')?.room.members.some((m) => m.name === 'rage' && !m.spectator));
  mark = admin.mark();
  const rageMark = rage.mark();
  await startRaceWith(...hostAndOther());
  admin.send({ t: 'loaded', ok: true });
  const t1 = Date.now();
  const startedLate = await until(() => admin.since(mark, 'raceGo').length === 1, LOAD_TIMEOUT_MS + 3000);
  check(startedLate && Date.now() - t1 > LOAD_TIMEOUT_MS - 1000,
        `the race starts after the load timeout without the player who did not load (${Date.now() - t1} ms)`);
  check(retires(admin, mark, 'rage')[0]?.event.reason === 'could not load the race in time' &&
        rage.since(rageMark, 'room').at(-1)?.room === null, 'the player who did not load is retired and leaves the room');
  admin.send({ t: 'leaveRace' });
  check(await until(() => admin.since(mark, 'results').length === 1, 8000), 'the race without them runs to its results');
  rage.send({ t: 'joinRoom', roomId: room.id });
  await until(() => rage.last('room')?.room?.id === room.id);

  // Closing: only the host or an admin.
  newbie.send({ t: 'closeRoom', roomId: room.id });
  check(await until(() => newbie.errors().some((e) => e.includes('Only the host'))), 'others cannot close a room');
  admin.send({ t: 'closeRoom', roomId: room.id });
  check(await until(() => rage.last('room')?.room === null && rage.last('rooms')?.rooms.length === 0), 'an admin can close any room');

  // A player offline past the grace period loses their place.
  admin.send({ t: 'createRoom', settings: { ...settings, course: 0, classIndex: 2, name: 'Grace' } });
  await until(() => admin.last('room')?.room?.settings.name === 'Grace');
  rage.send({ t: 'joinRoom', roomId: admin.last('room').room.id });
  await until(() => admin.last('room')?.room.members.length === 2);
  rage.ws.close();
  const t2 = Date.now();
  await until(() => admin.last('room')?.room.members.find((m) => m.name === 'rage')?.online === false);
  const gone = await until(() => admin.last('room')?.room.members.length === 1, OFFLINE_GRACE_MS + 5000);
  check(gone && Date.now() - t2 > OFFLINE_GRACE_MS - 500, `an offline player leaves the room after the grace period (${Date.now() - t2} ms)`);
  check(admin.messages.some((m) => m.t === 'chat' && m.text === 'rage disconnected.'), 'the room hears that they disconnected');
  for (const p of [admin, rage, newbie]) p.ws.close();
} catch (error) {
  failures.push(String(error));
  console.error(error);
} finally {
  stop();
  rmSync(scratch, { recursive: true, force: true });
}
report();
