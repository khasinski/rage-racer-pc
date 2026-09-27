// The desktop controls: keys and gamepad buttons become PS1 pad buttons, and
// the bridge applies the retail button presets to them (rw_set_pad).
// Keyboard: input_config.c's default bindings. Gamepad: analog_pad.c's
// GamepadButtons, with the left stick and triggers driving its NeGcon.

export const PAD = {
  L2: 0x1, R2: 0x2, L1: 0x4, R1: 0x8,
  TRIANGLE: 0x10, CIRCLE: 0x20, CROSS: 0x40, SQUARE: 0x80,
  SELECT: 0x100, L3: 0x200, R3: 0x400, START: 0x800,
  UP: 0x1000, RIGHT: 0x2000, DOWN: 0x4000, LEFT: 0x8000,
} as const;

/* input_config.c default_keys. */
const KEYS: Record<string, number> = {
  KeyW: PAD.L2, KeyE: PAD.R2, KeyQ: PAD.L1, KeyR: PAD.R1,
  KeyS: PAD.TRIANGLE, KeyD: PAD.CIRCLE, KeyX: PAD.CROSS, KeyZ: PAD.SQUARE,
  Backspace: PAD.SELECT, Digit1: PAD.L3, Digit2: PAD.R3, Enter: PAD.START,
  ArrowUp: PAD.UP, ArrowRight: PAD.RIGHT, ArrowDown: PAD.DOWN, ArrowLeft: PAD.LEFT,
};

/* Standard-mapping button index -> PS1 button, as GamepadButtons. */
const GAMEPAD_BUTTONS: [number, number][] = [
  [12, PAD.UP], [15, PAD.RIGHT], [13, PAD.DOWN], [14, PAD.LEFT],
  [0, PAD.CROSS], [2, PAD.SQUARE], [1, PAD.CIRCLE], [3, PAD.TRIANGLE],
  [4, PAD.L1], [5, PAD.R1], [9, PAD.START],
];

export interface PadSample {
  held: number;
  stickX: number; // -32768..32767
  rightTrigger: number; // 0..32767
  leftTrigger: number; // 0..32767
  gamepad: boolean;
}

const held = new Set<string>();
const pressedEdges = new Set<string>();
/* Keys pressed since the last sample: a tap released before the next frame
 * still reaches the pad for one sample. */
const tapped = new Set<string>();

addEventListener('keydown', (event) => {
  if (!held.has(event.code)) { pressedEdges.add(event.code); tapped.add(event.code); }
  held.add(event.code);
  if (event.code in KEYS && !(event.target instanceof HTMLInputElement)) event.preventDefault();
});
addEventListener('keyup', (event) => held.delete(event.code));
addEventListener('blur', () => { held.clear(); tapped.clear(); });

const axis = (value: number) => Math.round(Math.max(-1, Math.min(1, value)) * 32767);

export function samplePad(): PadSample {
  let buttons = 0;
  for (const code of held) buttons |= KEYS[code] ?? 0;
  for (const code of tapped) buttons |= KEYS[code] ?? 0;
  tapped.clear();
  const sample: PadSample = { held: buttons, stickX: 0, rightTrigger: 0, leftTrigger: 0, gamepad: false };
  const pad = [...(navigator.getGamepads?.() ?? [])].find((p) => p?.connected && p.mapping === 'standard');
  if (pad) {
    for (const [index, bit] of GAMEPAD_BUTTONS) if (pad.buttons[index]?.pressed) sample.held |= bit;
    sample.stickX = axis(pad.axes[0] ?? 0);
    sample.rightTrigger = axis(pad.buttons[7]?.value ?? 0);
    sample.leftTrigger = axis(pad.buttons[6]?.value ?? 0);
    sample.gamepad = true;
  }
  return sample;
}

export function consumeKey(code: string): boolean {
  if (!pressedEdges.has(code)) return false;
  pressedEdges.delete(code);
  return true;
}

/* Pressed edges nobody consumed must not fire later. */
export function clearKeyEdges(): void { pressedEdges.clear(); }
