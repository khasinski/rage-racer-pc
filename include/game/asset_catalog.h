#ifndef GAME_ASSET_CATALOG_H
#define GAME_ASSET_CATALOG_H

#include "common.h"

typedef s32 AssetId;

enum {
    ASSET_ID_INVALID = -1,
    ASSET_ID_BOOT = 0,
    ASSET_ROUND_SCREEN_BASE = 0x4A,
    ASSET_TIME_ATTACK_ROUND_SCREEN = 0x55,
    ASSET_VOICE_BANK = 0x56,
    ASSET_TRACK_1ST_BASE = 0x57,
    ASSET_TRACK_2ND_BASE = 0x58,
    ASSET_ID_COUNT = 135,
    ASSET_STREAM_COUNT = 11,
    ASSET_SERIES_COUNT = 2,
    ASSET_CLASS_COUNT = 6,
    ASSET_COURSE_COUNT = 4,
    ASSET_PHYSICAL_COURSE_COUNT = ASSET_SERIES_COUNT * ASSET_COURSE_COUNT
};

int AssetIdIsValid(AssetId assetId);
AssetId AssetRoundScreenId(s32 series, s32 classIndex);
AssetId AssetTrackPackId(s32 classIndex, s32 physicalCourseIndex, s32 secondPack);

#endif
