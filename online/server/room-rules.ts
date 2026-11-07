// What rooms accept: room settings, the car a member may race, and the car
// and course a duel rolls. All checked against the simulation's own rules.
import { randomBytes, randomInt } from 'node:crypto';
import { TIRE_COMPOUND_COUNT, type DuelSetup, type RoomSettings } from '../shared/protocol.ts';
import type { Simulation } from './sim.ts';

/** An error for the player, or null when the settings can be used. */
export function checkSettings(sim: Simulation, s: RoomSettings): string | null {
  if (!s || typeof s !== 'object') return 'Missing room settings.';
  const int = (v: unknown, lo: number, hi: number) => Number.isInteger(v) && (v as number) >= lo && (v as number) <= hi;
  if (typeof s.name !== 'string' || !s.name.trim() || s.name.length > 40) return 'Give the room a name (up to 40 characters).';
  if (!int(s.classIndex, 0, 5)) return 'Unknown class.';
  if (!int(s.course, 0, 3) || !sim.courseAllowed(s.classIndex, s.course)) return 'That course is not raced in this class.';
  if (!int(s.laps, 1, 6)) return 'Laps must be between 1 and 6.';
  if (typeof s.reverse !== 'boolean' || typeof s.rivals !== 'boolean') return 'Malformed room settings.';
  const most = sim.maxHumans(s.classIndex, s.course, s.reverse);
  if (!int(s.maxPlayers, 1, most)) return `This course has ${most} starting places.`;
  return null;
}

/** Checked settings, with only the known fields and the name trimmed. */
export function normalizeSettings(s: RoomSettings): RoomSettings {
  return { name: s.name.trim(), classIndex: s.classIndex, course: s.course, reverse: s.reverse,
           laps: s.laps, rivals: s.rivals, maxPlayers: s.maxPlayers };
}

export function carValid(sim: Simulation, classIndex: number, variant: number, manual: boolean): boolean {
  return sim.carAllowed(classIndex, variant) && (manual || sim.carAutomatic(variant));
}

export function tireValid(tire: unknown): tire is number {
  return typeof tire === 'number' && Number.isInteger(tire) && tire >= 0 && tire < TIRE_COMPOUND_COUNT;
}

/** The first car a class offers, or -1. */
export function firstCar(sim: Simulation, classIndex: number): number {
  for (let model = 0; model < sim.carModels(); model++) {
    const variant = sim.classCar(classIndex, model);
    if (variant >= 0) return variant;
  }
  return -1;
}

const pick = <T>(items: T[]): T => items[randomInt(items.length)];

const ROOM_MOODS = ['Midnight', 'Sunny', 'Wet', 'Golden', 'Neon', 'Quiet', 'Fast', 'Lazy'];
const ROOM_PLACES = ['Harbor', 'Ridge', 'Hairpin', 'Tunnel', 'Summit', 'Loop', 'Bridge', 'Coast'];

/** The room a single click makes: a made-up name, the first class and course
 *  the disc offers, three laps, rivals filling the grid. The host changes
 *  anything in the room. Null when no course can be raced. */
export function rollRoom(sim: Simulation): RoomSettings | null {
  const classIndex = 0;
  const course = [0, 1, 2, 3].find((c) => sim.courseAllowed(classIndex, c));
  if (course === undefined) return null;
  return {
    name: `${pick(ROOM_MOODS)} ${pick(ROOM_PLACES)} ${1000 + randomInt(9000)}`,
    classIndex, course, reverse: false, laps: 3, rivals: true,
    maxPlayers: Math.min(8, sim.maxHumans(classIndex, course, false)),
  };
}

/** One car and one course, drawn once, for two drivers who share the link.
 *  `taken` tells whether an invite token is already in use. */
export function rollDuel(sim: Simulation, hostName: string, taken: (token: string) => boolean):
  { settings: RoomSettings; duel: DuelSetup } | null {
  const duelCourses = (classIndex: number) => [0, 1, 2, 3].filter((course) =>
    sim.courseAllowed(classIndex, course) && sim.maxHumans(classIndex, course, false) >= 2);
  const classes = [0, 1, 2, 3, 4, 5].filter((classIndex) => duelCourses(classIndex).length > 0);
  if (!classes.length) return null;
  const classIndex = pick(classes);
  const cars: number[] = [];
  for (let model = 0; model < sim.carModels(); model++) {
    const variant = sim.classCar(classIndex, model);
    if (variant >= 0) cars.push(variant);
  }
  if (!cars.length) return null;
  const course = pick(duelCourses(classIndex));
  const variant = pick(cars);
  let token: string;
  do token = randomBytes(9).toString('base64url');
  while (taken(token));
  return {
    settings: {
      name: `${hostName}'s duel`.slice(0, 40), classIndex, course,
      reverse: randomInt(2) === 1, laps: 3, rivals: false, maxPlayers: 2,
    },
    duel: { variant, manual: !sim.carAutomatic(variant), token },
  };
}
