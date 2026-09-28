// Messages between the browser and the multiplayer server (web/server).
// Control traffic is JSON text frames; the race itself uses binary frames:
//   client -> server  [BINARY_INPUT]  one or more inputs, oldest first, each
//                     u32 sequence number, u32 the race tick the client used
//                     it for, 8 int32 input words (the data channel repeats
//                     the last few, so a lost packet loses nothing)
//   server -> client  [BINARY_FRAME]  u32 server tick, u8 player count, per
//                     player (seat order) u32 last input sequence applied, u32
//                     the tick it was applied at and i32 how many ticks early
//                     the latest input arrived (negative: late), then the
//                     RaceFrame wire bytes (all little-endian)
// The server applies each input at the tick the client predicted it for, so
// a player's own car needs no correction when inputs arrive in time; the
// clients steer their clocks by the reported margin.
// The race can also run over a WebRTC data channel (unordered, unreliable)
// signalled with rtcOffer / rtcAnswer / rtcCandidate: the same inputs, and
// frames compressed with raw deflate. Frames may then arrive out of order or
// not at all; clients drop any older than the newest they have. The client
// pings the channel [BINARY_PING, f64 time] and drops it when the echoes stop.
// Types only: shared by the Vite client and the Node server (type stripping).

export const DEFAULT_PORT = 7243;
export const BINARY_INPUT = 1;
export const BINARY_FRAME = 2;
/* Data channel heartbeat: the client sends it, the server echoes it back. */
export const BINARY_PING = 3;
export const INPUT_WORDS = 8;
export const ACK_WORDS = 3; // per player in a frame: sequence, applied tick, margin
export const INPUT_BYTES = 8 + INPUT_WORDS * 4; // one input in a BINARY_INPUT packet
export const INPUT_REPEAT = 4; // inputs per data channel packet

export interface UserInfo { id: number; name: string; admin: boolean }

export interface RoomSettings {
  name: string;
  classIndex: number; // 0..5
  course: number; // 0..3
  reverse: boolean;
  laps: number; // 1..6
  rivals: boolean; // fill the remaining grid with the retail AI
  maxPlayers: number; // 1..the course's authored starts
}

export type RoomStatus = 'lobby' | 'loading' | 'racing';

export interface RoomPlayer {
  userId: number;
  name: string;
  variant: number; // -1 until chosen
  manual: boolean;
  ready: boolean;
  host: boolean;
  spectator: boolean; // watches instead of racing
  online: boolean; // false while reconnecting (the place is kept for a while)
}

export interface RoomSummary {
  id: number;
  settings: RoomSettings;
  status: RoomStatus;
  host: string;
  players: number; // racers
  spectators: number;
}

export interface RoomState extends RoomSummary { members: RoomPlayer[] }

/** Two colours from the game's paint catalogue (PAINT_COLORS entries): the
 *  first and second body colour. Presentation only; the race never sees it. */
export type Paint = [number, number];
export const PAINT_COLORS = 18; // RAGE_CAR_PAINT_COLOR_COUNT

/** A player's saved paints by car model (not variant: a car keeps its paint
 *  through its upgrades). Missing: the car's factory colours. */
export type Garage = Record<number, Paint>;

// userId null: a rival. paint null: the factory colours.
export interface RaceSeat { userId: number | null; name: string; variant: number; manual: boolean; paint: Paint | null }

export interface RaceResult {
  seat: number;
  userId: number | null;
  name: string;
  variant: number; // human variant, or -1 for AI
  place: number; // 0 when the car did not finish
  timeMs: number; // -1 when it did not finish
  bestLapMs: number; // -1 without a completed lap
  status: 'finished' | 'retired' | 'dnf';
}

export interface RecordRow {
  course: number;
  reverse: boolean;
  classIndex: number;
  name: string;
  variant: number;
  bestLapMs: number;
}

// GET /api/history: the player's recent races, newest first.
export interface HistoryRow {
  course: number;
  reverse: boolean;
  classIndex: number;
  place: number;
  entrants: number;
  timeMs: number;
  bestLapMs: number;
  status: RaceResult['status'];
}

export type ClientMessage =
  | { t: 'createRoom'; settings: RoomSettings }
  | { t: 'updateRoom'; settings: RoomSettings }
  | { t: 'joinRoom'; roomId: number }
  | { t: 'leaveRoom' }
  | { t: 'setCar'; variant: number; manual: boolean }
  | { t: 'setReady'; ready: boolean }
  | { t: 'setSpectator'; spectator: boolean }
  | { t: 'watchRace' }
  | { t: 'startRace' }
  | { t: 'loaded'; ok: boolean }
  | { t: 'leaveRace' }
  | { t: 'chat'; text: string }
  | { t: 'closeRoom'; roomId: number }
  | { t: 'rtcOffer'; sdp: string }
  | { t: 'rtcCandidate'; candidate: string; mid: string };

export type RaceEvent =
  | { kind: 'lap'; name: string; lap: number; lapMs: number }
  | { kind: 'finish'; name: string; place: number; timeMs: number }
  | { kind: 'retire'; name: string; reason: string };

export type ServerMessage =
  | { t: 'welcome'; user: UserInfo; discId: string }
  | { t: 'rooms'; rooms: RoomSummary[] }
  | { t: 'room'; room: RoomState | null }
  | { t: 'chat'; from: string; text: string; at: number }
  // localSeat is -1 for a spectator.
  | { t: 'raceStart'; settings: RoomSettings; seats: RaceSeat[]; humans: number; localSeat: number }
  // The rest of the field has this long to finish once the first car is home.
  | { t: 'finishDeadline'; remainingMs: number }
  | { t: 'raceGo' }
  // Every couple of seconds: each room member's round trip (null offline).
  | { t: 'latency'; latency: Record<number, number | null> }
  | { t: 'raceEvent'; event: RaceEvent }
  | { t: 'results'; results: RaceResult[] }
  | { t: 'error'; message: string }
  | { t: 'rtcAnswer'; sdp: string }
  | { t: 'rtcCandidate'; candidate: string; mid: string };
