#include "game/asset_catalog.h"

int AssetIdIsValid(AssetId assetId) {
    return assetId >= 0 && assetId < ASSET_ID_COUNT;
}

AssetId AssetRoundScreenId(s32 series, s32 classIndex) {
    if (series < 0 || series >= ASSET_SERIES_COUNT ||
        classIndex < 0 || classIndex >= ASSET_CLASS_COUNT) {
        return ASSET_ID_INVALID;
    }
    return ASSET_ROUND_SCREEN_BASE + series * ASSET_CLASS_COUNT + classIndex;
}

AssetId AssetTrackPackId(s32 classIndex, s32 physicalCourseIndex, s32 secondPack) {
    AssetId assetId;

    if (classIndex < 0 || classIndex >= ASSET_CLASS_COUNT ||
        physicalCourseIndex < 0 ||
        physicalCourseIndex >= ASSET_PHYSICAL_COURSE_COUNT ||
        (secondPack != 0 && secondPack != 1)) {
        return ASSET_ID_INVALID;
    }
    assetId = ASSET_TRACK_1ST_BASE + classIndex * (ASSET_COURSE_COUNT * 2) +
              physicalCourseIndex * 2 + secondPack;
    return AssetIdIsValid(assetId) ? assetId : ASSET_ID_INVALID;
}
