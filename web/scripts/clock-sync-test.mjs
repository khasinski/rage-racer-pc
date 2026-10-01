// The predicting player's clock (src/clock-sync.ts).   node scripts/clock-sync-test.mjs
import { ClockSync, MARGIN_TARGET, MARGIN_WINDOW } from '../src/clock-sync.ts';
import { checks } from './lib/harness.mjs';

const { check, report } = checks();
const TICK = 20;
/** Frame acks for seat 1 of two: [seq, tick, margin] per seat. */
const acks = (seq, margin) => Int32Array.from([0, 0, 0, seq, 0, margin]);

const started = () => {
  const clock = new ClockSync(TICK);
  clock.adjust(1); // the first frame only sets the starting point
  return clock;
};

{
  const clock = new ClockSync(TICK);
  clock.observe(acks(1, -5), 1);
  check(clock.adjust(2) === 0, 'the first applied frame never moves the clock');
  check(clock.adjust(2) === 0, 'and without a margin since, nothing moves');
}
{
  const clock = started();
  clock.observe(acks(10, -3), 1);
  check(clock.adjust(11) === (MARGIN_TARGET + 3) * TICK, 'a late input jumps the clock ahead to the target at once');
  clock.observe(acks(10, -3), 1); // sent on the old clock
  check(clock.adjust(11) === 0, 'inputs sent before the jump are not counted again');
  clock.observe(acks(11, 2), 1);
  check(clock.adjust(12) === 0, 'on target nothing moves');
}
{
  const clock = started();
  clock.observe(acks(5, MARGIN_TARGET + 9), 1);
  check(clock.adjust(6) === -9 * TICK, 'far too early jumps back');
}
{
  const clock = started();
  clock.observe(acks(5, 1), 1);
  check(clock.adjust(6) === TICK * 0.05, 'slightly short eases ahead');
  const later = started();
  later.observe(acks(5, MARGIN_TARGET + 3), 1);
  check(later.adjust(6) === -TICK * 0.05, 'slightly early eases back');
}
{
  const clock = started();
  clock.observe(acks(1, 0), 1);
  for (let i = 0; i < MARGIN_WINDOW; i++) clock.observe(acks(2 + i, 3), 1);
  check(clock.adjust(20) === 0, `the smallest of the last ${MARGIN_WINDOW} margins decides (an old one drops out)`);
}
{
  const clock = started();
  // Seat 0 (someone else) is late; this player (seat 1) is on target.
  clock.observe(Int32Array.from([7, 0, -4, 7, 0, MARGIN_TARGET]), 1);
  check(clock.adjust(8) === 0, 'only this seat\'s margin counts');
  const short = started();
  short.observe(Int32Array.from([1, 0, -4]), 1); // a frame without seat 1
  check(short.adjust(2) === 0, 'a frame without the seat is ignored');
  const spectator = started();
  spectator.observe(acks(1, -4), -1);
  check(spectator.adjust(2) === 0, 'a spectator (seat -1) has no clock to steer');
}
{
  const clock = started();
  clock.observe(acks(3, -2), 1);
  clock.reset();
  clock.observe(acks(3, -2), 1);
  check(clock.adjust(4) === 0, 'after a reset the first frame starts again');
}
report('clock sync ok');
