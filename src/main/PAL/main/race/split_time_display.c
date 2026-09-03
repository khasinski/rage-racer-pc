#include "game/race_hud_internal.h"

enum {
    SPLIT_SECTOR_COUNT = 3,
    SPLIT_DISPLAY_FRAMES = 60,
    MAX_DISPLAY_TIME_MS = 599998,
    SPLIT_AHEAD_CLUT = 0x7810,
    SPLIT_BEHIND_CLUT = 0x780F,
    SPLIT_TIME_CLUT = 0x78CC,
    SPLIT_TIME_OVERFLOW_CLUT = 0x7890,
};

s32 SplitCurrentTimeVisible(s32 timer, s32 sectorIndex) {
    return timer >= 0 && timer < SPLIT_DISPLAY_FRAMES && sectorIndex >= 0 &&
           sectorIndex < SPLIT_SECTOR_COUNT;
}

s32 SplitDeltaVisible(s32 timer, s32 sectorIndex, s32 sign,
                      s32 lapCount, s32 playerLap) {
    return SplitCurrentTimeVisible(timer, sectorIndex) && sign != 0 &&
           lapCount >= playerLap;
}

s32 SplitDeltaClut(s32 sign) {
    return sign > 0 ? SPLIT_AHEAD_CLUT : SPLIT_BEHIND_CLUT;
}

s32 SplitTimeClut(s32 timeMs) {
    return timeMs >= 0 && timeMs <= MAX_DISPLAY_TIME_MS
               ? SPLIT_TIME_CLUT
               : SPLIT_TIME_OVERFLOW_CLUT;
}

s32 SplitDisplaySectorIndex(s32 sector) {
    return (u32)sector < SPLIT_SECTOR_COUNT ? sector : 0;
}
