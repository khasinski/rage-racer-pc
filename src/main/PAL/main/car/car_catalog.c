#include "game/asset.h"
#include "game/car.h"

s32 GetCarAssetIndex(s32 model, s32 grade) {
    return g_CarModelBaseIndex[model] + grade;
}

s32 GetCarUnlockLevel(s32 model) {
    return g_CarTable[model].modelVariant + g_CarModelUnlockBase[model];
}
