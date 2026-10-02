// On-screen controls for touch screens: buttons that hold the same PS1 pad
// buttons the keys do (input.ts), so a phone or tablet races like a keyboard.
// Several fingers at once: each pointer holds its own button.
import { PAD } from './input';
import { $ } from './views';

const overlay = $('touch');
const fingers = new Map<number, number>(); // pointer id → pad bit
/* Keys some buttons stand for (leaving the race, which is not a pad button). */
const keyTaps: string[] = [];
/* ✕ leaves only when tapped twice within this long: one stray touch must not
 * retire the car. */
const LEAVE_CONFIRM_MS = 2000;
let leaveArmedAt = -Infinity;

/** The pad buttons fingers hold now. */
export function touchHeld(): number {
  let bits = 0;
  for (const bit of fingers.values()) bits |= bit;
  return bits;
}

/** Key presses the buttons made since the last call (input.ts). */
export function takeTouchKeys(): string[] {
  return keyTaps.splice(0);
}

/** Whether this device is driven by touch rather than keys or a gamepad. */
export const touchFirst = matchMedia('(pointer: coarse)').matches;

const BITS: Record<string, number> = {
  left: PAD.LEFT, right: PAD.RIGHT, accelerate: PAD.CROSS, brake: PAD.SQUARE,
  up: PAD.R1, down: PAD.L1, camera: PAD.TRIANGLE, pause: PAD.START,
};

function release(id: number): void {
  const bit = fingers.get(id);
  fingers.delete(id);
  if (bit === undefined) return;
  for (const button of overlay.querySelectorAll<HTMLElement>('[data-touch]')) {
    if (BITS[button.dataset.touch!] === bit && ![...fingers.values()].includes(bit)) button.classList.remove('held');
  }
}

overlay.addEventListener('pointerdown', (event) => {
  const button = (event.target as HTMLElement).closest<HTMLElement>('[data-touch]');
  if (!button) return;
  event.preventDefault();
  const name = button.dataset.touch!;
  if (name === 'leave') {
    const now = performance.now();
    if (now - leaveArmedAt <= LEAVE_CONFIRM_MS) {
      leaveArmedAt = -Infinity;
      button.classList.remove('armed');
      keyTaps.push('Escape');
    } else {
      leaveArmedAt = now;
      button.classList.add('armed');
      setTimeout(() => button.classList.remove('armed'), LEAVE_CONFIRM_MS);
    }
    return;
  }
  fingers.set(event.pointerId, BITS[name]);
  button.classList.add('held');
  overlay.setPointerCapture(event.pointerId);
});
for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) {
  overlay.addEventListener(type, (event) => release((event as PointerEvent).pointerId));
}
overlay.addEventListener('contextmenu', (event) => event.preventDefault());
