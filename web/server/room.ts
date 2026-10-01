// One room: its settings, members (players and spectators), its host, the
// race it runs (race.ts), and what its members are told about it.
import type { WebSocket } from 'ws';
import type { DuelSetup, RoomPlayer, Transport, RoomSettings, RoomState, RoomSummary, ServerMessage, UserInfo } from '../shared/protocol.ts';
import type { Race } from './race.ts';
import type { RtcLink } from './rtc.ts';

export interface Client {
  ws: WebSocket;
  user: UserInfo;
  roomId: number | null;
  latencyMs: number | null; // measured by the keep-alive pings (main.ts)
  replaced?: boolean; // superseded by a newer connection of the same account
  rtc?: RtcLink; // the race's data channel, once the client offers one
}

export interface Member {
  client: Client;
  variant: number;
  manual: boolean;
  tire: number; // 0..4; a duel picks it, every other room stays on 0
  ready: boolean;
  spectator: boolean;
  offlineSince: number | null; // connection lost; the place waits a while (rooms.ts)
}

/** Sends a message, or its JSON when several clients get the same one. */
export function send(client: Client, message: ServerMessage | string): void {
  if (client.ws.readyState === client.ws.OPEN) client.ws.send(typeof message === 'string' ? message : JSON.stringify(message));
}

export class Room {
  readonly members = new Map<number, Member>(); // insertion order is join order
  race: Race | null = null;
  readonly id: number;
  settings: RoomSettings;
  hostId: number;
  readonly duel: DuelSetup | null; // one shared car; joined by its token, not the room list

  constructor(id: number, settings: RoomSettings, hostId: number, duel: DuelSetup | null = null) {
    this.id = id;
    this.settings = settings;
    this.hostId = hostId;
    this.duel = duel;
  }

  /** The members on the grid (not watching). */
  racers(): Member[] {
    return [...this.members.values()].filter((m) => !m.spectator);
  }

  isHost(client: Client): boolean {
    return this.hostId === client.user.id;
  }

  /** Sends one message to every member, serialized once. */
  toAll(message: ServerMessage): void {
    const text = JSON.stringify(message);
    for (const member of this.members.values()) send(member.client, text);
  }

  /** A line in the chat from nobody: joins, leaves, connection news. */
  systemChat(text: string): void {
    this.toAll({ t: 'chat', from: '', text, at: Date.now() });
  }

  /** How each online member's race traffic travels: the data channel or
   *  the WebSocket fallback. */
  transports(): Record<number, Transport> {
    const transport: Record<number, Transport> = {};
    for (const [id, member] of this.members) {
      if (member.offlineSince === null) transport[id] = member.client.rtc?.open ? 'rtc' : 'ws';
    }
    return transport;
  }

  /** Every member's round trip (null while offline), for the latency message. */
  latencies(): Record<number, number | null> {
    const latency: Record<number, number | null> = {};
    for (const [id, member] of this.members) latency[id] = member.offlineSince === null ? member.client.latencyMs : null;
    return latency;
  }

  summary(): RoomSummary {
    const host = this.members.get(this.hostId)?.client.user.name ?? '';
    const racers = this.racers().length;
    return { id: this.id, settings: this.settings, host, players: racers, spectators: this.members.size - racers,
             status: this.race ? (this.race.started ? 'racing' : 'loading') : 'lobby' };
  }

  /** What the members see of the room. The host of an ordinary room counts
   *  as ready (they start the race); both duel drivers say so themselves. */
  state(): RoomState {
    const members: RoomPlayer[] = [...this.members.values()].map((m) => ({
      userId: m.client.user.id, name: m.client.user.name, variant: m.variant, manual: m.manual, tire: m.tire,
      ready: this.duel ? m.ready : m.ready || m.client.user.id === this.hostId, host: m.client.user.id === this.hostId,
      spectator: m.spectator, online: m.offlineSince === null,
    }));
    return { ...this.summary(), members, duel: this.duel };
  }
}
