// A WebRTC data channel beside a client's WebSocket, for the race stream.
// The channel is unordered and unreliable (UDP-like): a lost frame is simply
// replaced by the next one 40 ms later instead of holding up everything behind
// it, as a lost TCP segment does on the WebSocket. The client offers over the
// WebSocket (rtcOffer / rtcCandidate), the server answers; until the channel
// opens, and whenever it cannot, the race stays on the WebSocket.
//
// Deployment: RAGE_RTC_PORT sets the one UDP port every channel uses (it has
// to be published by the host, in host mode behind Docker), and
// RAGE_RTC_PUBLIC_IP the address the server advertises when it sits behind
// NAT. RAGE_RTC=0 turns the channel off.
import { PeerConnection, type DataChannel } from 'node-datachannel';
import { BINARY_PING, type ServerMessage } from '../shared/protocol.ts';

export const RTC_ENABLED = process.env.RAGE_RTC !== '0';
const PORT = Number(process.env.RAGE_RTC_PORT ?? 0);
const PUBLIC_IP = process.env.RAGE_RTC_PUBLIC_IP ?? '';
/* A channel this far behind skips frames (as the WebSocket's backlog does). */
const BACKLOG_BYTES = 64 * 1024;
/* The client pings twice a second. A channel carries frames only once the
 * client was heard on it (it works both ways), and is given up when silent
 * this long (its close may never arrive): frames go over the WebSocket again. */
const SILENT_MS = 2000;

/** Replaces the address of host candidates with the public one. */
function advertised(candidate: string): string {
  if (!PUBLIC_IP) return candidate;
  // candidate:<foundation> <component> <transport> <priority> <address> <port> typ <type> ...
  return candidate.replace(/^((?:a=)?candidate:\S+ \d+ \S+ \d+ )(\S+)( \d+ typ host)/gm, `$1${PUBLIC_IP}$3`);
}

export class RtcLink {
  private readonly pc: PeerConnection;
  private channel: DataChannel | null = null;
  private closed = false;
  private lastHeard = 0; // 0 until the client is first heard on the channel

  constructor(offer: string, signal: (message: ServerMessage) => void, onInput: (data: Buffer) => void) {
    this.pc = new PeerConnection('race', {
      iceServers: [],
      ...(PORT ? { portRangeBegin: PORT, portRangeEnd: PORT, enableIceUdpMux: true } : {}),
    });
    this.pc.onLocalDescription((sdp, type) => {
      if (type === 'answer') signal({ t: 'rtcAnswer', sdp: advertised(sdp) });
    });
    this.pc.onLocalCandidate((candidate, mid) => {
      signal({ t: 'rtcCandidate', candidate: advertised(candidate).replace(/^a=/, ''), mid });
    });
    this.pc.onDataChannel((channel) => {
      if (this.closed) return channel.close();
      this.channel = channel;
      this.lastHeard = 0;
      channel.onMessage((message) => {
        if (typeof message === 'string') return;
        this.lastHeard = performance.now();
        const data = Buffer.from(message as ArrayBuffer);
        if (data[0] === BINARY_PING) channel.sendMessageBinary(data);
        else onInput(data);
      });
      channel.onClosed(() => { if (this.channel === channel) this.channel = null; });
    });
    this.pc.setRemoteDescription(offer, 'offer');
  }

  candidate(candidate: string, mid: string): void {
    if (this.closed || !candidate) return;
    try { this.pc.addRemoteCandidate(candidate, mid); } catch { /* malformed: ignored */ }
  }

  /** Whether race frames can go this way. */
  get open(): boolean {
    if (!this.channel?.isOpen() || !this.lastHeard) return false;
    if (performance.now() - this.lastHeard <= SILENT_MS) return true;
    this.channel.close();
    this.channel = null;
    return false;
  }

  /** Sends one message; false when it has to go over the WebSocket. */
  send(packet: Uint8Array): boolean {
    const channel = this.channel;
    if (!this.open || !channel || packet.length > channel.maxMessageSize()) return false;
    if (channel.bufferedAmount() < BACKLOG_BYTES) channel.sendMessageBinary(packet);
    return true;
  }

  close(): void {
    if (this.closed) return;
    this.closed = true;
    try { this.channel?.close(); } catch { /* already gone */ }
    this.pc.close();
  }
}
