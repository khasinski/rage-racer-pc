// What the page's modules share: the WebAssembly module, the account
// session, the screen on show and its status line.
import type { Garage, Transport } from '../shared/protocol.ts';
import type { Logo } from './logo';
import type { RaceAudio } from './audio';
import { Session } from './net';
import { feed } from './overlay';
import { Rage } from './rage';
import { $, textWriter } from './views';

export type Screen = 'auth' | 'disc' | 'lobby' | 'room' | 'setup' | 'garage' | 'race';
const SCREENS: Screen[] = ['auth', 'disc', 'lobby', 'room', 'setup', 'garage'];
/* Where toast() writes on each screen (the race also shows it in its feed). */
const STATUS: Record<Screen, string> = {
  auth: 'auth-status', disc: 'disc-status', lobby: 'lobby-status', room: 'room-status', setup: 'setup-status',
  garage: 'garage-status',
  race: 'lobby-status',
};

export const ragePromise = Rage.load();
export const session = new Session();
export const app = {
  rage: null as Rage | null, // once ragePromise resolved
  audio: null as RaceAudio | null,
  screen: 'auth' as Screen,
  discLoaded: false,
  /* Per car variant: false where the disc only offers a manual gearbox. */
  automaticCars: [] as boolean[],
  /* The player's saved car paints (see garage.ts). */
  garage: {} as Garage,
  /* The player's team logo, or null for none. */
  logo: null as Logo | null,
  /* Round trips by user id from the server's latency message (null while
   * a player is reconnecting). */
  latency: {} as Record<number, number | null>,
  /* How each member's race traffic travels (see the latency message). */
  transport: {} as Record<number, Transport>,
  /* The car the player last chose (garage, practice or a room), which the
   * race forms start with; -1 before any. Kept in this browser. */
  lastCar: readLastCar(),
};

const LAST_CAR_KEY = 'rage-racer.last-car';
function readLastCar(): number {
  try {
    const value = Number(localStorage.getItem(LAST_CAR_KEY));
    return Number.isInteger(value) && localStorage.getItem(LAST_CAR_KEY) !== null ? value : -1;
  } catch { return -1; }
}

/** Remembers the car the player chose, for the next form that offers it. */
export function rememberCar(variant: number): void {
  if (!Number.isInteger(variant) || variant < 0) return;
  app.lastCar = variant;
  try { localStorage.setItem(LAST_CAR_KEY, String(variant)); } catch { /* private mode: this visit only */ }
}

/** The line in the middle of the race view (loading, waiting, reconnecting). */
export const setHint = textWriter($('hud-hint'));

export function show(next: Screen): void {
  app.screen = next;
  document.body.dataset.screen = next; // the touch controls show on the race (style.css)
  for (const id of SCREENS) $(id).hidden = id !== next;
  $('hud').hidden = $('view').hidden = next !== 'race';
  if (next !== 'race') $('results').hidden = true;
}

export function toast(message: string): void {
  $(STATUS[app.screen]).textContent = message;
  if (app.screen === 'race') feed(message);
}
