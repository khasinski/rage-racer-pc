// Messages between the browser and the multiplayer server (web/server).
// Control traffic is JSON text frames; the race itself uses binary frames:
//   client -> server  [BINARY_INPUT]  then 8 little-endian int32 input words
//   server -> client  [BINARY_FRAME]  u32 server tick, then the RaceFrame wire bytes
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
}

export interface RoomSummary {
  id: number;
  settings: RoomSettings;
  status: RoomStatus;
  host: string;
  players: number;
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
  | { t: 'raceStart'; raceId: number; settings: RoomSettings; seats: RaceSeat[]; humans: number; localSeat: number }
  | { t: 'raceGo' }
  | { t: 'raceEvent'; event: RaceEvent }
  | { t: 'results'; raceId: number; results: RaceResult[] }
  | { t: 'error'; message: string };
