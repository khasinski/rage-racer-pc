// Keyboard (and, if present, a standard gamepad) mapped to the simulation's
// driver levels: digital steering, 0..256 pedals, gear-change edges.
import type { DriverLevels } from './rage';

const held = new Set<string>();
const pressedEdges = new Set<string>();

addEventListener('keydown', (event) => {
  if (!held.has(event.code)) pressedEdges.add(event.code);
  held.add(event.code);
  if (event.code.startsWith('Arrow') || event.code === 'Space') event.preventDefault();
});
addEventListener('keyup', (event) => held.delete(event.code));
addEventListener('blur', () => held.clear());

const any = (...codes: string[]) => codes.some((code) => held.has(code));
const edge = (...codes: string[]) => codes.some((code) => pressedEdges.has(code));

let padUp = false;
let padDown = false;

export function sampleDriver(): DriverLevels {
  let left = any('ArrowLeft', 'KeyA');
  let right = any('ArrowRight', 'KeyD');
  let throttle = any('ArrowUp', 'KeyW') ? 256 : 0;
  let brake = any('ArrowDown', 'KeyS', 'Space') ? 256 : 0;
  let shiftUp = edge('KeyE', 'KeyX');
  let shiftDown = edge('KeyQ', 'KeyZ');
  pressedEdges.clear();

  for (const pad of navigator.getGamepads?.() ?? []) {
    if (!pad || pad.mapping !== 'standard') continue;
    const x = pad.axes[0] ?? 0;
    left ||= x < -0.35 || pad.buttons[14]?.pressed === true;
    right ||= x > 0.35 || pad.buttons[15]?.pressed === true;
    throttle = Math.max(throttle, Math.round((pad.buttons[7]?.value ?? 0) * 256),
                        pad.buttons[0]?.pressed ? 256 : 0);
    brake = Math.max(brake, Math.round((pad.buttons[6]?.value ?? 0) * 256),
                     pad.buttons[2]?.pressed ? 256 : 0);
    const up = pad.buttons[5]?.pressed === true;
    const down = pad.buttons[4]?.pressed === true;
    shiftUp ||= up && !padUp;
    shiftDown ||= down && !padDown;
    padUp = up;
    padDown = down;
  }
  return { left, right, throttle, brake, shiftUp, shiftDown };
}

export function consumeKey(code: string): boolean {
  if (!pressedEdges.has(code)) return false;
  pressedEdges.delete(code);
  return true;
}
