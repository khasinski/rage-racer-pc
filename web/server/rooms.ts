// The lobby: connections, the rooms (room.ts) and what members ask of them,
// and the clock that steps every running race (race-clock.ts). Each room runs
// at most one race at a time (race.ts).
import { randomInt } from 'node:crypto';
import {
  CLOSE_REPLACED, PAINTABLE_MODELS, type ClientMessage, type Paint, type RoomSettings, type ServerMessage, type UserInfo,
} from '../shared/protocol.ts';
import type { Store } from './db.ts';
import { Race } from './race.ts';
import { RaceClock } from './race-clock.ts';
import { Room, send, type Client } from './room.ts';
import { carValid, checkSettings, firstCar, normalizeSettings, rollDuel, tireValid } from './room-rules.ts';
import { RTC_ENABLED, RtcLink } from './rtc.ts';
import type { Simulation } from './sim.ts';

export type { Client } from './room.ts';

const CHAT_LIMIT = 200;
const MAX_SPECTATORS = 16;
/* A dropped connection keeps its place this long outside a race (a racing
 * seat waits until the race is over), so a reconnect puts the player back. */
const OFFLINE_GRACE_MS = Number(process.env.RAGE_OFFLINE_GRACE_MS ?? 60_000);

function shuffle<T>(items: T[]): T[] {
  for (let i = items.length - 1; i > 0; i--) {
    const j = randomInt(i + 1);
    [items[i], items[j]] = [items[j], items[i]];
  }
  return items;
}

export class Lobby {
  private readonly rooms = new Map<number, Room>();
  private readonly duels = new Map<string, Room>(); // invite token → room
  private readonly racing = new Set<Room>(); // rooms with a race
  private readonly clients = new Set<Client>();
  private readonly sim: Simulation;
  private readonly store: Store;
  private readonly clock = new RaceClock(() => this.tick());
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
        other.ws.close(CLOSE_REPLACED, 'replaced');
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
    if (wasOffline) room.systemChat(`${client.user.name} is back.`);
    this.publish(room);
    if (room.race?.involves(client.user.id)) this.sendRace(room, client);
  }

  /** An account got a new name (a guest saved it): its connections and room
   *  use it from now on. */
  renamed(user: UserInfo): void {
    for (const client of this.clients) {
      if (client.user.id !== user.id) continue;
      client.user = user;
      const room = this.roomOf(client);
      if (room) this.publish(room);
    }
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
    room.systemChat(`${client.user.name} lost the connection.`);
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
      if (this.rooms.has(room.id)) room.toAll({ t: 'latency', latency: room.latencies(), transport: room.transports() });
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
      case 'createDuel': return this.createDuel(client);
      case 'updateRoom': return this.updateRoom(client, message.settings);
      case 'joinRoom': return this.joinRoom(client, message.roomId);
      case 'joinDuel': return this.joinDuel(client, message.token);
      case 'leaveRoom': return this.leave(client, 'left');
      case 'setCar': return this.setCar(client, message.variant, message.manual, message.tire);
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

  private createRoom(client: Client, settings: RoomSettings): void {
    const error = checkSettings(this.sim, settings);
    if (error) return this.fail(client, error);
    if (client.roomId !== null) this.leave(client, 'left');
    const s = normalizeSettings(settings);
    const room = new Room(this.store.createRoom(client.user.id, s), s, client.user.id);
    this.rooms.set(room.id, room);
    this.addMember(room, client);
  }

  private createDuel(client: Client): void {
    const rolled = rollDuel(this.sim, client.user.name, (token) => this.duels.has(token));
    if (!rolled) return this.fail(client, 'A duel could not be set up.');
    if (client.roomId !== null) this.leave(client, 'left');
    const room = new Room(this.store.createRoom(client.user.id, rolled.settings), rolled.settings, client.user.id, rolled.duel);
    this.rooms.set(room.id, room);
    this.duels.set(rolled.duel.token, room);
    this.addMember(room, client);
  }

  private updateRoom(client: Client, settings: RoomSettings): void {
    const room = this.roomOf(client);
    if (!room) return;
    if (room.duel) return this.fail(client, 'A duel keeps the car and course it was given.');
    if (!room.isHost(client)) return this.fail(client, 'Only the host changes the room.');
    if (room.race) return this.fail(client, 'Wait until the race is over.');
    const error = checkSettings(this.sim, settings);
    if (error) return this.fail(client, error);
    const racers = room.racers().length;
    if (settings.maxPlayers < racers) return this.fail(client, `${racers} players are already on the grid.`);
    room.settings = normalizeSettings(settings);
    for (const member of room.members.values()) {
      member.ready = false;
      if (!carValid(this.sim, room.settings.classIndex, member.variant, member.manual)) member.variant = -1;
    }
    this.publish(room);
  }

  private joinRoom(client: Client, roomId: number): void {
    const room = this.rooms.get(roomId);
    if (!room) return this.fail(client, 'That room is gone.');
    if (room.members.has(client.user.id)) return;
    if (room.duel) return this.fail(client, 'That duel is joined with its link.');
    // A full or racing room still takes spectators.
    const spectator = room.race !== null || room.racers().length >= room.settings.maxPlayers;
    if (spectator && room.members.size >= room.settings.maxPlayers + MAX_SPECTATORS) return this.fail(client, 'That room is full.');
    if (client.roomId !== null) this.leave(client, 'left');
    this.addMember(room, client, spectator);
    if (room.race) this.sendRace(room, client);
  }

  /** The other driver, from the page link. A full duel does not take a third. */
  private joinDuel(client: Client, token: unknown): void {
    if (typeof token !== 'string' || token.length === 0) return this.fail(client, 'That duel link is not valid.');
    const room = this.duels.get(token);
    if (!room) return this.fail(client, 'That duel is over.');
    if (room.members.has(client.user.id)) return this.publish(room);
    if (room.race || room.racers().length >= room.settings.maxPlayers) {
      return this.fail(client, 'That duel already has two drivers.');
    }
    if (client.roomId !== null) this.leave(client, 'left');
    this.addMember(room, client);
  }

  /** A racer's saved paint for their car; the special cars have none. */
  private paintOf(userId: number, variant: number): Paint | null {
    const model = this.sim.carModel(variant);
    return model < PAINTABLE_MODELS ? this.store.garage(userId)[model] ?? null : null;
  }

  private setSpectator(client: Client, spectator: boolean): void {
    const room = this.roomOf(client);
    const member = room?.members.get(client.user.id);
    if (!room || !member || room.race || typeof spectator !== 'boolean' || member.spectator === spectator) return;
    if (room.duel) return this.fail(client, 'A duel is the two drivers.');
    if (!spectator && room.racers().length >= room.settings.maxPlayers) return this.fail(client, 'The grid is full.');
    member.spectator = spectator;
    member.ready = false;
    this.publish(room);
  }

  private addMember(room: Room, client: Client, spectator = false): void {
    // A duel's car, or the first car the class offers.
    const variant = room.duel ? room.duel.variant : firstCar(this.sim, room.settings.classIndex);
    const manual = room.duel ? room.duel.manual : !this.sim.carAutomatic(variant);
    room.members.set(client.user.id, { client, variant, manual, tire: 0, ready: false, spectator, offlineSince: null });
    client.roomId = room.id;
    room.systemChat(`${client.user.name} joined.`);
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
    if (room.isHost(client)) room.hostId = room.members.keys().next().value as number;
    room.systemChat(`${client.user.name} ${reason === 'disconnected' ? 'disconnected' : 'left'}.`);
    this.publish(room);
  }

  private closeRoom(client: Client, roomId: number): void {
    const room = this.rooms.get(roomId);
    if (!room) return;
    if (!room.isHost(client) && !client.user.admin) return this.fail(client, 'Only the host or an admin can close a room.');
    room.toAll({ t: 'room', room: null });
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
    if (room.duel) this.duels.delete(room.duel.token);
    this.rooms.delete(room.id);
    this.broadcastRooms();
  }

  private setCar(client: Client, variant: number, manual: boolean, tire?: number): void {
    const room = this.roomOf(client);
    const member = room?.members.get(client.user.id);
    if (!room || !member || room.race) return;
    const classIndex = room.settings.classIndex;
    if (room.duel) {
      if (!Number.isInteger(variant) || variant !== room.duel.variant) {
        return this.fail(client, 'A duel gives both drivers the same car.');
      }
      if (typeof manual !== 'boolean' || !carValid(this.sim, classIndex, variant, manual) || !tireValid(tire)) {
        return this.fail(client, 'That setup is not available for this car.');
      }
      if (member.manual === manual && member.tire === tire) return;
      member.manual = manual;
      member.tire = tire;
      member.ready = false;
      this.publish(room);
      return;
    }
    if (!Number.isInteger(variant) || typeof manual !== 'boolean' || !carValid(this.sim, classIndex, variant, manual)) {
      return this.fail(client, 'That car is not available in this class.');
    }
    member.variant = variant;
    member.manual = manual;
    member.tire = 0;
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
    if (clean) room.toAll({ t: 'chat', from: client.user.name, text: clean, at: Date.now() });
  }

  // ---- races -------------------------------------------------------------

  private startRace(client: Client): void {
    const room = this.roomOf(client);
    if (!room) return;
    if (!room.isHost(client)) return this.fail(client, 'Only the host starts the race.');
    if (room.race) return;
    // Grid places are drawn afresh for every race: nobody always starts behind.
    const members = shuffle(room.racers());
    if (!members.length) return this.fail(client, 'Nobody is racing: somebody has to leave the stands.');
    if (room.duel && members.length < 2) return this.fail(client, 'A duel starts when both drivers are here.');
    const unready = members.filter((m) => !m.ready && (room.duel || m.client.user.id !== room.hostId));
    if (unready.length) return this.fail(client, `Waiting for ${unready.map((m) => m.client.user.name).join(', ')}.`);
    if (members.some((m) => !carValid(this.sim, room.settings.classIndex, m.variant, m.manual))) {
      return this.fail(client, 'Every player needs a car for this class.');
    }
    const race = Race.create(this.sim, this.store, {
      broadcast: (message) => room.toAll(message),
      client: (userId) => {
        const member = room.members.get(userId);
        return member && member.offlineSince === null ? member.client : undefined;
      },
    }, room.id, room.settings, members.map((m) => ({
      userId: m.client.user.id, name: m.client.user.name, variant: m.variant, manual: m.manual, tire: m.tire,
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
    this.clock.ensureRunning();
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

  /** One 50 Hz step of every running race; false once none runs. */
  private tick(): boolean {
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
    return this.racing.size > 0;
  }

  private endRace(room: Room): void {
    room.race = null;
    this.racing.delete(room);
  }

  // ---- state publication -------------------------------------------------

  private roomOf(client: Client): Room | undefined {
    return client.roomId === null ? undefined : this.rooms.get(client.roomId);
  }

  private publish(room: Room): void {
    room.toAll({ t: 'room', room: room.state() });
    this.broadcastRooms();
  }

  /** The room list goes to everyone, only when it changed. Duels stay off it. */
  private broadcastRooms(): void {
    const rooms = [...this.rooms.values()].filter((room) => !room.duel).map((room) => room.summary());
    const list = JSON.stringify({ t: 'rooms', rooms } satisfies ServerMessage);
    if (list === this.roomList) return;
    this.roomList = list;
    for (const client of this.clients) send(client, list);
  }
}
