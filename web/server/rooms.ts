// The lobby: connections, rooms, their members and chat. Each room runs at
// most one race at a time (race.ts); one 50 Hz timer steps every running race.
import { randomInt } from 'node:crypto';
import type { WebSocket } from 'ws';
import {
  PAINTABLE_MODELS, type ClientMessage, type Paint, type RoomPlayer, type RoomSettings, type RoomState, type RoomSummary, type ServerMessage, type UserInfo,
} from '../shared/protocol.ts';
import type { Store } from './db.ts';
import { Race } from './race.ts';
import { RTC_ENABLED, RtcLink } from './rtc.ts';
import type { Simulation } from './sim.ts';

const TICK_MS = 1000 / 50;
const CHAT_LIMIT = 200;
const MAX_SPECTATORS = 16;
/* A dropped connection keeps its place this long outside a race (a racing
 * seat waits until the race is over), so a reconnect puts the player back. */
const OFFLINE_GRACE_MS = Number(process.env.RAGE_OFFLINE_GRACE_MS ?? 60_000);

export interface Client {
  ws: WebSocket;
  user: UserInfo;
  roomId: number | null;
  latencyMs: number | null; // measured by the keep-alive pings (main.ts)
  replaced?: boolean; // superseded by a newer connection of the same account
  rtc?: RtcLink; // the race's data channel, once the client offers one
}

interface Member {
  client: Client;
  variant: number;
  manual: boolean;
  ready: boolean;
  spectator: boolean;
  offlineSince: number | null; // connection lost; the place waits OFFLINE_GRACE_MS
}

interface Room {
  id: number;
  settings: RoomSettings;
  hostId: number;
  members: Map<number, Member>; // insertion order is join order
  race: Race | null;
}

function shuffle<T>(items: T[]): T[] {
  for (let i = items.length - 1; i > 0; i--) {
    const j = randomInt(i + 1);
    [items[i], items[j]] = [items[j], items[i]];
  }
  return items;
}

/** Sends a message, or its JSON when several clients get the same one. */
const send = (client: Client, message: ServerMessage | string) => {
  if (client.ws.readyState === client.ws.OPEN) client.ws.send(typeof message === 'string' ? message : JSON.stringify(message));
};

export class Lobby {
  private readonly rooms = new Map<number, Room>();
  private readonly racing = new Set<Room>(); // rooms with a race
  private readonly clients = new Set<Client>();
  private readonly sim: Simulation;
  private readonly store: Store;
  private timer: ReturnType<typeof setInterval> | null = null;
  private roomList = JSON.stringify({ t: 'rooms', rooms: [] } satisfies ServerMessage); // as last broadcast

  constructor(sim: Simulation, store: Store) {
    this.sim = sim;
    this.store = store;
    setInterval(() => this.sweep(Date.now()), 2000).unref();
  }

  // ---- connections -------------------------------------------------------

  connect(client: Client): void {
    // One live connection per account: a newer one takes over the older.
    for (const other of this.clients) {
      if (other.user.id === client.user.id) {
        other.replaced = true;
        this.clients.delete(other);
        send(other, { t: 'error', message: 'You connected somewhere else.' });
        other.ws.close(4001, 'replaced');
      }
    }
    this.clients.add(client);
    send(client, { t: 'welcome', user: client.user, discId: this.sim.discId });
    send(client, this.roomList);
    // Back from a dropped or replaced connection: the same place, and the race.
    const room = [...this.rooms.values()].find((r) => r.members.has(client.user.id));
    const member = room?.members.get(client.user.id);
    if (!room || !member) return;
    const wasOffline = member.offlineSince !== null;
    member.client.roomId = null;
    member.client = client;
    member.offlineSince = null;
    client.roomId = room.id;
    if (wasOffline) this.systemChat(room, `${client.user.name} is back.`);
    this.publish(room);
    if (room.race?.involves(client.user.id)) this.sendRace(room, client);
  }

  /** A lost connection keeps its place: offline for a while, not gone. */
  disconnect(client: Client): void {
    client.rtc?.close();
    if (!this.clients.delete(client) || client.replaced) return;
    const room = this.roomOf(client);
    const member = room?.members.get(client.user.id);
    if (!room || !member || member.client !== client) return;
    member.offlineSince = Date.now();
    member.ready = false;
    room.race?.coast(client.user.id);
    this.systemChat(room, `${client.user.name} lost the connection.`);
    this.publish(room);
  }

  /** Members offline past the grace period leave; a racing seat waits for
   *  the race to end. Every couple of seconds, rooms hear their latencies. */
  private sweep(now: number): void {
    for (const room of [...this.rooms.values()]) {
      for (const member of [...room.members.values()]) {
        if (member.offlineSince === null || now - member.offlineSince < OFFLINE_GRACE_MS) continue;
        if (room.race?.seated(member.client.user.id)) continue;
        this.leave(member.client, 'disconnected');
      }
      if (!this.rooms.has(room.id)) continue;
      const latency: Record<number, number | null> = {};
      for (const [id, member] of room.members) latency[id] = member.offlineSince === null ? member.client.latencyMs : null;
      this.toRoom(room, { t: 'latency', latency });
    }
  }

  message(client: Client, data: Buffer, binary: boolean): void {
    if (binary) {
      this.roomOf(client)?.race?.input(client.user.id, data);
      return;
    }
    let message: ClientMessage;
    try {
      message = JSON.parse(data.toString('utf8')) as ClientMessage;
    } catch {
      return this.fail(client, 'Malformed message.');
    }
    switch (message.t) {
      case 'createRoom': return this.createRoom(client, message.settings);
      case 'updateRoom': return this.updateRoom(client, message.settings);
      case 'joinRoom': return this.joinRoom(client, message.roomId);
      case 'leaveRoom': return this.leave(client, 'left');
      case 'setCar': return this.setCar(client, message.variant, message.manual);
      case 'setReady': return this.setReady(client, message.ready);
      case 'setSpectator': return this.setSpectator(client, message.spectator);
      case 'watchRace': return this.watchRace(client);
      case 'startRace': return this.startRace(client);
      case 'loaded': return this.loaded(client, message.ok);
      case 'leaveRace': return this.roomOf(client)?.race?.leave(client.user.id, 'left the race');
      case 'chat': return this.chat(client, message.text);
      case 'closeRoom': return this.closeRoom(client, message.roomId);
      case 'rtcOffer': return this.rtcOffer(client, message.sdp);
      case 'rtcCandidate': return client.rtc?.candidate(message.candidate, message.mid);
      default: return this.fail(client, 'Unknown message.');
    }
  }

  /** Answers a client's data channel offer (one channel per connection). */
  private rtcOffer(client: Client, sdp: string): void {
    if (!RTC_ENABLED || typeof sdp !== 'string') return;
    client.rtc?.close();
    try {
      client.rtc = new RtcLink(sdp, (message) => send(client, message), (data) => this.message(client, data, true));
    } catch {
      client.rtc = undefined; // a malformed offer: the race stays on the WebSocket
    }
  }

  private fail(client: Client, message: string): void {
    send(client, { t: 'error', message });
  }

  // ---- rooms -------------------------------------------------------------

  private validSettings(s: RoomSettings): string | null {
    if (!s || typeof s !== 'object') return 'Missing room settings.';
    const int = (v: unknown, lo: number, hi: number) => Number.isInteger(v) && (v as number) >= lo && (v as number) <= hi;
    if (typeof s.name !== 'string' || !s.name.trim() || s.name.length > 40) return 'Give the room a name (up to 40 characters).';
    if (!int(s.classIndex, 0, 5)) return 'Unknown class.';
    if (!int(s.course, 0, 3) || !this.sim.courseAllowed(s.classIndex, s.course)) return 'That course is not raced in this class.';
    if (!int(s.laps, 1, 6)) return 'Laps must be between 1 and 6.';
    if (typeof s.reverse !== 'boolean' || typeof s.rivals !== 'boolean') return 'Malformed room settings.';
    const most = this.sim.maxHumans(s.classIndex, s.course, s.reverse);
    if (!int(s.maxPlayers, 1, most)) return `This course has ${most} starting places.`;
    return null;
  }

  private normalize(s: RoomSettings): RoomSettings {
    return { name: s.name.trim(), classIndex: s.classIndex, course: s.course, reverse: s.reverse,
             laps: s.laps, rivals: s.rivals, maxPlayers: s.maxPlayers };
  }

  private createRoom(client: Client, settings: RoomSettings): void {
    const error = this.validSettings(settings);
    if (error) return this.fail(client, error);
    if (client.roomId !== null) this.leave(client, 'left');
    const s = this.normalize(settings);
    const id = this.store.createRoom(client.user.id, s);
    const room: Room = { id, settings: s, hostId: client.user.id, members: new Map(), race: null };
    this.rooms.set(id, room);
    this.addMember(room, client);
  }

  private updateRoom(client: Client, settings: RoomSettings): void {
    const room = this.roomOf(client);
    if (!room) return;
    if (room.hostId !== client.user.id) return this.fail(client, 'Only the host changes the room.');
    if (room.race) return this.fail(client, 'Wait until the race is over.');
    const error = this.validSettings(settings);
    if (error) return this.fail(client, error);
    const racers = this.racers(room).length;
    if (settings.maxPlayers < racers) return this.fail(client, `${racers} players are already on the grid.`);
    room.settings = this.normalize(settings);
    for (const member of room.members.values()) {
      member.ready = false;
      if (!this.carValid(room, member.variant, member.manual)) member.variant = -1;
    }
    this.publish(room);
  }

  private joinRoom(client: Client, roomId: number): void {
    const room = this.rooms.get(roomId);
    if (!room) return this.fail(client, 'That room is gone.');
    if (room.members.has(client.user.id)) return;
    // A full or racing room still takes spectators.
    const spectator = room.race !== null || this.racers(room).length >= room.settings.maxPlayers;
    if (spectator && room.members.size >= room.settings.maxPlayers + MAX_SPECTATORS) return this.fail(client, 'That room is full.');
    if (client.roomId !== null) this.leave(client, 'left');
    this.addMember(room, client, spectator);
    if (room.race) this.sendRace(room, client);
  }

  /** A racer's saved paint for their car; the special cars have none. */
  private paintOf(userId: number, variant: number): Paint | null {
    const model = this.sim.carModel(variant);
    return model < PAINTABLE_MODELS ? this.store.garage(userId)[model] ?? null : null;
  }

  private racers(room: Room): Member[] {
    return [...room.members.values()].filter((m) => !m.spectator);
  }

  private setSpectator(client: Client, spectator: boolean): void {
    const room = this.roomOf(client);
    const member = room?.members.get(client.user.id);
    if (!room || !member || room.race || typeof spectator !== 'boolean' || member.spectator === spectator) return;
    if (!spectator && this.racers(room).length >= room.settings.maxPlayers) return this.fail(client, 'The grid is full.');
    member.spectator = spectator;
    member.ready = false;
    this.publish(room);
  }

  private addMember(room: Room, client: Client, spectator = false): void {
    // Start with the first car the class offers that the player can drive.
    let variant = -1;
    for (let model = 0; model < this.sim.carModels() && variant < 0; model++) {
      const candidate = this.sim.classCar(room.settings.classIndex, model);
      if (candidate >= 0) variant = candidate;
    }
    room.members.set(client.user.id, { client, variant, manual: !this.sim.carAutomatic(variant), ready: false, spectator, offlineSince: null });
    client.roomId = room.id;
    this.systemChat(room, `${client.user.name} joined.`);
    this.publish(room);
  }

  /** Leaves the current room: retires a racing seat, hands over the host. */
  private leave(client: Client, reason: string): void {
    const room = this.roomOf(client);
    client.roomId = null;
    if (!room) return;
    room.members.delete(client.user.id);
    room.race?.leave(client.user.id, reason);
    send(client, { t: 'room', room: null });
    if (room.members.size === 0) return this.dropRoom(room);
    if (room.hostId === client.user.id) room.hostId = room.members.keys().next().value as number;
    this.systemChat(room, `${client.user.name} ${reason === 'disconnected' ? 'disconnected' : 'left'}.`);
    this.publish(room);
  }

  private closeRoom(client: Client, roomId: number): void {
    const room = this.rooms.get(roomId);
    if (!room) return;
    if (room.hostId !== client.user.id && !client.user.admin) return this.fail(client, 'Only the host or an admin can close a room.');
    this.toRoom(room, { t: 'room', room: null });
    for (const member of room.members.values()) {
      member.client.roomId = null;
      if (member.client !== client) send(member.client, { t: 'error', message: `The room “${room.settings.name}” was closed.` });
    }
    room.members.clear();
    this.dropRoom(room);
  }

  private dropRoom(room: Room): void {
    room.race?.free();
    this.endRace(room);
    this.rooms.delete(room.id);
    this.broadcastRooms();
  }

  private carValid(room: Room, variant: number, manual: boolean): boolean {
    return this.sim.carAllowed(room.settings.classIndex, variant) && (manual || this.sim.carAutomatic(variant));
  }

  private setCar(client: Client, variant: number, manual: boolean): void {
    const room = this.roomOf(client);
    const member = room?.members.get(client.user.id);
    if (!room || !member || room.race) return;
    if (!Number.isInteger(variant) || typeof manual !== 'boolean' || !this.carValid(room, variant, manual)) {
      return this.fail(client, 'That car is not available in this class.');
    }
    member.variant = variant;
    member.manual = manual;
    member.ready = false;
    this.publish(room);
  }

  private setReady(client: Client, ready: boolean): void {
    const room = this.roomOf(client);
    const member = room?.members.get(client.user.id);
    if (!room || !member || room.race) return;
    if (ready && member.variant < 0) return this.fail(client, 'Choose a car first.');
    member.ready = ready === true;
    this.publish(room);
  }

  private chat(client: Client, text: string): void {
    const room = this.roomOf(client);
    if (!room || typeof text !== 'string') return;
    const clean = text.replace(/\s+/g, ' ').trim().slice(0, CHAT_LIMIT);
    if (clean) this.toRoom(room, { t: 'chat', from: client.user.name, text: clean, at: Date.now() });
  }

  private systemChat(room: Room, text: string): void {
    this.toRoom(room, { t: 'chat', from: '', text, at: Date.now() });
  }

  // ---- races -------------------------------------------------------------

  private startRace(client: Client): void {
    const room = this.roomOf(client);
    if (!room) return;
    if (room.hostId !== client.user.id) return this.fail(client, 'Only the host starts the race.');
    if (room.race) return;
    // Grid places are drawn afresh for every race: nobody always starts behind.
    const members = shuffle(this.racers(room));
    if (!members.length) return this.fail(client, 'Nobody is racing: somebody has to leave the stands.');
    const unready = members.filter((m) => m.client.user.id !== room.hostId && !m.ready);
    if (unready.length) return this.fail(client, `Waiting for ${unready.map((m) => m.client.user.name).join(', ')}.`);
    if (members.some((m) => !this.carValid(room, m.variant, m.manual))) return this.fail(client, 'Every player needs a car for this class.');
    const race = Race.create(this.sim, this.store, {
      broadcast: (message) => this.toRoom(room, message),
      client: (userId) => {
        const member = room.members.get(userId);
        return member && member.offlineSince === null ? member.client : undefined;
      },
    }, room.id, room.settings, members.map((m) => ({
      userId: m.client.user.id, name: m.client.user.name, variant: m.variant, manual: m.manual,
      paint: this.paintOf(m.client.user.id, m.variant),
      logo: this.store.logo(m.client.user.id)?.toString('base64') ?? null,
    })));
    if (!race) return this.fail(client, 'The server could not prepare this race.');
    room.race = race;
    this.racing.add(room);
    for (const member of room.members.values()) {
      member.ready = false;
      this.sendRace(room, member.client);
    }
    this.publish(room);
    this.ensureTimer();
  }

  /** Sends the running race to a member: everyone at the start, then a
   *  spectator, or a player coming back (to their own seat). */
  private sendRace(room: Room, client: Client): void {
    for (const message of room.race?.watch(client.user.id) ?? []) send(client, message);
  }

  private watchRace(client: Client): void {
    const room = this.roomOf(client);
    if (room) this.sendRace(room, client);
  }

  private loaded(client: Client, ok: boolean): void {
    const race = this.roomOf(client)?.race;
    if (!race?.awaits(client.user.id)) return;
    if (ok) race.loaded(client.user.id);
    else this.leave(client, 'could not load the race');
  }

  private ensureTimer(): void {
    if (this.timer) return;
    let next = performance.now();
    this.timer = setInterval(() => {
      // Catch up on late timer callbacks so races keep the 50 Hz clock.
      const now = performance.now();
      let steps = 0;
      while (next <= now && steps++ < 10) {
        this.tick();
        next += TICK_MS;
      }
      if (next < now) next = now;
      if (!this.racing.size) {
        clearInterval(this.timer!);
        this.timer = null;
      }
    }, 4);
  }

  private tick(): void {
    const now = Date.now();
    for (const room of this.racing) {
      for (const id of room.race!.overdue(now)) {
        const member = room.members.get(id);
        if (member) this.leave(member.client, 'could not load the race in time');
      }
      const race = room.race;
      if (!race) continue; // everybody left
      const outcome = race.step(now);
      if (outcome === 'over') this.endRace(room);
      if (outcome !== 'running') this.publish(room);
    }
  }

  private endRace(room: Room): void {
    room.race = null;
    this.racing.delete(room);
  }

  // ---- state publication -------------------------------------------------

  private roomOf(client: Client): Room | undefined {
    return client.roomId === null ? undefined : this.rooms.get(client.roomId);
  }

  /** Sends one message to every member of a room, serialized once. */
  private toRoom(room: Room, message: ServerMessage): void {
    const text = JSON.stringify(message);
    for (const member of room.members.values()) send(member.client, text);
  }

  private summary(room: Room): RoomSummary {
    const host = room.members.get(room.hostId)?.client.user.name ?? '';
    const racers = this.racers(room).length;
    return { id: room.id, settings: room.settings, host, players: racers, spectators: room.members.size - racers,
             status: room.race ? (room.race.started ? 'racing' : 'loading') : 'lobby' };
  }

  private publish(room: Room): void {
    const members: RoomPlayer[] = [...room.members.values()].map((m) => ({
      userId: m.client.user.id, name: m.client.user.name, variant: m.variant, manual: m.manual,
      ready: m.ready || m.client.user.id === room.hostId, host: m.client.user.id === room.hostId,
      spectator: m.spectator, online: m.offlineSince === null,
    }));
    const state: RoomState = { ...this.summary(room), members };
    this.toRoom(room, { t: 'room', room: state });
    this.broadcastRooms();
  }

  /** The room list goes to everyone, only when it changed. */
  private broadcastRooms(): void {
    const list = JSON.stringify({ t: 'rooms', rooms: [...this.rooms.values()].map((room) => this.summary(room)) } satisfies ServerMessage);
    if (list === this.roomList) return;
    this.roomList = list;
    for (const client of this.clients) send(client, list);
  }
}
