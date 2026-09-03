#include "game/car.h"

enum {
    LIMITED_VARIANT_MODEL = 8,
    LIMITED_VARIANT_MAXIMUM = 2,
    FIRST_FIXED_ASSET_MODEL = 9,
};

s32 GetCarAssetIndex(s32 model, s32 grade) {
    s32 firstVariant;

    if ((u32)model >= GAME_CAR_COUNT || grade < 0) {
        return -1;
    }

    firstVariant = g_CarModelBaseIndex[model];
    if (grade >= CAR_MODEL_VARIANT_COUNT - firstVariant) {
        return -1;
    }
    return firstVariant + grade;
}

s32 GetCarUnlockLevel(s32 model) {
    if ((u32)model >= GAME_CAR_COUNT || g_CarTable == NULL) {
        return -1;
    }
    return g_CarTable[model].modelVariant + g_CarModelUnlockBase[model];
}

s32 GetOwnedCarAssetIndex(s32 model) {
    s32 variant;

    if ((u32)model >= GAME_CAR_COUNT) {
        return -1;
    }
    if (model >= FIRST_FIXED_ASSET_MODEL) {
        return g_CarModelBaseIndex[model];
    }
    if (g_CarTable == NULL) {
        return -1;
    }

    variant = g_CarTable[model].modelVariant;
    if (model == LIMITED_VARIANT_MODEL) {
        if (variant > LIMITED_VARIANT_MAXIMUM) {
            variant = LIMITED_VARIANT_MAXIMUM;
        }
        return GetCarAssetIndex(model, variant);
    }
    if (variant >= g_CarModelBaseIndex[model + 1] -
                       g_CarModelBaseIndex[model]) {
        return -1;
    }
    return GetCarAssetIndex(model, variant);
}
