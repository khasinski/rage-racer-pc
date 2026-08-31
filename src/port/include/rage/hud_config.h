#ifndef RAGE_HUD_CONFIG_H
#define RAGE_HUD_CONFIG_H

/* Horizontal anchors are expressed in the game's 320-wide HUD coordinates.
 * Modern 16:9 adds 53 pixels on each side of that canvas. */
int HudLeftX(int x);
int HudRightX(int x);

/*
 * Where a HUD element at this x belongs when the layout is widened.
 *
 * Only what sits against an edge follows that edge out. A readout placed
 * near the middle of the 320-wide canvas is positioned against the centre,
 * not against a side, and moving it leaves it stranded: the split time's sign
 * sits at 120 beside a time drawn at 128, and pushing the sign fifty-three
 * pixels left while the time stayed put is exactly what that looks like.
 */
int HudAnchorX(int x);
int HudShowLapTimes(void);
int HudShowTimeLimit(void);

#endif
