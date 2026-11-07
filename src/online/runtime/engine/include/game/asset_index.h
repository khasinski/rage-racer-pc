#ifndef GAME_ASSET_INDEX_H
#define GAME_ASSET_INDEX_H
#include "common.h"

/*
 * Index of the first entry of each variable-size family in that table. Read off
 * the retail path table and cross-checked against the 135-entry RAGE.BIN index
 * on the PAL disc.
 *
 * CAR_1ST / CAR_2ND: [0x0A] starts the model/image pair for the first car
 * variant; every following variant occupies the next pair.
 *
 * ROUND_SCREEN: [0x4A] = "\DATA\GP0.TMS". Six screens per series, the sixth
 * being GP10 / GP11, so LoadGrandPrixScreen wants base + series * 6 + class.
 *
 * TRACK_1ST / TRACK_2ND: [0x57] = "\PACK\BIG1.1ST", [0x58] its ".2ND" sibling.
 * Four courses (BIG, MID, HI, OVAL) x two packs = eight entries per class, so
 * both are indexed base + class * 8 + course-slot * 2. The slot must be 0..3,
 * not the menu's physical 0..7 selector. Six classes fill [0x57..0x86], which
 * is exactly the end of the table.
 */
#define ASSET_BOOT_CAR_SCREEN 5
#define ASSET_CAR_1ST_BASE      0x0A
#define ASSET_CAR_2ND_BASE      0x0B
#define ASSET_ROUND_SCREEN_BASE 0x4A
#define ASSET_TIME_ATTACK_ROUND_SCREEN 0x55
#define ASSET_VOICE_BANK        0x56
#define ASSET_TRACK_1ST_BASE    0x57
#define ASSET_TRACK_2ND_BASE    0x58

enum {
    GAME_ASSET_COUNT = 135,
    CAR_ASSETS_PER_VARIANT = 2,
    TRACK_CLASS_COUNT = 6,
    TRACK_COURSE_COUNT = 4,
    TRACK_ASSETS_PER_CLASS = 8,
    TRACK_ASSETS_PER_COURSE = 2,
};

static inline s32 CarVariantAssetIndex(s32 base, s32 variantIndex) {
    return base + variantIndex * CAR_ASSETS_PER_VARIANT;
}

static inline s32 TrackCourseAssetIndex(s32 base, s32 classIndex,
                                        s32 courseIndex) {
    if ((u32)classIndex >= TRACK_CLASS_COUNT ||
        (u32)courseIndex >= TRACK_COURSE_COUNT) {
        return -1;
    }
    return base + classIndex * TRACK_ASSETS_PER_CLASS +
           courseIndex * TRACK_ASSETS_PER_COURSE;
}

#endif
