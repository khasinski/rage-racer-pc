#ifndef RAGE_HUD_CONFIG_H
#define RAGE_HUD_CONFIG_H

/* Horizontal anchors are expressed in the game's 320-wide HUD coordinates.
 * Modern 16:9 adds 53 pixels on each side of that canvas. */
int RageHudLeftX(int x);
int RageHudRightX(int x);
int RageHudShowLapTimes(void);
int RageHudShowTimeLimit(void);

#endif
