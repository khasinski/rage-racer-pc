#ifndef RAGE_HUD_UNIT_DIFF_H
#define RAGE_HUD_UNIT_DIFF_H
#include <stddef.h>
#include <stdint.h>

enum {
    RAGE_HUD_FACE_BYTES = 24 * 94 * 2,
    RAGE_HUD_UNIT_BYTES = RAGE_HUD_FACE_BYTES + 2 * 16 * 2,
    RAGE_HUD_DIFF_MAX_BYTES = 139 + RAGE_HUD_UNIT_BYTES * 3
};
typedef struct RageHudUnitDiff {
    unsigned car;
    char metricHash[65], imperialHash[65];
    uint8_t delta[RAGE_HUD_UNIT_BYTES];
} RageHudUnitDiff;
/* Fixed PAL-observed page-10 face and its two CLUTs. The reference capture
 * must confirm this layout on the other region before generating a patch. */
int RageHudUnitSnapshot(const uint8_t *vram, size_t size,
                        uint8_t snapshot[RAGE_HUD_UNIT_BYTES]);
int RageHudUnitDiffBuild(RageHudUnitDiff *out, unsigned car,
                        const uint8_t metric[RAGE_HUD_UNIT_BYTES],
                        const uint8_t imperial[RAGE_HUD_UNIT_BYTES]);
size_t RageHudUnitDiffEncode(const RageHudUnitDiff *diff, uint8_t *out, size_t capacity);
int RageHudUnitDiffDecode(RageHudUnitDiff *out, const uint8_t *bytes, size_t size);
/* 0 = rejected (snapshot untouched), 1 = already desired, 2 = converted.
 * Hashes bind both the original face and both palettes, in either direction. */
int RageHudUnitDiffApply(const RageHudUnitDiff *diff, unsigned car, int imperial,
                        uint8_t snapshot[RAGE_HUD_UNIT_BYTES]);
#endif
