// Keeps a predicting player's clock just far enough ahead of the server that
// each input reaches it a couple of ticks before the tick it was predicted for
// (the server applies inputs at that tick, so the player's own car needs no
// correction while they arrive in time). Every frame reports how many ticks
// early the player's latest input arrived; the clock is steered by the
// smallest recent margin: a late input jumps it ahead, a margin far above the
// target jumps it back, and small differences ease it by a twentieth of a tick.
// Pure: no DOM, no WebAssembly (see scripts/clock-sync-test.mjs).
import { ACK_WORDS } from '../shared/protocol.ts';

export const MARGIN_WINDOW = 12; // frames, about half a second
export const MARGIN_TARGET = 2; // ticks of slack for the network's jitter
/* Beyond this many ticks over the target the clock jumps back instead of easing. */
const JUMP_BACK_ABOVE = 8;
const EASE_TICKS = 0.05;

export class ClockSync {
  /* How many ticks early the inputs reached the server, oldest first. */
  private readonly margins: number[] = [];
  /* After the clock moves, margins count again from this input on (older
   * ones were sent on the old clock). */
  private settleSeq = 0;
  /* The first applied frame only sets the starting point. */
  private started = false;

  private readonly tickMs: number;

  constructor(tickMs: number) {
    this.tickMs = tickMs;
  }

  reset(): void {
    this.margins.length = 0;
    this.settleSeq = 0;
    this.started = false;
  }

  /** A frame arrived: notes the margin of this seat's latest input. */
  observe(acks: Int32Array, seat: number): void {
    const at = seat * ACK_WORDS;
    if (seat < 0 || at + 2 >= acks.length || acks[at] >>> 0 < this.settleSeq) return;
    this.margins.push(acks[at + 2]);
    if (this.margins.length > MARGIN_WINDOW) this.margins.shift();
  }

  /** After a frame was applied: the milliseconds to add to the local clock.
   *  `nextSeq` is the sequence number the next input will carry. */
  adjust(nextSeq: number): number {
    if (!this.started) {
      this.started = true;
      this.margins.length = 0;
      return 0;
    }
    if (!this.margins.length) return 0;
    const lowest = Math.min(...this.margins);
    if (lowest < 0 || lowest > MARGIN_TARGET + JUMP_BACK_ABOVE) {
      // Inputs arrive late (start, or the network got slower) or far too
      // early (it got faster): move the clock at once.
      this.margins.length = 0;
      this.settleSeq = nextSeq;
      return (MARGIN_TARGET - lowest) * this.tickMs;
    }
    if (lowest < MARGIN_TARGET) return this.tickMs * EASE_TICKS;
    if (lowest > MARGIN_TARGET + 1) return -this.tickMs * EASE_TICKS;
    return 0;
  }
}
