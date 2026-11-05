#ifndef RAGE_SPEED_UNITS_POC_H
#define RAGE_SPEED_UNITS_POC_H
#include <stdint.h>

enum { RAGE_UNITS_AUTO, RAGE_UNITS_KMH, RAGE_UNITS_MPH };
/* Prototype: PAL AGE Erriso (0) and Lizard Esperanza (3), selected at car load.
 * Unsupported cars/regions keep both their retail label and retail numbers. */
void RageSpeedUnitsConfigure(int car, const char *region, const char *requested);
int RageSpeedUnits(void);
void RageSpeedUnitsPatchIndexed(uint16_t face[24 * 94], const uint16_t palette[16]);
void RageSpeedUnitsPatchRGBA(const char *key, uint8_t *pixels, unsigned w, unsigned h);
#endif
