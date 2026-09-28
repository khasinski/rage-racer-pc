// Messages between the browser and the multiplayer server (web/server).
// Control traffic is JSON text frames; the race itself uses binary frames:
//   client -> server  [BINARY_INPUT]  u32 sequence number, 8 int32 input words
//   server -> client  [BINARY_FRAME]  u32 server tick, u8 player count, per
//                     player (seat order) u32 last input sequence received and
//                     u32 the server tick that first used it, then the
//                     RaceFrame wire bytes (all little-endian)
// The acknowledgements let each player's client predict its own car.
// Types only: shared by the Vite client and the Node server (type stripping).

export const DEFAULT_PORT = 7243;
export const BINARY_INPUT = 1;
export const BINARY_FRAME = 2;
export const INPUT_WORDS = 8;

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

export type RoomStatus = 'lobby' | 'loading' | 'racing' | 'results';

export interface RoomPlayer {
  userId: number;
  name: string;
  variant: number; // -1 until chosen
  manual: boolean;
  ready: boolean;
  host: boolean;
  spectator: boolean; // watches instead of racing
  online: boolean; // false while reconnecting (the place is kept for a while)
  latencyMs: number | null; // round trip to the server
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

export interface RaceSeat { userId: number | null; name: string; variant: number; manual: boolean; ai: boolean }

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
  raceMs: number | null;
  at: string;
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
  | { t: 'closeRoom'; roomId: number };

export type RaceEvent =
  | { kind: 'lap'; seat: number; name: string; lap: number; lapMs: number }
  | { kind: 'finish'; seat: number; name: string; place: number; timeMs: number }
  | { kind: 'retire'; seat: number; name: string; reason: string };

export type ServerMessage =
  | { t: 'welcome'; user: UserInfo; discId: string }
  | { t: 'rooms'; rooms: RoomSummary[] }
  | { t: 'room'; room: RoomState | null }
  | { t: 'chat'; roomId: number; from: string; text: string; at: number }
  // localSeat is -1 for a spectator.
  | { t: 'raceStart'; raceId: number; settings: RoomSettings; seats: RaceSeat[]; humans: number; localSeat: number }
  // The rest of the field has this long to finish once the first car is home.
  | { t: 'finishDeadline'; remainingMs: number }
  | { t: 'raceGo' }
  // Every couple of seconds: each room member's round trip (null offline).
  | { t: 'latency'; latency: Record<number, number | null> }
  | { t: 'raceEvent'; event: RaceEvent }
  | { t: 'results'; raceId: number; results: RaceResult[] }
  | { t: 'error'; message: string };
