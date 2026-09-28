// What the page's modules share: the WebAssembly module, the account
// session, the screen on show and its status line.
import type { RaceAudio } from './audio';
import { Session } from './net';
import { feed } from './overlay';
import { Rage } from './rage';
import { $, textWriter } from './views';

export type Screen = 'auth' | 'disc' | 'lobby' | 'room' | 'setup' | 'race';
const SCREENS: Screen[] = ['auth', 'disc', 'lobby', 'room', 'setup'];
/* Where toast() writes on each screen (the race also shows it in its feed). */
const STATUS: Record<Screen, string> = {
  auth: 'auth-status', disc: 'disc-status', lobby: 'lobby-status', room: 'room-status', setup: 'setup-status',
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
  /* Round trips by user id from the server's latency message (null while
   * a player is reconnecting). */
  latency: {} as Record<number, number | null>,
};

/** The line in the middle of the race view (loading, waiting, reconnecting). */
export const setHint = textWriter($('hud-hint'));

export function show(next: Screen): void {
  app.screen = next;
  for (const id of SCREENS) $(id).hidden = id !== next;
  $('hud').hidden = $('view').hidden = next !== 'race';
  if (next !== 'race') $('results').hidden = true;
}

export function toast(message: string): void {
  $(STATUS[app.screen]).textContent = message;
  if (app.screen === 'race') feed(message);
}
