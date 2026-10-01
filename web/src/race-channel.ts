// The race's WebRTC data channel (see server/rtc.ts): unordered and
// unreliable, so a lost frame costs only itself instead of holding up the ones
// behind it as on the WebSocket. Signalling goes over the WebSocket; the
// channel pings itself and is dropped when the echoes stop, and the race goes
// on over the WebSocket while a new channel is offered, again and again with a
// growing pause, until one opens or the connection closes.
import { BINARY_PING, type ClientMessage } from '../shared/protocol.ts';
import type { LinkSimulator } from './net';

/* Pinged this often; given up when no echo came back for DEAD_MS. */
const PING_MS = 500;
const DEAD_MS = 2000;
/* A heartbeat this late means this page stalled (loading a race, a tab in the
 * background), not that the channel did. */
const STALL_MS = PING_MS * 3;
/* A channel carries the race only once a ping came back over it (it works
 * both ways). An offer not confirmed by then is given up; another follows
 * after the retry pause. */
const OPEN_TIMEOUT_MS = 10_000;
const RETRY_FIRST_MS = 1_000;
const RETRY_LONGEST_MS = 30_000;

async function inflate(data: ArrayBuffer): Promise<ArrayBuffer> {
  return new Response(new Blob([data]).stream().pipeThrough(new DecompressionStream('deflate-raw'))).arrayBuffer();
}

export class RaceChannel {
  private pc: RTCPeerConnection | null = null;
  private channel: RTCDataChannel | null = null;
  private heartbeat: ReturnType<typeof setInterval> | null = null;
  private openTimer: ReturnType<typeof setTimeout> | null = null;
  private retryTimer: ReturnType<typeof setTimeout> | null = null;
  private retryPause = RETRY_FIRST_MS;
  private closed = false; // for good: the connection is gone

  /**
   * @param signal sends a signalling message over the WebSocket
   * @param onFrame gets each race frame, inflated
   * @param up / down simulated conditions for experiments (see LinkSimulator)
   */
  constructor(private readonly signal: (message: ClientMessage) => void,
              private readonly onFrame: (frame: ArrayBuffer) => void,
              private readonly up: LinkSimulator | null,
              private readonly down: LinkSimulator | null) {}

  /** Whether race traffic can go this way. */
  get open(): boolean { return this.channel?.readyState === 'open'; }

  /** Offers the server a channel; failing, the race stays on the WebSocket
   *  and another offer follows later. */
  async offer(): Promise<void> {
    if (this.closed) return;
    try {
      const pc = this.pc = new RTCPeerConnection();
      const channel = pc.createDataChannel('race', { ordered: false, maxRetransmits: 0 });
      channel.binaryType = 'arraybuffer';
      let lastEcho = 0;
      channel.onmessage = (event) => {
        const data = event.data as ArrayBuffer;
        if (new Uint8Array(data)[0] === BINARY_PING) {
          lastEcho = performance.now();
          if (this.pc === pc && !this.channel) this.confirmed(channel);
          return;
        }
        const deliver = () => void inflate(data).then(this.onFrame, () => {});
        if (this.down) this.down.pass(deliver);
        else deliver();
      };
      channel.onopen = () => {
        if (this.pc !== pc) return; // closed (or replaced) while opening
        lastEcho = performance.now();
        this.startHeartbeat(channel, () => lastEcho, (now) => { lastEcho = now; });
      };
      channel.onclose = () => { if (this.pc === pc) this.lost(); };
      this.openTimer = setTimeout(() => { if (this.pc === pc && !this.channel) this.lost(); }, OPEN_TIMEOUT_MS);
      pc.onicecandidate = (event) => {
        if (event.candidate?.candidate) {
          this.signal({ t: 'rtcCandidate', candidate: event.candidate.candidate, mid: event.candidate.sdpMid ?? '0' });
        }
      };
      await pc.setLocalDescription(await pc.createOffer());
      this.signal({ t: 'rtcOffer', sdp: pc.localDescription!.sdp });
    } catch {
      this.lost();
    }
  }

  /* The first echo came back: race traffic can use this channel. */
  private confirmed(channel: RTCDataChannel): void {
    this.channel = channel;
    this.retryPause = RETRY_FIRST_MS;
    if (this.openTimer) clearTimeout(this.openTimer);
    this.openTimer = null;
  }

  /* Pings the server; a silent channel (a NAT forgot it, say) is dropped. */
  private startHeartbeat(channel: RTCDataChannel, lastEcho: () => number, heard: (now: number) => void): void {
    const ping = new DataView(new ArrayBuffer(9));
    ping.setUint8(0, BINARY_PING);
    let lastBeat = performance.now();
    this.heartbeat = setInterval(() => {
      const now = performance.now();
      // After a stall the echoes could not be read: give them another chance.
      if (now - lastBeat > STALL_MS) heard(now);
      lastBeat = now;
      if (now - lastEcho() > DEAD_MS) return this.lost();
      ping.setFloat64(1, now, true);
      if (channel.readyState === 'open') channel.send(ping.buffer);
    }, PING_MS);
  }

  answer(sdp: string): void {
    void this.pc?.setRemoteDescription({ type: 'answer', sdp }).catch(() => this.lost());
  }

  candidate(candidate: string, mid: string): void {
    void this.pc?.addIceCandidate({ candidate, sdpMid: mid }).catch(() => {});
  }

  /** Sends one packet; false when the channel is not open (use the WebSocket). */
  send(packet: Uint8Array<ArrayBuffer>): boolean {
    const channel = this.channel;
    if (channel?.readyState !== 'open') return false;
    if (this.up) this.up.pass(() => { if (channel.readyState === 'open') channel.send(packet); });
    else channel.send(packet);
    return true;
  }

  /* The channel failed or went silent: drop it and offer another later. */
  private lost(): void {
    this.dispose();
    if (this.closed || this.retryTimer) return;
    this.retryTimer = setTimeout(() => {
      this.retryTimer = null;
      void this.offer();
    }, this.retryPause);
    this.retryPause = Math.min(RETRY_LONGEST_MS, this.retryPause * 2);
  }

  private dispose(): void {
    if (this.heartbeat) clearInterval(this.heartbeat);
    if (this.openTimer) clearTimeout(this.openTimer);
    this.heartbeat = this.openTimer = null;
    const { channel, pc } = this;
    this.channel = null;
    this.pc = null; // first: their close events must not count as a new loss
    channel?.close();
    pc?.close();
  }

  /** For good, with the connection: no further offers. */
  close(): void {
    this.closed = true;
    if (this.retryTimer) clearTimeout(this.retryTimer);
    this.retryTimer = null;
    this.dispose();
  }
}
