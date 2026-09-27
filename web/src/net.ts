// Talks to the multiplayer server: REST for accounts, one WebSocket for the
// lobby, rooms and the race stream (see shared/protocol.ts).
import {
  BINARY_FRAME, BINARY_INPUT, INPUT_WORDS,
  type ClientMessage, type RecordRow, type ServerMessage, type UserInfo,
} from '../shared/protocol.ts';

const TOKEN_KEY = 'rage-racer.session';

function storedToken(): string | null {
  try { return localStorage.getItem(TOKEN_KEY); } catch { return null; }
}
function storeToken(token: string | null): void {
  try {
    if (token) localStorage.setItem(TOKEN_KEY, token);
    else localStorage.removeItem(TOKEN_KEY);
  } catch { /* private mode: the session lasts until reload */ }
}

async function request<T>(path: string, init: RequestInit = {}, token?: string | null): Promise<T> {
  const headers: Record<string, string> = { 'content-type': 'application/json' };
  if (token) headers.authorization = `Bearer ${token}`;
  const response = await fetch(new URL(path, document.baseURI), { ...init, headers });
  const body = await response.json().catch(() => ({})) as T & { error?: string };
  if (!response.ok) throw new Error(body.error ?? `The server answered ${response.status}.`);
  return body;
}

export class Session {
  token: string | null = storedToken();
  user: UserInfo | null = null;
  discId = '';

  /** Restores a stored session; false when there is none or it expired. */
  async resume(): Promise<boolean> {
    if (!this.token) return false;
    try {
      const me = await request<{ user: UserInfo; discId: string }>('api/me', {}, this.token);
      this.user = me.user;
      this.discId = me.discId;
      return true;
    } catch {
      this.forget();
      return false;
    }
  }

  async login(name: string, password: string, register: boolean): Promise<void> {
    const result = await request<{ token: string; user: UserInfo }>(register ? 'api/register' : 'api/login',
      { method: 'POST', body: JSON.stringify({ name, password }) });
    this.token = result.token;
    this.user = result.user;
    storeToken(result.token);
    await this.resume();
  }

  /** Plays as a new guest account ("Guest #n"). */
  async guest(): Promise<void> {
    const result = await request<{ token: string; user: UserInfo }>('api/guest', { method: 'POST' });
    this.token = result.token;
    this.user = result.user;
    storeToken(result.token);
    await this.resume();
  }

  async logout(): Promise<void> {
    if (this.token) await request('api/logout', { method: 'POST' }, this.token).catch(() => undefined);
    this.forget();
  }

  private forget(): void {
    this.token = null;
    this.user = null;
    storeToken(null);
  }

  records(): Promise<{ records: RecordRow[] }> { return request('api/records'); }
}

/** A race frame as received: the server tick it describes and its bytes. */
export interface Frame {
  tick: number;
  data: Uint8Array;
  acks: Uint32Array; // per player seat: last input sequence, tick that first used it
}

export class Connection {
  private readonly ws: WebSocket;
  private readonly input = new DataView(new ArrayBuffer(5 + INPUT_WORDS * 4));
  onMessage: (message: ServerMessage) => void = () => {};
  onFrame: (frame: Frame) => void = () => {};
  onClose: (reason: string) => void = () => {};

  constructor(token: string) {
    const url = new URL('ws', document.baseURI);
    url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:';
    url.searchParams.set('token', token);
    this.ws = new WebSocket(url);
    this.ws.binaryType = 'arraybuffer';
    this.ws.onmessage = (event) => {
      if (typeof event.data === 'string') {
        this.onMessage(JSON.parse(event.data) as ServerMessage);
        return;
      }
      const bytes = new Uint8Array(event.data as ArrayBuffer);
      if (bytes[0] !== BINARY_FRAME || bytes.length < 6) return;
      const view = new DataView(bytes.buffer);
      const players = bytes[5];
      const header = 6 + players * 8;
      if (bytes.length < header) return;
      const acks = new Uint32Array(players * 2);
      for (let i = 0; i < acks.length; i++) acks[i] = view.getUint32(6 + i * 4, true);
      this.onFrame({ tick: view.getUint32(1, true), data: bytes.subarray(header), acks });
    };
    this.ws.onclose = (event) => this.onClose(event.reason || 'The connection to the server closed.');
  }

  send(message: ClientMessage): void {
    if (this.ws.readyState === WebSocket.OPEN) this.ws.send(JSON.stringify(message));
  }

  sendInput(words: Int32Array, sequence: number): void {
    if (this.ws.readyState !== WebSocket.OPEN) return;
    this.input.setUint8(0, BINARY_INPUT);
    this.input.setUint32(1, sequence, true);
    for (let i = 0; i < INPUT_WORDS; i++) this.input.setInt32(5 + i * 4, words[i], true);
    this.ws.send(this.input.buffer);
  }

  close(): void { this.ws.close(); }
}

/**
 * Plays server frames back a fixed number of ticks behind the newest one, so
 * network jitter does not show as stutter. Frames arrive every second tick
 * (the field's 25 Hz motion); the playback clock follows the local 50 Hz
 * loop and eases towards the target delay instead of jumping.
 */
export class FrameBuffer {
  private frames: Frame[] = [];
  private play = -1;
  private newest = -1;
  static readonly DELAY_TICKS = 4;

  push(frame: Frame): void {
    if (frame.tick <= this.newest) return;
    this.newest = frame.tick;
    this.frames.push(frame);
    if (this.play < 0) this.play = frame.tick - FrameBuffer.DELAY_TICKS;
  }

  /** Advances one local tick; returns the frame due now, if any. */
  next(): Frame | null {
    if (this.play < 0) return null;
    const lag = this.newest - this.play;
    if (lag > FrameBuffer.DELAY_TICKS + 12) this.play = this.newest - FrameBuffer.DELAY_TICKS; // fell behind
    else if (lag > FrameBuffer.DELAY_TICKS + 2) this.play += 2; // catch up gently
    else if (lag >= FrameBuffer.DELAY_TICKS - 2) this.play += 1;
    // otherwise hold: the buffer is running dry
    let due: Frame | null = null;
    while (this.frames.length && this.frames[0].tick <= this.play) due = this.frames.shift()!;
    return due;
  }

  /** The newest frame received since the last call, dropping older ones
   *  (a predicting player only needs the latest authoritative state). */
  takeLatest(): Frame | null {
    const latest = this.frames.pop() ?? null;
    this.frames = [];
    return latest;
  }

  reset(): void {
    this.frames = [];
    this.play = this.newest = -1;
  }
}
