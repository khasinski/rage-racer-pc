// One authoritative race: the server builds the field, waits for the players
// to load it, steps it at 50 Hz, applies their inputs, streams the complete
// RaceFrame to everyone watching, reports race events and stores the results.
import type { WebSocket } from 'ws';
import {
  BINARY_FRAME, BINARY_INPUT, INPUT_WORDS,
  type RaceEvent, type RaceResult, type RaceSeat, type RoomSettings, type ServerMessage,
} from '../shared/protocol.ts';
import type { Store } from './db.ts';
import {
  PHASE_FINISHED, STATUS_DRIVING, STATUS_EMPTY, STATUS_FINISHED, STATUS_RETIRED, type Simulation,
} from './sim.ts';

const LOAD_TIMEOUT_MS = Number(process.env.RAGE_LOAD_TIMEOUT_MS ?? 30_000);
/* After the winner (player or rival) crosses the line, the rest of the field
 * has this long: at least 90 s, or 30% of the winning time on long races. */
const FINISH_GRACE_MS = Number(process.env.RAGE_FINISH_GRACE_MS ?? 90_000);
const FINISH_GRACE_SHARE = 0.3;
const END_DELAY_MS = 2_500; // lets the last finisher see the line before results
/* A client this far behind on frames skips some instead of queueing more. */
const BACKLOG_BYTES = 256 * 1024;
const NEUTRAL_INPUT = new Int32Array([1, 0, 0, 0, 0, 0, 0, 0]);

export interface Racer { userId: number; name: string; variant: number; manual: boolean }

/** What a race needs from its room. */
export interface RaceRoom {
  /** Sends a message to every member of the room. */
  broadcast(message: ServerMessage): void;
  /** A member's connection while they are online. */
  socket(userId: number): WebSocket | undefined;
}

export class Race {
  readonly settings: RoomSettings;
  private readonly sim: Simulation;
  private readonly store: Store;
  private readonly room: RaceRoom;
  private readonly handle: number;
  private readonly raceId: number;
  private readonly seats: RaceSeat[]; // players first, in seat order, then the rivals
  private readonly seatOf: Map<number, number>; // userId -> seat
  private readonly acks: Uint32Array; // per player seat: last input sequence, then the tick that first used it
  private readonly viewers = new Set<number>(); // everyone receiving the race (players and spectators)
  private readonly awaiting: Set<number>; // players the start waits for, until loadDeadline
  private readonly retiring = new Set<number>(); // seats given up before the start, retired at it
  private readonly loadDeadline = Date.now() + LOAD_TIMEOUT_MS;
  private readonly laps: number[];
  private readonly status: number[];
  private finishDeadline: number | null = null;
  private endAt: number | null = null;
  started = false;

  private constructor(sim: Simulation, store: Store, room: RaceRoom, roomId: number, settings: RoomSettings,
                      handle: number, racers: Racer[]) {
    this.sim = sim;
    this.store = store;
    this.room = room;
    this.settings = settings;
    this.handle = handle;
    this.seats = [];
    for (let seat = 0; seat < 12; seat++) {
      if (sim.seat(handle, seat).status === STATUS_EMPTY) continue;
      const racer = racers[seat];
      this.seats.push(racer
        ? { userId: racer.userId, name: racer.name, variant: racer.variant, manual: racer.manual }
        : { userId: null, name: `CPU ${seat - racers.length + 1}`, variant: -1, manual: false });
    }
    this.seatOf = new Map(racers.map((racer, seat) => [racer.userId, seat]));
    this.acks = new Uint32Array(racers.length * 2);
    this.awaiting = new Set(this.seatOf.keys());
    this.laps = this.seats.map(() => 0);
    this.status = this.seats.map(() => STATUS_DRIVING);
    this.raceId = store.createRace(roomId, settings);
  }

  /** Builds the field with the players in seat order; null when the
   *  simulation refuses it. */
  static create(sim: Simulation, store: Store, room: RaceRoom, roomId: number, settings: RoomSettings,
                racers: Racer[]): Race | null {
    const s = settings;
    const handle = sim.createRace(s.classIndex, s.course, s.reverse, s.laps, s.rivals, racers);
    return handle ? new Race(sim, store, room, roomId, settings, handle, racers) : null;
  }

  /** A player in the race, or a member receiving it. */
  involves(userId: number): boolean {
    return this.seatOf.has(userId) || this.viewers.has(userId);
  }

  /** A player whose car is in the field (driving, finished or retired). */
  seated(userId: number): boolean {
    return this.seatOf.has(userId);
  }

  /** Starts sending the race to a member; returns what they need to join it.
   *  A player back before the start reloads it, so the start waits again. */
  watch(userId: number): ServerMessage[] {
    this.viewers.add(userId);
    const seat = this.seatOf.get(userId);
    if (!this.started && seat !== undefined && !this.retiring.has(seat)) this.awaiting.add(userId);
    const messages: ServerMessage[] = [{ t: 'raceStart', settings: this.settings, seats: this.seats,
      humans: this.seatOf.size, localSeat: seat ?? -1 }];
    if (this.started) messages.push({ t: 'raceGo' });
    if (this.finishDeadline !== null) messages.push({ t: 'finishDeadline', remainingMs: Math.max(0, this.finishDeadline - Date.now()) });
    return messages;
  }

  /** Whether the start still waits for this player to load. */
  awaits(userId: number): boolean {
    return this.awaiting.has(userId);
  }

  loaded(userId: number): void {
    this.awaiting.delete(userId);
  }

  /** Stops sending the race to a member and retires their car. */
  leave(userId: number, reason: string): void {
    this.viewers.delete(userId);
    this.retire(userId, reason);
  }

  /** Takes a player's car out of the race: at once while racing, at the
   *  start before it. A car that finished or already retired stays as it is. */
  private retire(userId: number, reason: string): void {
    const seat = this.seatOf.get(userId);
    if (seat === undefined) return;
    if (this.started) {
      if (this.sim.seat(this.handle, seat).status !== STATUS_DRIVING) return;
      this.sim.retire(this.handle, seat);
    } else {
      if (this.retiring.has(seat)) return;
      this.retiring.add(seat);
      this.awaiting.delete(userId);
    }
    this.event({ kind: 'retire', name: this.seats[seat].name, reason });
  }

  /** The car of a player who lost the connection coasts. */
  coast(userId: number): void {
    const seat = this.seatOf.get(userId);
    if (seat !== undefined) this.sim.setInput(this.handle, seat, NEUTRAL_INPUT);
  }

  input(userId: number, data: Buffer): void {
    const seat = this.seatOf.get(userId);
    if (seat === undefined || data.length !== 5 + INPUT_WORDS * 4 || data[0] !== BINARY_INPUT) return;
    const sequence = data.readUInt32LE(1);
    if (sequence <= this.acks[seat * 2]) return; // late duplicate
    const words = new Int32Array(INPUT_WORDS);
    for (let i = 0; i < INPUT_WORDS; i++) words[i] = data.readInt32LE(5 + i * 4);
    if (!this.sim.setInput(this.handle, seat, words)) return;
    this.acks[seat * 2] = sequence;
    this.acks[seat * 2 + 1] = this.sim.simTick(this.handle) + 1; // used from the next tick
  }

  /** Players still loading once the load timeout has passed. */
  overdue(now: number): number[] {
    return !this.started && now >= this.loadDeadline ? [...this.awaiting] : [];
  }

  /** One 50 Hz step: 'started' when the countdown began, 'over' once the
   *  results are out (or the race could not start), else 'running'. */
  step(now: number): 'running' | 'started' | 'over' {
    let outcome: 'running' | 'started' = 'running';
    if (!this.started) {
      if (this.awaiting.size && now < this.loadDeadline) return 'running';
      if (!this.sim.start(this.handle)) {
        this.room.broadcast({ t: 'error', message: 'The race could not start.' });
        this.free();
        return 'over';
      }
      this.started = true;
      for (const seat of this.retiring) this.sim.retire(this.handle, seat);
      this.room.broadcast({ t: 'raceGo' });
      outcome = 'started';
    }
    const phase = this.sim.tick(this.handle);
    const serverTick = this.sim.simTick(this.handle);
    // The field moves every second tick (25 Hz); publish each such state.
    if (serverTick % 2 === 0 || phase === PHASE_FINISHED) this.publishFrame(serverTick);
    if (serverTick % 2 === 0) this.detectEvents(now);
    if (phase === PHASE_FINISHED && this.endAt === null) this.endAt = now + END_DELAY_MS;
    if (this.endAt === null || now < this.endAt) return outcome;
    this.finish();
    return 'over';
  }

  free(): void {
    this.sim.free(this.handle);
  }

  private publishFrame(serverTick: number): void {
    const frame = this.sim.frame(this.handle);
    if (!frame) return;
    const header = 6 + this.acks.length * 4;
    const packet = Buffer.allocUnsafe(header + frame.length);
    packet[0] = BINARY_FRAME;
    packet.writeUInt32LE(serverTick >>> 0, 1);
    packet[5] = this.seatOf.size;
    this.acks.forEach((value, i) => packet.writeUInt32LE(value, 6 + i * 4));
    packet.set(frame, header);
    for (const id of this.viewers) {
      const ws = this.room.socket(id);
      // A slow link skips frames rather than falling ever further behind.
      if (ws && ws.readyState === ws.OPEN && ws.bufferedAmount < BACKLOG_BYTES) ws.send(packet);
    }
  }

  private detectEvents(now: number): void {
    let playersDone = true;
    this.seats.forEach((seat, index) => {
      const state = this.sim.seat(this.handle, index);
      const player = seat.userId !== null;
      if (player && state.lap > this.laps[index] && this.laps[index] >= 1) {
        const lapMs = this.sim.lapTime(this.handle, index, this.laps[index] - 1);
        if (lapMs >= 0 && state.status === STATUS_DRIVING) this.event({ kind: 'lap', name: seat.name, lap: this.laps[index], lapMs });
      }
      this.laps[index] = state.lap;
      if (state.status === STATUS_FINISHED && this.status[index] !== STATUS_FINISHED) {
        this.event({ kind: 'finish', name: seat.name, place: state.place, timeMs: state.timeMs });
        if (this.finishDeadline === null) {
          const grace = Math.max(FINISH_GRACE_MS, state.timeMs * FINISH_GRACE_SHARE);
          this.finishDeadline = now + grace;
          this.room.broadcast({ t: 'finishDeadline', remainingMs: grace });
        }
      }
      this.status[index] = state.status;
      if (player && state.status === STATUS_DRIVING) playersDone = false;
    });
    if (playersDone && this.endAt === null) this.endAt = now + END_DELAY_MS;
    if (this.finishDeadline !== null && this.endAt === null && now >= this.finishDeadline) this.endAt = now;
  }

  private event(event: RaceEvent): void {
    this.room.broadcast({ t: 'raceEvent', event });
  }

  /** Stores and announces the results, and frees the race. */
  private finish(): void {
    const results: RaceResult[] = this.seats.map((seat, index) => {
      const state = this.sim.seat(this.handle, index);
      const finished = state.status === STATUS_FINISHED;
      return {
        seat: index, userId: seat.userId, name: seat.name, variant: seat.variant,
        // Cars still running when the race closes keep their running position.
        place: state.status === STATUS_RETIRED ? 0 : state.place,
        timeMs: finished ? state.timeMs : -1, bestLapMs: state.bestLapMs,
        status: finished ? 'finished' : state.status === STATUS_RETIRED ? 'retired' : 'dnf',
      };
    });
    results.sort((a, b) => (a.place || 99) - (b.place || 99) || a.seat - b.seat);
    this.store.finishRace(this.raceId, results);
    this.free();
    this.room.broadcast({ t: 'results', results });
  }
}
