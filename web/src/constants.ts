// Values the client shares between modules.

/** The simulation's fixed clock: ticks per second, and one tick in ms. */
export const TICK_RATE = 50;
export const TICK_MS = 1000 / TICK_RATE;

/** A seat's race status (rw_seat_status, Hud.status). */
export const SEAT_RACING = 1;
export const SEAT_FINISHED = 2;
export const SEAT_RETIRED = 3;

/** The PAL screen the HUD and the mirror are laid out on. */
export const PAL_WIDTH = 320;
export const PAL_HEIGHT = 240;
