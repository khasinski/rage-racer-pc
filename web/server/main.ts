// Rage Racer multiplayer server: accounts and sessions (SQLite), the lobby
// and rooms over WebSocket, and the authoritative races (the C simulation as
// WebAssembly). It also serves the built browser client.
//
//   node server/main.ts --disc <CUE or Track 01 BIN> [--port 7243]
//                       [--db server/data/rage.db] [--static dist]
// RAGE_ADMIN_PASSWORD creates or updates the administrator account `admin`.
import { readFile, stat } from 'node:fs/promises';
import { createServer, type IncomingMessage, type ServerResponse } from 'node:http';
import { extname, join, normalize, resolve } from 'node:path';
import { parseArgs } from 'node:util';
import { WebSocketServer, type WebSocket } from 'ws';
import { DEFAULT_PORT } from '../shared/protocol.ts';
import { Store } from './db.ts';
import { Lobby, type Client } from './rooms.ts';
import { Simulation } from './sim.ts';

const here = import.meta.dirname;
const { values: options } = parseArgs({
  options: {
    disc: { type: 'string', default: process.env.RAGE_DISC },
    port: { type: 'string', default: process.env.PORT ?? String(DEFAULT_PORT) },
    host: { type: 'string', default: process.env.HOST ?? '0.0.0.0' },
    db: { type: 'string', default: process.env.RAGE_DB ?? join(here, 'data', 'rage.db') },
    static: { type: 'string', default: join(here, '..', 'dist') },
  },
});
if (!options.disc) {
  console.error('usage: node server/main.ts --disc <Rage Racer CUE or Track 01 BIN> [--port 7243]');
  process.exit(2);
}

const store = new Store(options.db!);
// Deployments set the administrator password here instead of running the seed.
if (process.env.RAGE_ADMIN_PASSWORD) store.ensureUser('admin', process.env.RAGE_ADMIN_PASSWORD, true);
const sim = await Simulation.load(options.disc);
const lobby = new Lobby(sim, store);
const staticRoot = resolve(options.static!);
console.log(`disc ${sim.discId}, database ${options.db}`);

const NAME = /^[A-Za-z0-9_.-]{3,20}$/;
const MIME: Record<string, string> = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript', '.mjs': 'text/javascript',
  '.css': 'text/css', '.wasm': 'application/wasm', '.json': 'application/json', '.png': 'image/png',
  '.svg': 'image/svg+xml', '.ico': 'image/x-icon',
};

function json(res: ServerResponse, status: number, body: unknown): void {
  res.writeHead(status, { 'content-type': 'application/json', 'cache-control': 'no-store' });
  res.end(JSON.stringify(body));
}

async function body(req: IncomingMessage): Promise<Record<string, unknown>> {
  let size = 0;
  const chunks: Buffer[] = [];
  for await (const chunk of req) {
    size += (chunk as Buffer).length;
    if (size > 16 * 1024) throw new Error('too large');
    chunks.push(chunk as Buffer);
  }
  const parsed = JSON.parse(Buffer.concat(chunks).toString('utf8') || '{}');
  return parsed && typeof parsed === 'object' ? parsed : {};
}

function bearer(req: IncomingMessage): string | null {
  const header = req.headers.authorization ?? '';
  return header.startsWith('Bearer ') ? header.slice(7) : null;
}

async function api(req: IncomingMessage, res: ServerResponse, path: string): Promise<void> {
  if (req.method === 'POST' && (path === '/api/register' || path === '/api/login')) {
    let input: Record<string, unknown>;
    try { input = await body(req); } catch { return json(res, 400, { error: 'Malformed request.' }); }
    const name = typeof input.name === 'string' ? input.name.trim() : '';
    const password = typeof input.password === 'string' ? input.password : '';
    if (path === '/api/register') {
      if (!NAME.test(name)) return json(res, 400, { error: 'Names are 3–20 letters, digits, dots, dashes or underscores.' });
      if (password.length < 4 || password.length > 200) return json(res, 400, { error: 'Use a password of at least 4 characters.' });
      const user = store.createUser(name, password);
      if (!user) return json(res, 409, { error: 'That name is taken.' });
      return json(res, 201, { token: store.createSession(user.id), user });
    }
    const user = store.verifyUser(name, password);
    if (!user) return json(res, 401, { error: 'Wrong name or password.' });
    return json(res, 200, { token: store.createSession(user.id), user });
  }
  if (path === '/api/guest' && req.method === 'POST') {
    const guest = store.createGuest();
    return json(res, 201, { token: store.createSession(guest.id), user: guest });
  }
  if (path === '/api/records' && req.method === 'GET') return json(res, 200, { records: store.records() });
  const token = bearer(req);
  const user = token ? store.sessionUser(token) : null;
  if (!user || !token) return json(res, 401, { error: 'Log in first.' });
  if (path === '/api/me' && req.method === 'GET') return json(res, 200, { user, discId: sim.discId });
  if (path === '/api/history' && req.method === 'GET') return json(res, 200, { history: store.history(user.id) });
  if (path === '/api/logout' && req.method === 'POST') {
    store.deleteSession(token);
    return json(res, 200, {});
  }
  return json(res, 404, { error: 'Not found.' });
}

async function serveStatic(res: ServerResponse, path: string): Promise<void> {
  let file = normalize(join(staticRoot, decodeURIComponent(path)));
  if (!file.startsWith(staticRoot)) {
    res.writeHead(403).end();
    return;
  }
  try {
    if ((await stat(file)).isDirectory()) file = join(file, 'index.html');
  } catch {
    file = join(staticRoot, 'index.html'); // single-page app
  }
  try {
    const data = await readFile(file);
    res.writeHead(200, { 'content-type': MIME[extname(file)] ?? 'application/octet-stream' });
    res.end(data);
  } catch {
    res.writeHead(404, { 'content-type': 'text/plain' }).end('Build the client first: npm run build');
  }
}

const server = createServer((req, res) => {
  const path = new URL(req.url ?? '/', 'http://localhost').pathname;
  const handler = path.startsWith('/api/') ? api(req, res, path) : serveStatic(res, path);
  handler.catch((error) => {
    console.error(error);
    if (!res.headersSent) json(res, 500, { error: 'Server error.' });
  });
});

const sockets = new WebSocketServer({ noServer: true, maxPayload: 64 * 1024, perMessageDeflate: true });

/* Keep-alive: a quiet room sends nothing, and proxies (nginx in front of
 * CapRover apps) close idle connections after about a minute. A ping every
 * KEEPALIVE_MS keeps the connection open; a socket that has not answered the
 * previous ping is dead and is closed, which also frees its seat. */
const KEEPALIVE_MS = Number(process.env.RAGE_KEEPALIVE_MS ?? 20_000);
const alive = new WeakMap<WebSocket, boolean>();
setInterval(() => {
  for (const ws of sockets.clients) {
    if (alive.get(ws) === false) {
      ws.terminate();
      continue;
    }
    alive.set(ws, false);
    ws.ping();
  }
}, KEEPALIVE_MS).unref();
server.on('upgrade', (req, socket, head) => {
  const url = new URL(req.url ?? '/', 'http://localhost');
  const user = url.pathname === '/ws' ? store.sessionUser(url.searchParams.get('token') ?? '') : null;
  if (!user) {
    socket.write('HTTP/1.1 401 Unauthorized\r\n\r\n');
    socket.destroy();
    return;
  }
  sockets.handleUpgrade(req, socket, head, (ws) => {
    const client: Client = { ws, user, roomId: null };
    alive.set(ws, true);
    ws.on('pong', () => alive.set(ws, true));
    lobby.connect(client);
    ws.on('message', (data, binary) => lobby.message(client, data as Buffer, binary));
    ws.on('close', () => lobby.disconnect(client));
    ws.on('error', () => lobby.disconnect(client));
  });
});

server.listen(Number(options.port), options.host, () => {
  console.log(`Rage Racer server on http://localhost:${options.port}/`);
});
