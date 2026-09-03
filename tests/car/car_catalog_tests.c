#include "game/asset.h"
#include "game/car.h"

#include <stdio.h>

static CarEntry s_cars[GAME_CAR_COUNT];
CarEntry *g_CarTable = s_cars;
u8 g_CarModelBaseIndex[GAME_CAR_COUNT] = {
    0, 4, 7, 9, 14, 18, 21, 23, 26, 28, 29, 30, 31,
};
u8 g_CarModelUnlockBase[GAME_CAR_COUNT] = {
    1, 2, 3, 0, 1, 2, 3, 2, 3, 4, 5, 5, 5,
};

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    s32 model;

    for (model = 0; model < 9; model++) {
        s32 variantCount = g_CarModelBaseIndex[model + 1] -
                           g_CarModelBaseIndex[model];
        s32 variant;

        for (variant = 0; variant < variantCount; variant++) {
            g_CarTable[model].modelVariant = (u8)variant;
            CHECK(GetOwnedCarAssetIndex(model) ==
                  g_CarModelBaseIndex[model] + variant);
            CHECK(GetCarUnlockLevel(model) ==
                  g_CarModelUnlockBase[model] + variant);
        }
    }

    g_CarTable[0].modelVariant = 4;
    CHECK(GetOwnedCarAssetIndex(0) == -1);
    g_CarTable[8].modelVariant = 0xFF;
    CHECK(GetOwnedCarAssetIndex(8) == 28);

    for (model = 9; model < GAME_CAR_COUNT; model++) {
        g_CarTable[model].modelVariant = 0xFF;
        CHECK(GetOwnedCarAssetIndex(model) == g_CarModelBaseIndex[model]);
    }

    CHECK(GetOwnedCarAssetIndex(-1) == -1);
    CHECK(GetOwnedCarAssetIndex(GAME_CAR_COUNT) == -1);
    CHECK(GetCarAssetIndex(-1, 0) == -1);
    CHECK(GetCarAssetIndex(GAME_CAR_COUNT, 0) == -1);
    CHECK(GetCarAssetIndex(0, -1) == -1);
    CHECK(GetCarAssetIndex(0, CAR_MODEL_VARIANT_COUNT - 1) ==
          CAR_MODEL_VARIANT_COUNT - 1);
    CHECK(GetCarAssetIndex(0, CAR_MODEL_VARIANT_COUNT) == -1);
    CHECK(GetCarAssetIndex(8, 2) == 28);
    CHECK(GetCarAssetIndex(GAME_CAR_COUNT - 1, 0) ==
          CAR_MODEL_VARIANT_COUNT - 1);
    CHECK(GetCarAssetIndex(GAME_CAR_COUNT - 1, 1) == -1);
    CHECK(GetCarUnlockLevel(-1) == -1);
    CHECK(GetCarUnlockLevel(GAME_CAR_COUNT) == -1);

    g_CarTable = NULL;
    CHECK(GetCarUnlockLevel(0) == -1);
    CHECK(GetOwnedCarAssetIndex(0) == -1);
    CHECK(GetOwnedCarAssetIndex(9) == g_CarModelBaseIndex[9]);

    puts("car catalog tests passed");
    return 0;
}
