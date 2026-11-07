// The one 50 Hz clock that steps every running race. A timer firing late
// (a busy event loop) is caught up with several steps, at most MAX_CATCH_UP,
// so races keep simulated time; it stops itself when nothing runs.
const TICK_MS = 1000 / 50;
const POLL_MS = 4;
const MAX_CATCH_UP = 10;

export class RaceClock {
  private timer: ReturnType<typeof setInterval> | null = null;
  private readonly step: () => boolean;

  /** `step` advances every race by one tick; it returns whether any still runs. */
  constructor(step: () => boolean) {
    this.step = step;
  }

  /** Starts the clock if it is not running. */
  ensureRunning(): void {
    if (this.timer) return;
    let next = performance.now();
    this.timer = setInterval(() => {
      const now = performance.now();
      let steps = 0;
      let running = true;
      while (next <= now && steps++ < MAX_CATCH_UP && running) {
        running = this.step();
        next += TICK_MS;
      }
      if (next < now) next = now;
      if (!running) this.stop();
    }, POLL_MS);
  }

  private stop(): void {
    if (this.timer) clearInterval(this.timer);
    this.timer = null;
  }
}
