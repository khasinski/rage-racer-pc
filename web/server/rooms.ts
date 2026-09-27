// Rooms, lobby and the authoritative races. The server owns every race: it
// builds the field, steps it at 50 Hz, applies the players' inputs, streams
// the complete RaceFrame to each player, reports race events and stores the
// results.
import type { WebSocket } from 'ws';
import {
  BINARY_FRAME, BINARY_INPUT, INPUT_WORDS,
  type ClientMessage, type RaceEvent, type RaceResult, type RaceSeat, type RoomPlayer,
  type RoomSettings, type RoomState, type RoomSummary, type ServerMessage, type UserInfo,
} from '../shared/protocol.ts';
import type { Store } from './db.ts';
import {
  PHASE_FINISHED, STATUS_DRIVING, STATUS_EMPTY, STATUS_FINISHED, STATUS_RETIRED, type Simulation,
} from './sim.ts';

const TICK_MS = 1000 / 50;
const LOAD_TIMEOUT_MS = 30_000;
const FINISH_GRACE_MS = 60_000; // after the first player finishes, the others get this long
const END_DELAY_MS = 2_500; // lets the last finisher see the line before results
const CHAT_LIMIT = 200;

export interface Client {
  ws: WebSocket;
  user: UserInfo;
  roomId: number | null;
}

interface Member {
  client: Client;
  variant: number;
  manual: boolean;
  ready: boolean;
}

interface RaceRun {
  raceId: number;
  handle: number;
  seats: RaceSeat[];
  seatOf: Map<number, number>; // userId -> seat
  humans: number;
  loaded: Set<number>;
  loadDeadline: number;
  started: boolean;
  laps: number[];
  status: number[];
  firstFinishAt: number | null;
  endAt: number | null;
}

interface Room {
  id: number;
  settings: RoomSettings;
  hostId: number;
  members: Map<number, Member>; // insertion order is join order
  race: RaceRun | null;
}

const send = (client: Client, message: ServerMessage) => {
  if (client.ws.readyState === client.ws.OPEN) client.ws.send(JSON.stringify(message));
};

export class Lobby {
  private readonly rooms = new Map<number, Room>();
  private readonly clients = new Set<Client>();
  private readonly sim: Simulation;
  private readonly store: Store;
  private timer: ReturnType<typeof setInterval> | null = null;
  private clock = 0;

  constructor(sim: Simulation, store: Store) {
    this.sim = sim;
    this.store = store;
  }

  // ---- connections -------------------------------------------------------

  connect(client: Client): void {
    // One live connection per account: a new login replaces the old one.
    for (const other of this.clients) {
      if (other.user.id === client.user.id) {
        send(other, { t: 'error', message: 'You logged in somewhere else.' });
        this.disconnect(other);
        other.ws.close(4001, 'replaced');
      }
    }
    this.clients.add(client);
    send(client, { t: 'welcome', user: client.user, discId: this.sim.discId });
    send(client, { t: 'rooms', rooms: this.summaries() });
  }

  disconnect(client: Client): void {
    if (!this.clients.delete(client)) return;
    this.leave(client, 'disconnected');
  }

  message(client: Client, data: Buffer, binary: boolean): void {
    if (binary) {
      this.input(client, data);
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
      case 'startRace': return this.startRace(client);
      case 'loaded': return this.loaded(client, message.ok);
      case 'leaveRace': return this.leaveRace(client);
      case 'chat': return this.chat(client, message.text);
      case 'closeRoom': return this.closeRoom(client, message.roomId);
      default: return this.fail(client, 'Unknown message.');
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
    if (settings.maxPlayers < room.members.size) return this.fail(client, `${room.members.size} players are already here.`);
    room.settings = this.normalize(settings);
    this.store.updateRoom(room.id, room.hostId, room.settings);
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
    if (room.race) return this.fail(client, 'That room is racing; join when the race is over.');
    if (room.members.size >= room.settings.maxPlayers) return this.fail(client, 'That room is full.');
    if (client.roomId !== null) this.leave(client, 'left');
    this.addMember(room, client);
  }

  private addMember(room: Room, client: Client): void {
    // Start with the first car the class offers that the player can drive.
    let variant = -1;
    for (let model = 0; model < this.sim.carModels() && variant < 0; model++) {
      const candidate = this.sim.classCar(room.settings.classIndex, model);
      if (candidate >= 0) variant = candidate;
    }
    room.members.set(client.user.id, { client, variant, manual: !this.sim.carAutomatic(variant), ready: false });
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
    const run = room.race;
    if (run) {
      const seat = run.seatOf.get(client.user.id);
      if (seat !== undefined) {
        run.loaded.add(client.user.id);
        if (run.started) this.sim.retire(run.handle, seat);
        this.raceEvent(room, { kind: 'retire', seat, name: client.user.name, reason });
      }
    }
    send(client, { t: 'room', room: null });
    if (room.members.size === 0) return this.dropRoom(room);
    if (room.hostId === client.user.id) {
      room.hostId = room.members.keys().next().value as number;
      this.store.updateRoom(room.id, room.hostId, room.settings);
    }
    this.systemChat(room, `${client.user.name} ${reason === 'disconnected' ? 'disconnected' : 'left'}.`);
    this.publish(room);
  }

  private closeRoom(client: Client, roomId: number): void {
    const room = this.rooms.get(roomId);
    if (!room) return;
    if (room.hostId !== client.user.id && !client.user.admin) return this.fail(client, 'Only the host or an admin can close a room.');
    for (const member of [...room.members.values()]) {
      member.client.roomId = null;
      send(member.client, { t: 'room', room: null });
      if (member.client !== client) send(member.client, { t: 'error', message: `The room “${room.settings.name}” was closed.` });
    }
    room.members.clear();
    this.dropRoom(room);
  }

  private dropRoom(room: Room): void {
    if (room.race) {
      this.sim.free(room.race.handle);
      room.race = null;
    }
    this.rooms.delete(room.id);
    this.store.closeRoom(room.id);
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
    if (!clean) return;
    for (const member of room.members.values()) {
      send(member.client, { t: 'chat', roomId: room.id, from: client.user.name, text: clean, at: Date.now() });
    }
  }

  private systemChat(room: Room, text: string): void {
    for (const member of room.members.values()) {
      send(member.client, { t: 'chat', roomId: room.id, from: '', text, at: Date.now() });
    }
  }

  // ---- race lifecycle ----------------------------------------------------

  private startRace(client: Client): void {
    const room = this.roomOf(client);
    if (!room) return;
    if (room.hostId !== client.user.id) return this.fail(client, 'Only the host starts the race.');
    if (room.race) return;
    const members = [...room.members.values()];
    const unready = members.filter((m) => m.client.user.id !== room.hostId && !m.ready);
    if (unready.length) return this.fail(client, `Waiting for ${unready.map((m) => m.client.user.name).join(', ')}.`);
    if (members.some((m) => !this.carValid(room, m.variant, m.manual))) return this.fail(client, 'Every player needs a car for this class.');
    const s = room.settings;
    const handle = this.sim.createRace(s.classIndex, s.course, s.reverse, s.laps, s.rivals,
      members.map((m) => ({ variant: m.variant, manual: m.manual })));
    if (!handle) return this.fail(client, 'The server could not prepare this race.');

    const seats: RaceSeat[] = [];
    for (let seat = 0; seat < 12; seat++) {
      const status = this.sim.seat(handle, seat).status;
      if (status === STATUS_EMPTY) continue;
      const member = members[seat];
      seats.push(member
        ? { userId: member.client.user.id, name: member.client.user.name, variant: member.variant, manual: member.manual, ai: false }
        : { userId: null, name: `CPU ${seat - members.length + 1}`, variant: -1, manual: false, ai: true });
    }
    const run: RaceRun = {
      raceId: this.store.createRace(room.id, s), handle, seats,
      seatOf: new Map(members.map((m, seat) => [m.client.user.id, seat])),
      humans: members.length, loaded: new Set(), loadDeadline: Date.now() + LOAD_TIMEOUT_MS, started: false,
      laps: seats.map(() => 0), status: seats.map(() => STATUS_DRIVING), firstFinishAt: null, endAt: null,
    };
    room.race = run;
    for (const member of members) {
      member.ready = false;
      send(member.client, { t: 'raceStart', raceId: run.raceId, settings: s, seats, humans: run.humans,
                            localSeat: run.seatOf.get(member.client.user.id)! });
    }
    this.publish(room);
    this.ensureTimer();
  }

  private loaded(client: Client, ok: boolean): void {
    const room = this.roomOf(client);
    const run = room?.race;
    if (!room || !run || run.started || !run.seatOf.has(client.user.id)) return;
    run.loaded.add(client.user.id);
    if (!ok) this.leave(client, 'could not load the race');
  }

  private leaveRace(client: Client): void {
    const room = this.roomOf(client);
    const run = room?.race;
    const seat = run?.seatOf.get(client.user.id);
    if (!room || !run || seat === undefined) return;
    if (run.started) this.sim.retire(run.handle, seat);
    run.loaded.add(client.user.id);
    this.raceEvent(room, { kind: 'retire', seat, name: client.user.name, reason: 'left the race' });
  }

  private input(client: Client, data: Buffer): void {
    const room = this.roomOf(client);
    const run = room?.race;
    const seat = run?.seatOf.get(client.user.id);
    if (!run || seat === undefined || data.length !== 1 + INPUT_WORDS * 4 || data[0] !== BINARY_INPUT) return;
    const words = new Int32Array(INPUT_WORDS);
    for (let i = 0; i < INPUT_WORDS; i++) words[i] = data.readInt32LE(1 + i * 4);
    this.sim.setInput(run.handle, seat, words);
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
      if (![...this.rooms.values()].some((room) => room.race)) {
        clearInterval(this.timer!);
        this.timer = null;
      }
    }, 4);
  }

  private tick(): void {
    this.clock++;
    const now = Date.now();
    for (const room of this.rooms.values()) {
      const run = room.race;
      if (!run) continue;
      if (!run.started) {
        const waiting = [...run.seatOf.keys()].filter((id) => !run.loaded.has(id));
        if (waiting.length && now < run.loadDeadline) continue;
        for (const id of waiting) {
          const member = room.members.get(id);
          if (member) this.leave(member.client, 'could not load the race in time');
        }
        if (!room.race) continue;
        for (const [id, seat] of run.seatOf) if (!room.members.has(id)) this.sim.retire(run.handle, seat);
        if (!this.sim.start(run.handle)) {
          this.abortRace(room, 'The race could not start.');
          continue;
        }
        run.started = true;
        this.publish(room);
        for (const member of room.members.values()) send(member.client, { t: 'raceGo' });
      }
      const phase = this.sim.tick(run.handle);
      const serverTick = this.sim.simTick(run.handle);
      // The field moves every second tick (25 Hz); publish each such state.
      if (serverTick % 2 === 0 || phase === PHASE_FINISHED) this.publishFrame(room, run, serverTick);
      if (serverTick % 2 === 0) this.detectEvents(room, run, now);
      if (phase === PHASE_FINISHED && run.endAt === null) run.endAt = now + END_DELAY_MS;
      if (run.endAt !== null && now >= run.endAt) this.endRace(room);
    }
  }

  private publishFrame(room: Room, run: RaceRun, serverTick: number): void {
    const frame = this.sim.frame(run.handle);
    if (!frame) return;
    const packet = Buffer.allocUnsafe(5 + frame.length);
    packet[0] = BINARY_FRAME;
    packet.writeUInt32LE(serverTick >>> 0, 1);
    packet.set(frame, 5);
    for (const [id] of run.seatOf) {
      const member = room.members.get(id);
      if (member && member.client.ws.readyState === member.client.ws.OPEN) member.client.ws.send(packet);
    }
  }

  private detectEvents(room: Room, run: RaceRun, now: number): void {
    let humansDone = true;
    run.seats.forEach((seat, index) => {
      const state = this.sim.seat(run.handle, index);
      if (!seat.ai && state.lap > run.laps[index] && run.laps[index] >= 1) {
        const lapMs = this.sim.lapTime(run.handle, index, run.laps[index] - 1);
        if (lapMs >= 0 && state.status === STATUS_DRIVING) {
          this.raceEvent(room, { kind: 'lap', seat: index, name: seat.name, lap: run.laps[index], lapMs });
        }
      }
      run.laps[index] = state.lap;
      if (state.status === STATUS_FINISHED && run.status[index] !== STATUS_FINISHED) {
        this.raceEvent(room, { kind: 'finish', seat: index, name: seat.name, place: state.place, timeMs: state.timeMs });
        if (!seat.ai && run.firstFinishAt === null) run.firstFinishAt = now;
      }
      run.status[index] = state.status;
      if (!seat.ai && state.status === STATUS_DRIVING) humansDone = false;
    });
    if (humansDone && run.endAt === null) run.endAt = now + END_DELAY_MS;
    if (run.firstFinishAt !== null && run.endAt === null && now - run.firstFinishAt > FINISH_GRACE_MS) run.endAt = now;
  }

  private raceEvent(room: Room, event: RaceEvent): void {
    for (const member of room.members.values()) send(member.client, { t: 'raceEvent', event });
  }

  private endRace(room: Room): void {
    const run = room.race;
    if (!run) return;
    const results: RaceResult[] = run.seats.map((seat, index) => {
      const state = this.sim.seat(run.handle, index);
      const finished = state.status === STATUS_FINISHED;
      return {
        seat: index, userId: seat.userId, name: seat.name, variant: seat.ai ? -1 : seat.variant,
        // Cars still running when the race closes keep their running position.
        place: state.status === STATUS_RETIRED ? 0 : state.place,
        timeMs: finished ? state.timeMs : -1, bestLapMs: state.bestLapMs,
        status: finished ? 'finished' : state.status === STATUS_RETIRED ? 'retired' : 'dnf',
      };
    });
    results.sort((a, b) => (a.place || 99) - (b.place || 99) || a.seat - b.seat);
    this.store.finishRace(run.raceId, results);
    this.sim.free(run.handle);
    room.race = null;
    for (const member of room.members.values()) {
      member.ready = false;
      send(member.client, { t: 'results', raceId: run.raceId, results });
    }
    this.publish(room);
  }

  private abortRace(room: Room, message: string): void {
    const run = room.race;
    if (!run) return;
    this.sim.free(run.handle);
    room.race = null;
    for (const member of room.members.values()) send(member.client, { t: 'error', message });
    this.publish(room);
  }

  // ---- state publication -------------------------------------------------

  private roomOf(client: Client): Room | undefined {
    return client.roomId === null ? undefined : this.rooms.get(client.roomId);
  }

  private summary(room: Room): RoomSummary {
    const host = room.members.get(room.hostId)?.client.user.name ?? '';
    return { id: room.id, settings: room.settings, host, players: room.members.size,
             status: room.race ? (room.race.started ? 'racing' : 'loading') : 'lobby' };
  }

  private summaries(): RoomSummary[] {
    return [...this.rooms.values()].map((room) => this.summary(room));
  }

  private publish(room: Room): void {
    const members: RoomPlayer[] = [...room.members.values()].map((m) => ({
      userId: m.client.user.id, name: m.client.user.name, variant: m.variant, manual: m.manual,
      ready: m.ready || m.client.user.id === room.hostId, host: m.client.user.id === room.hostId,
    }));
    const state: RoomState = { ...this.summary(room), members };
    for (const member of room.members.values()) send(member.client, { t: 'room', room: state });
    this.broadcastRooms();
  }

  private broadcastRooms(): void {
    const message: ServerMessage = { t: 'rooms', rooms: this.summaries() };
    for (const client of this.clients) send(client, message);
  }
}
