#ifndef RAGE_PORT_GRAPHICS_OVERLAY_H
#define RAGE_PORT_GRAPHICS_OVERLAY_H

#include <stdint.h>

/* In-game settings. Escape opens a panel over the frozen game; the game is
 * held for as long as the panel is up. The panel draws through the
 * presentation device, so it appears over either renderer. Pages cover
 * graphics, controls, audio and the view. */

/* Registers the input and swapchain hooks. Safe before platform start-up. */
void PortGraphicsOverlayInstall(void);

/* Non-zero while the panel is open. The main loop holds game logic then. */
int PortGraphicsOverlayActive(void);

/* Non-zero once after the panel closes, so the caller can discard the keys
 * pressed while it was up instead of passing them to the game. */
int PortGraphicsOverlayTakeClosed(void);

/* Applies pending changes and redraws the panel. Called from the main loop
 * between presentations, never from inside one. */
void PortGraphicsOverlayTick(void);

/* Drops GPU objects ahead of device destruction or presentation restart. */
void PortGraphicsOverlayReleaseGpu(void);

/* The controls the player holds on the on-screen pad, bit n for TouchControl n
 * (touch_model.h). Zero when the pad is off or the panel is up. */
uint32_t PortTouchHeldControls(void);

#endif
