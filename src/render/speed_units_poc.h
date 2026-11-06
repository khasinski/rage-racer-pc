#ifndef RAGE_SPEED_UNITS_POC_H
#define RAGE_SPEED_UNITS_POC_H
#include "hud_unit_diff.h"
enum { RAGE_UNITS_AUTO, RAGE_UNITS_KMH, RAGE_UNITS_MPH };
/* Override becomes effective only after a verified native snapshot is selected. */
void RageSpeedUnitsConfigure(int car, const char *region, const char *requested);
int RageSpeedUnitsOverrideRequested(void);
int RageSpeedUnits(void);
int RageSpeedUnitsRetailDiff(unsigned car, RageHudUnitDiff *out);
int RageSpeedUnitsPatchSnapshot(uint8_t snapshot[RAGE_HUD_UNIT_BYTES]);
/* HD: compose the verified retail label pixels over the pack's background.
 * Never apply a binary VRAM XOR to RGBA data or invent a new glyph.
 * Zero rejects an incompatible HD face, so presentation uses the verified
 * native face in the same renderer instead of showing mismatched units. */
int RageSpeedUnitsPatchRGBA(const char *key, uint8_t *pixels, unsigned w, unsigned h);
#endif
