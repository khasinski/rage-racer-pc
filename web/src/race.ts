// The race loop: the simulation's fixed ticks, frames drawn between the last
// two physics steps, and the HUD. Offline practice runs on its own; online a
// RaceNet (online.ts) carries the player's input or the server's frames.
import { app, setHint, show, toast } from './app';
import { SEAT_FINISHED, TICK_MS, TICK_RATE } from './constants';
import { Tachometer } from './hud';
import { clearKeyEdges, consumeKey, PAD, samplePad } from './input';
import { drawDeadline, drawStandings, followRace, resetOverlay, spectateKeys } from './overlay';
import { PHASE_COUNTDOWN, PHASE_FINISHED, PHASE_RACING, type Hud, type Rage } from './rage';
import { Renderer } from './renderer';
import { $, formatTime, textWriter } from './views';

/* Offline practice; an online player predicts their own car; a spectator
 * watches the server's frames. */
export type RaceMode = 'offline' | 'player' | 'spectator';

/** Who races: seats below `humans` are players, the rest retail rivals. */
export interface RaceState {
  mode: RaceMode;
  names: string[]; // every seat
  humans: number;
  userIds: (number | null)[]; // per player seat
  localSeat: number; // -1 for a spectator
  deadlineAt: number | null; // performance.now() when the server closes the race
}

/** The network side of an online race. */
export interface RaceNet {
  /** Before a frame's ticks: milliseconds to add to the local clock. */
  sync(rage: Rage): number;
  /** Before each tick. */
  tick(rage: Rage): void;
  /** Esc: leave the race (it has already stopped here). */
  leave(): void;
}

export function practiceRace(): RaceState {
  return {
    mode: 'offline', names: ['You', ...Array.from({ length: 11 }, (_, i) => `Rival ${i + 1}`)],
    humans: 1, userIds: [null], localSeat: 0, deadlineAt: null,
  };
}

export let race: RaceState = practiceRace();
export let racing = false;
export const raceSettings = { shadows: true };
let net: RaceNet | null = null;

const canvas = $<HTMLCanvasElement>('view');
const tachometer = new Tachometer($<HTMLCanvasElement>('tacho'));
let renderer: Renderer | null = null;
const hudPlace = textWriter($('hud-place'));
const hudLap = textWriter($('hud-lap'));
const hudTime = textWriter($('hud-time'));
const hudBanner = textWriter($('hud-banner'));

let last = 0;
let accumulator = 0;
/* Simulated time (ms) and the times of the last two physics steps: frames
 * are drawn between those two snapshots, one step behind the simulation. */
let simTime = 0;
let previousStep = 0;
let currentStep = 0;
/* Offline, Start pauses during the countdown and the race (race_scene_rules.c
 * CanPauseRace); online the server's clock does not stop. */
let paused = false;
let startHeld = false;
let phase = 0;
let standingsAt = 0;

/** Runs the race the module just prepared; the race screen is on show. */
export function beginRace(rage: Rage, state: RaceState, raceNet: RaceNet | null = null): void {
  race = state;
  net = raceNet;
  renderer ??= new Renderer(canvas, rage);
  renderer.shadows = raceSettings.shadows;
  resize();
  tachometer.prepare(rage);
  last = performance.now();
  accumulator = simTime = previousStep = currentStep = standingsAt = 0;
  paused = startHeld = false;
  phase = 0;
  clearKeyEdges();
  resetOverlay(race);
  racing = true;
  app.audio?.startRace();
}

export function stopRace(): void {
  racing = false;
  net = null;
  race.deadlineAt = null;
  app.audio?.stopRace();
}

function resize() {
  renderer?.resize(canvas.clientWidth, canvas.clientHeight);
}
addEventListener('resize', resize);

function drawHud(h: Hud) {
  hudPlace(h.place > 0 ? `${h.place}/${h.entrants}` : `–/${h.entrants}`);
  hudLap(`${Math.max(1, Math.min(h.lap || 1, h.laps))}/${h.laps}`);
  hudTime(formatTime(h.timeMs));
  hudBanner(h.phase === PHASE_COUNTDOWN ? String(Math.ceil(h.countdown / TICK_RATE) || 'GO')
    : h.phase === PHASE_FINISHED || h.status === SEAT_FINISHED ? 'FINISH'
    : paused ? 'PAUSE' : '');
}

function leave() {
  const leaving = net;
  stopRace();
  if (leaving) {
    leaving.leave();
  } else {
    show('setup');
    $('start').focus();
  }
}

function frame(now: number) {
  requestAnimationFrame(frame);
  const rage = app.rage;
  if (!racing || !renderer || !rage || app.screen !== 'race') return;
  if (consumeKey('Escape')) {
    leave();
    return;
  }
  if (consumeKey('KeyM')) app.audio?.toggleMute();
  const pad = samplePad();
  const start = (pad.held & PAD.START) !== 0;
  if (race.mode === 'offline' && start && !startHeld && (phase === PHASE_COUNTDOWN || phase === PHASE_RACING)) paused = !paused;
  startHeld = start;
  accumulator = paused ? 0 : Math.min(accumulator + (now - last), 250);
  last = now;
  rage.setPad(pad);
  if (net) accumulator += net.sync(rage);
  while (accumulator >= TICK_MS) {
    net?.tick(rage);
    phase = rage.tick();
    if (phase < 0) {
      const mode = race.mode;
      stopRace();
      toast('The race stopped unexpectedly.');
      show(mode === 'offline' ? 'setup' : 'room');
      return;
    }
    accumulator -= TICK_MS;
    simTime += TICK_MS;
    if (rage.lastTickStepped()) {
      previousStep = currentStep;
      currentStep = simTime;
    }
  }
  const interval = Math.max(TICK_MS, currentStep - previousStep);
  const t = Math.min(1, Math.max(0, (simTime + accumulator - currentStep) / interval));
  const vertices = rage.buildFrame(renderer.aspect, t);
  if (vertices >= 0) renderer.update(vertices);
  const h = rage.hud();
  if (race.mode !== 'offline' && h.phase >= PHASE_RACING) setHint('');
  app.audio?.update(h, paused);
  drawHud(h);
  if (spectateKeys(rage, race)) standingsAt = 0;
  if (now - standingsAt > 250) {
    standingsAt = now;
    if (race.mode !== 'offline') drawStandings(rage, race, app.latency);
    followRace(rage, race);
  }
  drawDeadline(race, now);
  renderer.render();
  tachometer.draw(rage);
}

export function runRaceLoop(): void {
  requestAnimationFrame(frame);
}
