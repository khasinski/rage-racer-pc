// A separate public mode: one persistent community, reserved driver seats
// driven by the C autopilot until a loaded human takes over at an exact tick.
import { deflateRawSync } from 'node:zlib';
import { BINARY_ARCADE_FRAME,BINARY_INPUT, INPUT_BYTES, INPUT_REPEAT, INPUT_WORDS,
  type RaceSeat, type RoomSettings, type ServerMessage, type RaceResult } from '../shared/protocol.ts';
import { send, type Client } from './room.ts';
import { RaceClock } from './race-clock.ts';
import { PHASE_FINISHED, STATUS_DRIVING, STATUS_FINISHED, type Simulation } from './sim.ts';

interface Pending { sequence: number; tick: number; words: Int32Array }
interface Member {
  client: Client; seat: number; loaded: boolean; active: boolean; activateAt: number;
  offlineUntil: number; joinedLate: boolean; joinedLap: number; nextCar: number;
  vote: number; chatAt: number; loadedAt: number;
}
const ROTATION = [[0, 0], [0, 1], [1, 2], [2, 3]] as const;
const INTERMISSION_MS = 15_000, RECONNECT_MS = 15_000, TAKEOVER_TICKS = 150;
const MAX_ROUND_TICKS = 50 * 240;
export class ArcadeSession {
  private readonly now: () => number;
  private readonly members = new Map<number, Member>();
  private readonly clock = new RaceClock(() => this.tick(this.now()));
  private readonly history: { from: string; text: string; at: number }[] = [];
  private handle = 0;
  private epoch = 0;
  private rotation = 0;
  private slots = 0;
  private seats: RaceSeat[] = [];
  private settings!: RoomSettings;
  private acks = new Uint32Array(0);
  private pending: Pending[][] = [];
  private intermissionUntil = 0;
  private finishAt = 0;
  private stateAt = 0;

  private readonly sim: Simulation;
  constructor(sim: Simulation, now: () => number = Date.now) { this.sim=sim;this.now=now; }
  has(id: number): boolean { return this.members.has(id); }
  join(client: Client): void {
    const previous = this.members.get(client.user.id);
    if (!previous && this.members.size >= 24) return send(client, { t: 'error', message: 'Online Arcade is full. Please try again shortly.' });
    client.arcade = true;
    const m: Member = previous ?? { client, seat: -1, loaded: false, active: false, activateAt: 0,
      offlineUntil: 0, joinedLate: false, joinedLap: 0, nextCar: -1, vote: -1, chatAt: 0, loadedAt: 0 };
    m.client = client; m.offlineUntil = 0; m.loaded = m.active = false; m.activateAt = 0;
    if(this.handle&&m.seat>=0)this.sim.autopilot(this.handle,m.seat,true);
    this.members.set(client.user.id, m);
    send(client, { t: 'arcadeChat', lines: this.history });
    if (!this.handle && !this.intermissionUntil) this.createRound();
    else if (this.handle) { if (m.seat < 0) m.seat = this.pickSeat(); this.load(m); }
    this.chat('', `${client.user.name} joined Online Arcade.`);
    this.state(); this.clock.ensureRunning();
  }
  disconnect(client: Client, now = this.now()): void {
    const m = this.members.get(client.user.id); if (!m || m.client !== client) return;
    if (this.handle && m.seat >= 0) this.sim.autopilot(this.handle, m.seat, true);
    m.active = m.loaded = false; m.activateAt = 0; m.offlineUntil = now + RECONNECT_MS;
    this.state();
  }
  leave(client: Client): void {
    const m = this.members.get(client.user.id); client.arcade = false;
    if (!m || m.client !== client) return;
    if (this.handle && m.seat >= 0) this.sim.autopilot(this.handle, m.seat, true);
    this.members.delete(client.user.id); this.chat('', `${client.user.name} left; CPU took over.`);
    send(client,{t:'arcadeLeft'});
    this.state(); this.fillWaiting(); if (!this.members.size) this.sleep();
  }
  loaded(client: Client, epoch: number, ok: boolean): void {
    const m = this.members.get(client.user.id);
    if (!m || m.client !== client || epoch !== this.epoch || !this.handle || m.loaded) return;
    if (!ok) { this.leave(client); return; }
    m.loaded = true;
    this.schedule(m);
  }
  chooseCar(client: Client, variant: number): void {
    const m = this.members.get(client.user.id);
    if (m && Number.isInteger(variant) && variant >= 0 && variant < 32) { m.nextCar = variant; send(client, { t: 'chat', from: '', text: 'Car selection saved for the next race (subject to its class).', at: this.now() }); }
  }
  vote(client: Client, course: number): void {
    const m = this.members.get(client.user.id);
    if (m && Number.isInteger(course) && course >= 0 && course < 4) { m.vote = course; this.state(); }
  }
  message(client: Client, text: string): void {
    const m = this.members.get(client.user.id), now = this.now();
    if (!m || typeof text !== 'string' || now - m.chatAt < 500) return;
    const clean = text.trim().replace(/[\x00-\x1f\x7f]/g, ' ').slice(0, 200); if (!clean) return;
    m.chatAt = now; this.chat(client.user.name, clean);
  }
  private chat(from: string, text: string): void {
    const line = { from, text, at: this.now() }; this.history.push(line);
    if (this.history.length > 64) this.history.shift();
    this.broadcast({ t: 'chat', ...line });
  }
  private broadcast(message: ServerMessage): void {
    const json = JSON.stringify(message);
    for (const m of this.members.values()) if (!m.offlineUntil) send(m.client, json);
  }
  private nextCourse(): number {
    const classIndex = ROTATION[this.rotation % ROTATION.length][0];
    const counts = [0, 0, 0, 0]; for (const m of this.members.values()) if (!m.offlineUntil && m.vote >= 0 && this.sim.courseAllowed(classIndex, m.vote)) counts[m.vote]++;
    let course = ROTATION[this.rotation % ROTATION.length][1] as number;
    for (let c = 0; c < 4; c++) if (counts[c] > counts[course]) course = c;
    return course;
  }
  private state(): void {
    this.broadcast({ t: 'arcadeState', epoch: this.epoch, slots: this.slots,
      members: [...this.members.values()].map(m => ({ userId: m.client.user.id, name: m.client.user.name, seat: m.seat, online: !m.offlineUntil })),
      nextInMs: this.intermissionUntil ? Math.max(0, this.intermissionUntil - this.now()) : 0, nextCourse: this.nextCourse(), nextClass: ROTATION[this.rotation % ROTATION.length][0] });
  }
  private createRound(): void {
    const classIndex = ROTATION[this.rotation % ROTATION.length][0], course = this.nextCourse();
    this.settings = { name: 'Online Arcade', classIndex, course, reverse: false, laps: 2, rivals: true,
      maxPlayers: Math.min(8, this.sim.maxHumans(classIndex, course, false)) };
    const first = Array.from({ length: 13 }, (_, model) => this.sim.classCar(classIndex, model)).find(v => v >= 0);
    if (first === undefined || this.settings.maxPlayers < 1) throw new Error('No valid Arcade field');
    this.slots = this.settings.maxPlayers;
    const online = [...this.members.values()].filter(m => !m.offlineUntil);
    this.seats = Array.from({ length: this.slots }, (_, seat) => {
      const m = online[seat], choice = m?.nextCar ?? -1;
      const variant = this.sim.carAllowed(classIndex, choice) ? choice : first;
      return { userId: null, name: `CPU ${seat + 1}`, variant, manual: !this.sim.carAutomatic(variant), tire: 0, paint: null, logo: null };
    });
    this.handle = this.sim.createRace(classIndex, course, false, 2, true, this.seats);
    if (!this.handle) throw new Error('Arcade field could not be prepared');
    for (let seat = 0; seat < this.slots; seat++) this.sim.autopilot(this.handle, seat, true);
    if (!this.sim.start(this.handle)) throw new Error('Arcade race could not start');
    this.epoch++; this.acks = new Uint32Array(this.slots * 3); this.pending = this.seats.map(() => []);
    this.finishAt = this.intermissionUntil = 0;
    let seat = 0;
    for (const m of this.members.values()) {
      m.seat = m.offlineUntil || seat >= this.slots ? -1 : seat++;
      m.loaded = m.active = m.joinedLate = false; m.activateAt = 0; m.vote = -1;
      if (!m.offlineUntil) this.load(m);
    }
    this.rotation++; this.state();
  }
  private load(m: Member): void {
    m.loadedAt = this.now();
    send(m.client, { t: 'raceStart', settings: this.settings, seats: this.seats, humans: this.slots, localSeat: -1,
      arcade: true, epoch: this.epoch, viewSeat: Math.max(0, m.seat) });
    send(m.client, { t: 'raceGo' });
  }
  private pickSeat(): number {
    const occupied = new Set([...this.members.values()].filter(m => m.seat >= 0).map(m => m.seat));
    const humans = [...this.members.values()].filter(m => m.active && !m.offlineUntil);
    const target = humans.length ? humans.reduce((sum, m) => sum + this.sim.seat(this.handle, m.seat).place, 0) / humans.length : 12;
    let picked = -1, distance = Infinity;
    let moving=false;
    for(let seat=0;seat<this.slots;seat++){const s=this.sim.seat(this.handle,seat);if(!occupied.has(seat)&&s.status===STATUS_DRIVING&&s.speed>=80)moving=true;}
    for (let seat = 0; seat < this.slots; seat++) {
      const state = this.sim.seat(this.handle, seat);
      if (occupied.has(seat) || state.status !== STATUS_DRIVING || (moving&&state.speed<80)) continue;
      const d = Math.abs(state.place - target); if (d < distance) { distance = d; picked = seat; }
    }
    return picked;
  }
  private schedule(m: Member): void {
    if (!this.handle || !m.loaded || m.active || m.activateAt || m.offlineUntil) return;
    // Very late arrivals stay with this community for the next common start.
    if (this.finishAt && this.finishAt - this.now() <= 20_000) return;
    if (m.seat < 0) m.seat = this.pickSeat(); if (m.seat < 0) return;
    const tick = this.sim.simTick(this.handle);
    m.activateAt = tick < TAKEOVER_TICKS ? TAKEOVER_TICKS : tick + TAKEOVER_TICKS;
    m.joinedLate = tick >= TAKEOVER_TICKS;
    send(m.client, { t: 'arcadeControl', epoch: this.epoch, seat: m.seat, activateAt: m.activateAt, late: m.joinedLate });
  }
  private fillWaiting(): void { for (const m of this.members.values()) this.schedule(m); }
  input(client: Client, data: Buffer): void {
    const m = this.members.get(client.user.id);
    if (!m || m.client !== client || !m.active || !this.handle || m.offlineUntil || data[0] !== BINARY_INPUT) return;
    const count = (data.length - 1) / INPUT_BYTES;
    if (!Number.isInteger(count) || count < 1 || count > INPUT_REPEAT) return;
    const seat = m.seat, queue = this.pending[seat], next = this.sim.simTick(this.handle) + 1;
    for (let at = 1; at < data.length; at += INPUT_BYTES) {
      const sequence = data.readUInt32LE(at), newest = queue.at(-1)?.sequence ?? this.acks[seat * 3];
      if (sequence <= newest || queue.length >= 64) continue;
      const tick = data.readUInt32LE(at + 4), words = new Int32Array(INPUT_WORDS);
      for (let i = 0; i < INPUT_WORDS; i++) words[i] = data.readInt32LE(at + 8 + i * 4);
      queue.push({ sequence, tick: Math.min(Math.max(tick, next), next + 25), words });
      this.acks[seat * 3 + 2] = (tick - next) >>> 0;
    }
  }
  private publishFrame(tick: number,reliable=false): void {
    const frame = this.sim.frame(this.handle); if (!frame) throw new Error('Invalid Arcade checkpoint');
    const header = 10 + this.acks.length * 4, packet = Buffer.allocUnsafe(header + frame.length);
    packet[0] = BINARY_ARCADE_FRAME; packet.writeUInt32LE(this.epoch, 1); packet.writeUInt32LE(tick, 5); packet[9] = this.slots;
    this.acks.forEach((v, i) => packet.writeUInt32LE(v, 10 + i * 4)); packet.set(frame, header);
    let compressed: Buffer | undefined;
    for (const m of this.members.values()) if (!m.offlineUntil && m.loaded) {
      if (!reliable && m.client.rtc?.open && m.client.rtc.send(compressed ??= deflateRawSync(packet, { level: 1 }))) continue;
      if (m.client.ws.readyState === m.client.ws.OPEN && (reliable||m.client.ws.bufferedAmount < 65536)) m.client.ws.send(packet);
    }
  }
  tick(now: number): boolean {
    for (const m of [...this.members.values()]) {
      if (m.offlineUntil && now >= m.offlineUntil) this.members.delete(m.client.user.id);
      if (!m.loaded && !m.offlineUntil && this.handle && now - m.loadedAt > 30_000) this.leave(m.client);
    }
    if (!this.members.size) { this.sleep(); return false; }
    if (this.intermissionUntil) { if (now >= this.intermissionUntil) this.createRound(); }
    else if (this.handle) {
      const next = this.sim.simTick(this.handle) + 1;
      for (const m of this.members.values()) if (m.loaded && !m.offlineUntil && m.activateAt && next >= m.activateAt && !m.active) {
        this.sim.autopilot(this.handle, m.seat, false); m.active = true; m.joinedLap = this.sim.seat(this.handle, m.seat).lap;
        this.pending[m.seat] = []; this.acks.fill(0, m.seat * 3, m.seat * 3 + 3);
        this.chat('', `${m.client.user.name} took control of CPU ${m.seat + 1}.`);
      }
      for (let seat = 0; seat < this.slots; seat++) {
        const queue = this.pending[seat];
        while (queue.length && queue[0].tick <= next) {
          const input = queue.shift()!; if (!this.sim.setInput(this.handle, seat, input.words)) continue;
          this.acks[seat * 3] = input.sequence; this.acks[seat * 3 + 1] = next;
        }
      }
      const phase = this.sim.tick(this.handle), tick = this.sim.simTick(this.handle);
      if (!(tick % 2)) this.publishFrame(tick);
      let finished = false, anyActive = false, allDone = true;
      for (let seat = 0; seat < 12; seat++) if (this.sim.seat(this.handle, seat).status === STATUS_FINISHED) finished = true;
      for (const m of this.members.values()) if (m.active && !m.offlineUntil) { anyActive = true; if (this.sim.seat(this.handle, m.seat).status === STATUS_DRIVING) allDone = false; }
      if (finished && !this.finishAt) this.finishAt = now + 30_000;
      if (phase === PHASE_FINISHED || tick >= MAX_ROUND_TICKS || (this.finishAt && now >= this.finishAt) || (anyActive && allDone)) this.finish(now);
      else this.fillWaiting();
    }
    if (now >= this.stateAt) { this.stateAt = now + 1000; this.state(); }
    return true;
  }
  private finish(now: number): void {
    // The classified final state must arrive on the same ordered channel as
    // results, even when the last unreliable race packet was lost.
    this.publishFrame(this.sim.simTick(this.handle),true);
    const results: RaceResult[] = [];
    for (let seat = 0; seat < 12; seat++) {
      const state = this.sim.seat(this.handle, seat); if (state.status < STATUS_DRIVING) continue;
      const m = [...this.members.values()].find(p => p.seat === seat && p.active);
      const finished = state.status === STATUS_FINISHED;
      let bestLapMs = -1;
      if(m)for(let lap=Math.max(0,m.joinedLap);lap<state.lap-1;lap++){
        const ms=this.sim.lapTime(this.handle,seat,lap);
        if(ms>=0&&(bestLapMs<0||ms<bestLapMs))bestLapMs=ms;
      }
      results.push({ seat, userId: m?.client.user.id ?? null, name: m?.client.user.name ?? `CPU ${seat + 1}`,
        variant: this.seats[seat]?.variant ?? -1, place: state.place, timeMs: m?.joinedLate ? -1 : finished ? state.timeMs : -1,
        bestLapMs, joinedLate:m?.joinedLate??false,status: finished ? 'finished' : 'dnf' });
    }
    results.sort((a, b) => (a.place || 99) - (b.place || 99));
    this.broadcast({ t: 'results', results });
    this.sim.free(this.handle); this.handle = 0; this.intermissionUntil = now + INTERMISSION_MS;
    for (const m of this.members.values()) { m.active = false; m.activateAt = 0; }
    this.chat('', 'Next race starts in 15 seconds. Vote for a course in the Arcade lobby.'); this.state();
  }
  private sleep(): void {
    if (this.handle) this.sim.free(this.handle);
    this.handle = this.intermissionUntil = 0; this.slots = 0;
  }
}
