#include "game/asset.h"
#include "game/car.h"

s32 GetCarAssetIndex(s32 model, s32 grade) {
    if ((u32)model >= GAME_CAR_COUNT) return -1;
    return g_CarModelBaseIndex[model] + grade;
}

s32 GetCarUnlockLevel(s32 model) {
    if ((u32)model >= GAME_CAR_COUNT) return -1;
    return g_CarTable[model].modelVariant + g_CarModelUnlockBase[model];
}

s32 GetOwnedCarAssetIndex(s32 model) {
    s32 variant;

    if ((u32)model >= GAME_CAR_COUNT) {
        return -1;
    }

    variant = g_CarTable[model].modelVariant;
    if (model >= 9) {
        return g_CarModelBaseIndex[model];
    }
    if (model == 8) {
        if (variant > 2) variant = 2;
        return GetCarAssetIndex(model, variant);
    }
    if (variant >= g_CarModelBaseIndex[model + 1] -
                       g_CarModelBaseIndex[model]) {
        return -1;
    }
    return GetCarAssetIndex(model, variant);
}
