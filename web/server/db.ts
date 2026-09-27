// SQLite storage (node:sqlite): accounts, sessions, rooms, races, results.
import { randomBytes, scryptSync, timingSafeEqual } from 'node:crypto';
import { mkdirSync } from 'node:fs';
import { dirname } from 'node:path';
import { DatabaseSync } from 'node:sqlite';
import type { RaceResult, RecordRow, RoomSettings, UserInfo } from '../shared/protocol.ts';

const SESSION_DAYS = 30;

const SCHEMA = `
PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;
CREATE TABLE IF NOT EXISTS users (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE COLLATE NOCASE,
  password_hash TEXT NOT NULL,
  password_salt TEXT NOT NULL,
  admin INTEGER NOT NULL DEFAULT 0,
  created_at TEXT NOT NULL DEFAULT (datetime('now'))
);
CREATE TABLE IF NOT EXISTS sessions (
  token TEXT PRIMARY KEY,
  user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  expires_at TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS rooms (
  id INTEGER PRIMARY KEY,
  name TEXT NOT NULL,
  host_id INTEGER NOT NULL REFERENCES users(id),
  class_index INTEGER NOT NULL,
  course INTEGER NOT NULL,
  reverse INTEGER NOT NULL,
  laps INTEGER NOT NULL,
  rivals INTEGER NOT NULL,
  max_players INTEGER NOT NULL,
  created_at TEXT NOT NULL DEFAULT (datetime('now')),
  closed_at TEXT
);
CREATE TABLE IF NOT EXISTS races (
  id INTEGER PRIMARY KEY,
  room_id INTEGER NOT NULL REFERENCES rooms(id),
  class_index INTEGER NOT NULL,
  course INTEGER NOT NULL,
  reverse INTEGER NOT NULL,
  laps INTEGER NOT NULL,
  started_at TEXT NOT NULL DEFAULT (datetime('now')),
  finished_at TEXT
);
CREATE TABLE IF NOT EXISTS race_results (
  race_id INTEGER NOT NULL REFERENCES races(id) ON DELETE CASCADE,
  seat INTEGER NOT NULL,
  user_id INTEGER REFERENCES users(id),
  name TEXT NOT NULL,
  variant INTEGER NOT NULL,
  place INTEGER NOT NULL,
  time_ms INTEGER NOT NULL,
  best_lap_ms INTEGER NOT NULL,
  status TEXT NOT NULL,
  PRIMARY KEY (race_id, seat)
);
CREATE INDEX IF NOT EXISTS race_results_user ON race_results(user_id);
`;

function hashPassword(password: string, salt: string): string {
  return scryptSync(password, salt, 64).toString('hex');
}

export class Store {
  readonly db: DatabaseSync;

  constructor(path: string) {
    if (path !== ':memory:') mkdirSync(dirname(path), { recursive: true });
    this.db = new DatabaseSync(path);
    this.db.exec(SCHEMA);
    this.db.prepare(`DELETE FROM sessions WHERE expires_at < datetime('now')`).run();
    // Rooms live in memory; any left open by a previous run are over.
    this.db.prepare(`UPDATE rooms SET closed_at = datetime('now') WHERE closed_at IS NULL`).run();
  }

  /** Creates an account; null when the name is taken. */
  createUser(name: string, password: string, admin = false): UserInfo | null {
    const salt = randomBytes(16).toString('hex');
    try {
      const result = this.db.prepare(
        'INSERT INTO users (name, password_hash, password_salt, admin) VALUES (?, ?, ?, ?)')
        .run(name, hashPassword(password, salt), salt, admin ? 1 : 0);
      return { id: Number(result.lastInsertRowid), name, admin };
    } catch (error) {
      if (String(error).includes('UNIQUE')) return null;
      throw error;
    }
  }

  /** Sets a password (and the admin flag), creating the account if needed. */
  ensureUser(name: string, password: string, admin: boolean): void {
    const salt = randomBytes(16).toString('hex');
    this.db.prepare(`INSERT INTO users (name, password_hash, password_salt, admin) VALUES (?, ?, ?, ?)
      ON CONFLICT(name) DO UPDATE SET password_hash = excluded.password_hash,
        password_salt = excluded.password_salt, admin = excluded.admin`)
      .run(name, hashPassword(password, salt), salt, admin ? 1 : 0);
  }

  verifyUser(name: string, password: string): UserInfo | null {
    const row = this.db.prepare('SELECT id, name, password_hash, password_salt, admin FROM users WHERE name = ?')
      .get(name) as { id: number; name: string; password_hash: string; password_salt: string; admin: number } | undefined;
    if (!row) return null;
    const expected = Buffer.from(row.password_hash, 'hex');
    const actual = Buffer.from(hashPassword(password, row.password_salt), 'hex');
    if (expected.length !== actual.length || !timingSafeEqual(expected, actual)) return null;
    return { id: row.id, name: row.name, admin: row.admin === 1 };
  }

  createSession(userId: number): string {
    const token = randomBytes(32).toString('base64url');
    this.db.prepare(`INSERT INTO sessions (token, user_id, expires_at) VALUES (?, ?, datetime('now', ?))`)
      .run(token, userId, `+${SESSION_DAYS} days`);
    return token;
  }

  sessionUser(token: string): UserInfo | null {
    const row = this.db.prepare(`SELECT users.id, users.name, users.admin FROM sessions
      JOIN users ON users.id = sessions.user_id
      WHERE sessions.token = ? AND sessions.expires_at >= datetime('now')`)
      .get(token) as { id: number; name: string; admin: number } | undefined;
    return row ? { id: row.id, name: row.name, admin: row.admin === 1 } : null;
  }

  deleteSession(token: string): void {
    this.db.prepare('DELETE FROM sessions WHERE token = ?').run(token);
  }

  createRoom(hostId: number, s: RoomSettings): number {
    return Number(this.db.prepare(`INSERT INTO rooms
      (name, host_id, class_index, course, reverse, laps, rivals, max_players)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?)`)
      .run(s.name, hostId, s.classIndex, s.course, s.reverse ? 1 : 0, s.laps, s.rivals ? 1 : 0, s.maxPlayers)
      .lastInsertRowid);
  }

  updateRoom(roomId: number, hostId: number, s: RoomSettings): void {
    this.db.prepare(`UPDATE rooms SET name = ?, host_id = ?, class_index = ?, course = ?, reverse = ?,
      laps = ?, rivals = ?, max_players = ? WHERE id = ?`)
      .run(s.name, hostId, s.classIndex, s.course, s.reverse ? 1 : 0, s.laps, s.rivals ? 1 : 0,
           s.maxPlayers, roomId);
  }

  closeRoom(roomId: number): void {
    this.db.prepare(`UPDATE rooms SET closed_at = datetime('now') WHERE id = ?`).run(roomId);
  }

  createRace(roomId: number, s: RoomSettings): number {
    return Number(this.db.prepare(`INSERT INTO races (room_id, class_index, course, reverse, laps)
      VALUES (?, ?, ?, ?, ?)`).run(roomId, s.classIndex, s.course, s.reverse ? 1 : 0, s.laps).lastInsertRowid);
  }

  /** Stores a finished race's results in one transaction. */
  finishRace(raceId: number, results: RaceResult[]): void {
    const insert = this.db.prepare(`INSERT INTO race_results
      (race_id, seat, user_id, name, variant, place, time_ms, best_lap_ms, status)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)`);
    this.db.exec('BEGIN');
    try {
      for (const r of results) {
        insert.run(raceId, r.seat, r.userId, r.name, r.variant, r.place, r.timeMs, r.bestLapMs, r.status);
      }
      this.db.prepare(`UPDATE races SET finished_at = datetime('now') WHERE id = ?`).run(raceId);
      this.db.exec('COMMIT');
    } catch (error) {
      this.db.exec('ROLLBACK');
      throw error;
    }
  }

  /** Best player lap for every course variant and class that has one. */
  records(): RecordRow[] {
    const rows = this.db.prepare(`
      WITH laps AS (
        SELECT races.course, races.reverse, races.class_index, rr.name, rr.variant, rr.best_lap_ms,
               CASE WHEN rr.status = 'finished' THEN rr.time_ms END AS race_ms, races.started_at,
               ROW_NUMBER() OVER (PARTITION BY races.course, races.reverse, races.class_index
                                  ORDER BY rr.best_lap_ms) AS rank
        FROM race_results rr JOIN races ON races.id = rr.race_id
        WHERE rr.user_id IS NOT NULL AND rr.best_lap_ms > 0)
      SELECT course, reverse, class_index, name, variant, best_lap_ms, race_ms, started_at
      FROM laps WHERE rank = 1 ORDER BY course, reverse, class_index`).all() as Array<{
        course: number; reverse: number; class_index: number; name: string; variant: number;
        best_lap_ms: number; race_ms: number | null; started_at: string }>;
    return rows.map((r) => ({ course: r.course, reverse: r.reverse === 1, classIndex: r.class_index,
      name: r.name, variant: r.variant, bestLapMs: r.best_lap_ms, raceMs: r.race_ms, at: r.started_at }));
  }

  /** A player's recent races, newest first. */
  history(userId: number, limit = 20): Array<{ raceId: number; course: number; reverse: boolean; classIndex: number;
      place: number; entrants: number; timeMs: number; bestLapMs: number; status: string; at: string }> {
    const rows = this.db.prepare(`SELECT races.id, races.course, races.reverse, races.class_index, rr.place,
        (SELECT COUNT(*) FROM race_results x WHERE x.race_id = races.id) AS entrants,
        rr.time_ms, rr.best_lap_ms, rr.status, races.started_at
      FROM race_results rr JOIN races ON races.id = rr.race_id
      WHERE rr.user_id = ? ORDER BY races.id DESC LIMIT ?`).all(userId, limit) as Array<{
        id: number; course: number; reverse: number; class_index: number; place: number; entrants: number;
        time_ms: number; best_lap_ms: number; status: string; started_at: string }>;
    return rows.map((r) => ({ raceId: r.id, course: r.course, reverse: r.reverse === 1, classIndex: r.class_index,
      place: r.place, entrants: r.entrants, timeMs: r.time_ms, bestLapMs: r.best_lap_ms, status: r.status,
      at: r.started_at }));
  }
}
