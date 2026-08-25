#ifndef RAGE_TRACK_LIGHTING_H
#define RAGE_TRACK_LIGHTING_H

/* Converts the authored 0..256 track-zone ramp into a renderer-neutral
 * ambient light colour. `zoneCode == 0` is the warm tunnel transition used
 * by the original game; other zones darken all channels uniformly. */
void RageTrackZoneLightColor(int blend, int zoneCode, float out[3]);

#endif
