// Protocol and race-management checks for the multiplayer server, without a
// browser:  node scripts/server-test.mjs <Track 01 BIN or CUE>
// Starts the server on a scratch database and checks accounts, room and car
// rules, host hand-over, chat, and a complete race run by the server to the
// finish (rivals finish, players who do not are closed out), with events,
// results and their storage.
import { spawn, spawnSync } from 'node:child_process';
import { mkdtempSync, rmSync } from 'node:fs';
import { tmpdir } from 'node:os';
import { dirname, join, resolve } from 'node:path';
import { DatabaseSync } from 'node:sqlite';
import { fileURLToPath } from 'node:url';

const web = join(dirname(fileURLToPath(import.meta.url)), '..');
const disc = process.argv[2] && resolve(process.argv[2]);
if (!disc) { console.error('usage: server-test.mjs <disc>'); process.exit(2); }
const scratch = mkdtempSync(join(tmpdir(), 'rage-server-test-'));
const db = join(scratch, 'test.db');
spawnSync(process.execPath, [join(web, 'server/seed.ts'), '--db', db], { stdio: 'ignore' });
const port = 4181;
const server = spawn(process.execPath, [join(web, 'server/main.ts'), '--disc', disc, '--port', String(port), '--db', db],
  { cwd: web, stdio: ['ignore', 'pipe', 'inherit'], env: { ...process.env, RAGE_FINISH_GRACE_MS: '3000' } });
await new Promise((ready) => server.stdout.on('data', (d) => String(d).includes('server on') && ready()));

const base = `http://localhost:${port}`;
const failures = [];
const check = (ok, what) => { console.log(`${ok ? '✓' : '✗'} ${what}`); if (!ok) failures.push(what); };
const wait = (ms) => new Promise((r) => setTimeout(r, ms));
const post = async (path, body, token) => {
  const response = await fetch(base + path, { method: 'POST', body: JSON.stringify(body),
    headers: token ? { authorization: `Bearer ${token}` } : {} });
  return { status: response.status, body: await response.json() };
};

async function connect(name, password) {
  const { body } = await post('/api/login', { name, password });
  const ws = new WebSocket(`ws://localhost:${port}/ws?token=${body.token}`);
  ws.binaryType = 'arraybuffer';
  const p = { name, ws, token: body.token, user: body.user, messages: [], frames: 0, lastTick: 0 };
  ws.onmessage = (event) => {
    if (typeof event.data === 'string') p.messages.push(JSON.parse(event.data));
    else { p.frames++; p.lastTick = new DataView(event.data).getUint32(1, true); }
  };
  await new Promise((r) => { ws.onopen = r; });
  p.send = (message) => ws.send(JSON.stringify(message));
  p.last = (type) => p.messages.filter((m) => m.t === type).at(-1);
  p.errors = () => p.messages.filter((m) => m.t === 'error').map((m) => m.message);
  p.input = (throttle) => {
    const buffer = new ArrayBuffer(33);
    const view = new DataView(buffer);
    view.setUint8(0, 1);
    [1, 0, 0, 0, throttle, 0, 0, 0].forEach((word, i) => view.setInt32(1 + i * 4, word, true));
    ws.send(buffer);
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

  const admin = await connect('admin', 'admin');
  const rage = await connect('rage', 'racer');
  const newbie = await connect('newbie', 'secret');
  await wait(200);
  check(admin.last('welcome')?.user.admin === true && rage.last('welcome')?.user.admin === false, 'welcome carries the account and admin flag');

  // Room rules come from the same compiled code the browser runs.
  const settings = { name: 'Test', classIndex: 0, course: 3, reverse: false, laps: 1, rivals: true, maxPlayers: 2 };
  admin.send({ t: 'createRoom', settings });
  await wait(200);
  check(admin.errors().some((e) => e.includes('not raced in this class')), 'the Extreme Oval is refused below class 3');
  admin.send({ t: 'createRoom', settings: { ...settings, course: 0, maxPlayers: 40 } });
  await wait(200);
  check(admin.errors().some((e) => e.includes('starting places')), 'more players than grid places are refused');
  admin.send({ t: 'createRoom', settings: { ...settings, course: 0, classIndex: 2 } });
  await wait(300);
  const room = admin.last('room')?.room;
  check(room && room.members.length === 1 && room.members[0].host, 'the creator hosts the new room');
  check(rage.last('rooms')?.rooms.some((r) => r.id === room.id), 'the room list reaches other players');

  rage.send({ t: 'joinRoom', roomId: room.id });
  newbie.send({ t: 'joinRoom', roomId: room.id });
  await wait(300);
  check(newbie.errors().some((e) => e.includes('full')), 'a full room refuses more players');
  rage.send({ t: 'setCar', variant: 9, manual: false }); // Esperanza I is a class 1 car
  await wait(200);
  check(rage.errors().some((e) => e.includes('not available')), 'a car from another class is refused');
  rage.send({ t: 'setCar', variant: 11, manual: false }); // Esperanza III, class 3
  admin.send({ t: 'startRace' });
  await wait(200);
  check(admin.errors().some((e) => e.includes('Waiting for rage')), 'the host waits for everyone to be ready');
  rage.send({ t: 'chat', text: 'ready when you are' });
  rage.send({ t: 'setReady', ready: true });
  await wait(200);
  check(admin.messages.some((m) => m.t === 'chat' && m.from === 'rage'), 'chat reaches the room');

  // Hand-over: the host leaves and comes back; rage now hosts.
  admin.send({ t: 'leaveRoom' });
  await wait(200);
  check(rage.last('room')?.room.members.find((m) => m.name === 'rage')?.host, 'the host role passes on when the host leaves');
  admin.send({ t: 'joinRoom', roomId: room.id });
  await wait(200);
  admin.send({ t: 'setReady', ready: true });
  await wait(200);

  // A one-lap race on class 3 Mythical Coast with the retail rivals. The
  // players only hold the throttle; the rivals finish and the server closes
  // the race after the grace period.
  rage.send({ t: 'startRace' });
  await wait(300);
  const start = rage.last('raceStart');
  check(start && start.seats.length === 12 && start.humans === 2 && start.localSeat === 0, 'the race starts with both players and ten rivals');
  check(admin.last('raceStart')?.localSeat === 1, 'each player learns its own seat');
  admin.send({ t: 'loaded', ok: true });
  rage.send({ t: 'loaded', ok: true });
  const t0 = Date.now();
  while (!admin.last('results') && Date.now() - t0 < 240_000) {
    admin.input(256);
    rage.input(256);
    await wait(20);
  }
  const results = admin.last('results');
  const events = admin.messages.filter((m) => m.t === 'raceEvent').map((m) => m.event);
  check(admin.frames > 100 && rage.frames > 100, `both players received the race stream (${admin.frames} frames)`);
  check(events.some((e) => e.kind === 'finish' && e.place === 1), 'the winner\'s finish is announced');
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

  // Closing: only the host or an admin.
  newbie.send({ t: 'closeRoom', roomId: room.id });
  await wait(200);
  check(newbie.errors().some((e) => e.includes('Only the host')), 'others cannot close a room');
  admin.send({ t: 'closeRoom', roomId: room.id });
  await wait(300);
  check(rage.last('room')?.room === null && rage.last('rooms')?.rooms.length === 0, 'an admin can close any room');
  for (const p of [admin, rage, newbie]) p.ws.close();
} catch (error) {
  failures.push(String(error));
  console.error(error);
} finally {
  server.kill();
  rmSync(scratch, { recursive: true, force: true });
}
if (failures.length) { console.error(`${failures.length} failed`); process.exit(1); }
console.log('ok');
