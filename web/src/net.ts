// Talks to the multiplayer server: REST for accounts, one WebSocket for the
// lobby, rooms and the race stream (see shared/protocol.ts).
import {
  ACK_WORDS, BINARY_FRAME, CLOSE_REPLACED, BINARY_INPUT, INPUT_BYTES, INPUT_REPEAT, INPUT_WORDS,
  type ClientMessage, type Garage, type Paint, type RecordRow, type ServerMessage, type UserInfo,
} from '../shared/protocol.ts';
import { RaceChannel } from './race-channel';

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

/** One of the player's recent races (GET api/history). */
export interface HistoryRow {
  course: number;
  reverse: boolean;
  classIndex: number;
  place: number;
  entrants: number;
  timeMs: number;
  bestLapMs: number;
  status: string;
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

  login(name: string, password: string, register: boolean): Promise<void> {
    return this.adopt(request(register ? 'api/register' : 'api/login',
      { method: 'POST', body: JSON.stringify({ name, password }) }));
  }

  /** Plays as a new guest account ("Guest #n"). */
  guest(): Promise<void> {
    return this.adopt(request('api/guest', { method: 'POST' }));
  }

  /* Keeps the session a login or guest request answered with. */
  private async adopt(answer: Promise<{ token: string; user: UserInfo }>): Promise<void> {
    const result = await answer;
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

  /** The signed-in player's saved car paints. */
  async garage(): Promise<Garage> {
    return (await request<{ paints: Garage }>('api/garage', {}, this.token)).paints;
  }

  /** The signed-in player's team logo as base64 (see shared/protocol.ts), or null. */
  async logo(): Promise<string | null> {
    return (await request<{ logo: string | null }>('api/logo', {}, this.token)).logo;
  }

  async saveLogo(logo: string | null): Promise<string | null> {
    return (await request<{ logo: string | null }>('api/logo',
      { method: 'PUT', body: JSON.stringify({ logo }) }, this.token)).logo;
  }

  /** Saves a car model's paint (null: back to the factory colours). */
  async savePaint(model: number, paint: Paint | null): Promise<Garage> {
    return (await request<{ paints: Garage }>('api/garage',
      { method: 'PUT', body: JSON.stringify({ model, paint }) }, this.token)).paints;
  }

  /** The signed-in player's recent races. */
  history(): Promise<{ history: HistoryRow[] }> { return request('api/history', {}, this.token); }
}

/** A race frame as received: the server tick it describes and its bytes. */
export interface Frame {
  tick: number;
  data: Uint8Array;
  acks: Int32Array; // per player seat (ACK_WORDS each): last input applied, its tick, the latest input's margin
}

/**
 * Network conditions for experiments, from the page address:
 *   ?net=rtt:150,jitter:30,loss:2   (milliseconds, milliseconds, percent)
 * Each direction gets half the round trip plus random jitter. `ordered`
 * links (TCP: the WebSocket) keep order, so a lost packet is retransmitted
 * about one round trip later and holds up everything behind it; unordered
 * links (a DataChannel) simply drop it.
 */
export class LinkSimulator {
  private lastDelivery = 0;
  constructor(readonly rtt: number, readonly jitter: number, readonly loss: number, private readonly ordered: boolean) {}

  static fromAddress(ordered: boolean): LinkSimulator | null {
    const spec = new URLSearchParams(location.search).get('net');
    if (!spec) return null;
    const value = (key: string) => Number(spec.match(new RegExp(`${key}:(\\d+(?:\\.\\d+)?)`))?.[1] ?? 0);
    return new LinkSimulator(value('rtt'), value('jitter'), value('loss'), ordered);
  }

  /** Runs `deliver` when the packet would arrive (never, if lost unordered). */
  pass(deliver: () => void): void {
    const now = performance.now();
    let at = now + this.rtt / 2 + Math.random() * this.jitter;
    if (Math.random() * 100 < this.loss) {
      if (!this.ordered) return;
      at += this.rtt * 1.5; // retransmission
    }
    if (this.ordered) at = Math.max(at, this.lastDelivery);
    this.lastDelivery = at;
    setTimeout(deliver, Math.max(0, at - now));
  }
}

/* Browsers without WebRTC, and ?transport=ws, keep the race on the WebSocket. */
const RTC_WANTED = typeof RTCPeerConnection !== 'undefined' && new URLSearchParams(location.search).get('transport') !== 'ws';

/** A BINARY_INPUT packet carrying the given inputs (INPUT_BYTES each, oldest first). */
function inputPacket(inputs: Uint8Array[]): Uint8Array<ArrayBuffer> {
  const packet = new Uint8Array(1 + inputs.length * INPUT_BYTES);
  packet[0] = BINARY_INPUT;
  inputs.forEach((input, i) => packet.set(input, 1 + i * INPUT_BYTES));
  return packet;
}

/**
 * The connection to the server: a WebSocket for everything, plus (where the
 * browser and server manage it) a WebRTC data channel for the race stream
 * (race-channel.ts). See shared/protocol.ts.
 */
export class Connection {
  private readonly ws: WebSocket;
  /* Messages sent while the WebSocket was still opening. */
  private readonly unsent: string[] = [];
  private readonly channel: RaceChannel | null;
  /* The latest inputs (INPUT_BYTES each, oldest first): the data channel
   * repeats them, so the server still gets an input whose packet was lost. */
  private readonly recent: Uint8Array[] = [];
  private newestTick = -1; // frames older than this one are dropped
  /* Simulated conditions on the WebSocket, one per direction (see LinkSimulator). */
  private readonly upLink = LinkSimulator.fromAddress(true);
  private readonly downLink = LinkSimulator.fromAddress(true);
  onMessage: (message: ServerMessage) => void = () => {};
  onFrame: (frame: Frame) => void = () => {};
  /** `replaced`: a newer connection of the same account took this one over. */
  onClose: (reason: string, replaced: boolean) => void = () => {};

  constructor(token: string) {
    const url = new URL('ws', document.baseURI);
    url.protocol = url.protocol === 'https:' ? 'wss:' : 'ws:';
    url.searchParams.set('token', token);
    this.ws = new WebSocket(url);
    this.ws.binaryType = 'arraybuffer';
    this.channel = RTC_WANTED
      ? new RaceChannel((message) => this.send(message), (data) => this.frame(data),
                        LinkSimulator.fromAddress(false), LinkSimulator.fromAddress(false))
      : null;
    this.ws.onopen = () => {
      for (const text of this.unsent.splice(0)) this.ws.send(text);
      void this.channel?.offer();
    };
    this.ws.onmessage = (event) => {
      if (this.downLink) this.downLink.pass(() => this.receive(event.data));
      else this.receive(event.data);
    };
    this.ws.onclose = (event) => {
      this.channel?.close();
      this.onClose(event.reason || 'The connection to the server closed.', event.code === CLOSE_REPLACED);
    };
  }

  /** Whether the race runs over the data channel. */
  get transport(): 'rtc' | 'ws' {
    return this.channel?.open ? 'rtc' : 'ws';
  }

  private receive(data: string | ArrayBuffer): void {
    if (typeof data !== 'string') return this.frame(data);
    const message = JSON.parse(data) as ServerMessage;
    switch (message.t) {
      case 'rtcAnswer':
        this.channel?.answer(message.sdp);
        return;
      case 'rtcCandidate':
        this.channel?.candidate(message.candidate, message.mid);
        return;
      case 'raceStart':
        // A new race counts its ticks and inputs from the start.
        this.newestTick = -1;
        this.recent.length = 0;
        break;
    }
    this.onMessage(message);
  }

  private frame(buffer: ArrayBuffer): void {
    const bytes = new Uint8Array(buffer);
    if (bytes[0] !== BINARY_FRAME || bytes.length < 6) return;
    const view = new DataView(buffer);
    const tick = view.getUint32(1, true);
    if (tick <= this.newestTick) return; // overtaken on the data channel
    const players = bytes[5];
    const header = 6 + players * ACK_WORDS * 4;
    if (bytes.length < header) return;
    this.newestTick = tick;
    const acks = new Int32Array(players * ACK_WORDS);
    for (let i = 0; i < acks.length; i++) acks[i] = view.getInt32(6 + i * 4, true);
    this.onFrame({ tick, data: bytes.subarray(header), acks });
  }

  send(message: ClientMessage): void {
    const text = JSON.stringify(message);
    if (this.ws.readyState === WebSocket.OPEN) this.ws.send(text);
    else if (this.ws.readyState === WebSocket.CONNECTING) this.unsent.push(text); // not lost: sent on open
  }

  /** Sends one tick's input: with the last few repeated on the data channel,
   *  on its own over the WebSocket (which loses nothing). */
  sendInput(words: Int32Array, sequence: number, tick: number): void {
    const entry = new Uint8Array(INPUT_BYTES);
    const view = new DataView(entry.buffer);
    view.setUint32(0, sequence, true);
    view.setUint32(4, tick, true);
    for (let i = 0; i < INPUT_WORDS; i++) view.setInt32(8 + i * 4, words[i], true);
    this.recent.push(entry);
    if (this.recent.length > INPUT_REPEAT) this.recent.shift();
    if (this.channel?.send(inputPacket(this.recent))) return;
    if (this.ws.readyState !== WebSocket.OPEN) return;
    const packet = inputPacket([entry]);
    if (this.upLink) this.upLink.pass(() => { if (this.ws.readyState === WebSocket.OPEN) this.ws.send(packet); });
    else this.ws.send(packet);
  }

  close(): void {
    this.channel?.close();
    this.ws.close();
  }
}

/**
 * A spectator's view of the race: plays server frames back a fixed number
 * of ticks behind the newest one, so network jitter does not show as
 * stutter. (Players predict from the newest frame alone.) Frames arrive every second tick
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

  reset(): void {
    this.frames = [];
    this.play = this.newest = -1;
  }
}
