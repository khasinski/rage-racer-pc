// The race's WebRTC data channel (see server/rtc.ts): unordered and
// unreliable, so a lost frame costs only itself instead of holding up the ones
// behind it as on the WebSocket. Signalling goes over the WebSocket; the
// channel pings itself and closes when the echoes stop, and the race then
// stays on the WebSocket.
import { BINARY_PING, type ClientMessage } from '../shared/protocol.ts';
import type { LinkSimulator } from './net';

/* Pinged this often; given up when no echo came back for DEAD_MS. */
const PING_MS = 500;
const DEAD_MS = 2000;

async function inflate(data: ArrayBuffer): Promise<ArrayBuffer> {
  return new Response(new Blob([data]).stream().pipeThrough(new DecompressionStream('deflate-raw'))).arrayBuffer();
}

export class RaceChannel {
  private pc: RTCPeerConnection | null = null;
  private channel: RTCDataChannel | null = null;
  private heartbeat: ReturnType<typeof setInterval> | null = null;

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

  /** Offers the server a channel; failing, the race simply stays on the WebSocket. */
  async offer(): Promise<void> {
    try {
      const pc = this.pc = new RTCPeerConnection();
      const channel = pc.createDataChannel('race', { ordered: false, maxRetransmits: 0 });
      channel.binaryType = 'arraybuffer';
      let lastEcho = 0;
      channel.onmessage = (event) => {
        const data = event.data as ArrayBuffer;
        if (new Uint8Array(data)[0] === BINARY_PING) {
          lastEcho = performance.now();
          return;
        }
        const deliver = () => void inflate(data).then(this.onFrame, () => {});
        if (this.down) this.down.pass(deliver);
        else deliver();
      };
      channel.onopen = () => {
        if (this.pc !== pc) return; // closed (or replaced) while opening
        this.channel = channel;
        lastEcho = performance.now();
        this.startHeartbeat(channel, () => lastEcho);
      };
      channel.onclose = () => { if (this.channel === channel) this.channel = null; };
      pc.onicecandidate = (event) => {
        if (event.candidate?.candidate) {
          this.signal({ t: 'rtcCandidate', candidate: event.candidate.candidate, mid: event.candidate.sdpMid ?? '0' });
        }
      };
      await pc.setLocalDescription(await pc.createOffer());
      this.signal({ t: 'rtcOffer', sdp: pc.localDescription!.sdp });
    } catch {
      this.close();
    }
  }

  /* Pings the server; a silent channel (a NAT forgot it, say) is closed. */
  private startHeartbeat(channel: RTCDataChannel, lastEcho: () => number): void {
    const ping = new DataView(new ArrayBuffer(9));
    ping.setUint8(0, BINARY_PING);
    this.heartbeat = setInterval(() => {
      const now = performance.now();
      if (now - lastEcho() > DEAD_MS) return this.close();
      ping.setFloat64(1, now, true);
      if (channel.readyState === 'open') channel.send(ping.buffer);
    }, PING_MS);
  }

  answer(sdp: string): void {
    void this.pc?.setRemoteDescription({ type: 'answer', sdp }).catch(() => this.close());
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

  close(): void {
    if (this.heartbeat) clearInterval(this.heartbeat);
    this.heartbeat = null;
    this.channel?.close();
    this.pc?.close();
    this.channel = null;
    this.pc = null;
  }
}
