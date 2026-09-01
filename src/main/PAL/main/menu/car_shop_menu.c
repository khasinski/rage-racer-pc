#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"

s32 DrawCarShopScreen(s32 step) {
    s32 engineSpecStep = 0;

    if (step == 0) {
        g_CarShopScreenProgress = 0;
        return 0;
    }

    g_CarShopScreenProgress += step;
    if (step > 0) {
        if (g_CarShopScreenProgress >= MENU_FADE_COMPLETE) {
            g_CarShopScreenProgress = MENU_FADE_MAX;
        }
    } else {
        s32 fadeRemaining;

        if (g_CarShopScreenProgress < 0) {
            g_CarShopScreenProgress = 0;
        }
        fadeRemaining = MENU_FADE_MAX - g_CarShopScreenProgress;
        engineSpecStep = fadeRemaining * fadeRemaining / 2048;
    }

    DrawCarEngineSpec(engineSpecStep, (u8)(g_CarShopScreenProgress / 4U));

    return g_CarShopScreenProgress;
}
/* Below the last class, the shop previews cars from the class coming next. */
static s32 IsCarOnOffer(s32 index) {
    s32 progress = g_RaceProgress->maxClassReached;
    s32 unlockLevel;

    if (g_CarTable[index].enabled != 0) {
        return 0;
    }
    if (g_CarShopUnlockAll != 0) {
        return 1;
    }

    unlockLevel = GetCarUnlockLevel(index);
    return progress < 4 ? progress + 1 >= unlockLevel
                        : progress >= unlockLevel;
}

static s32 FindCarOnOffer(s32 from, s32 step) {
    s32 index;

    for (index = from; index >= 0 && index < GAME_CAR_COUNT; index += step) {
        if (IsCarOnOffer(index)) {
            return index;
        }
    }
    return -1;
}

void RefreshCarUnlockState(void) {
    g_ShopCarIndex = FindCarOnOffer(0, 1);
}

void UpdateCarListCursor(void) {
    g_PrevOwnedCarIndex = FindCarOnOffer(g_CarListCursor - 1, -1);
    g_NextOwnedCarIndex = FindCarOnOffer(g_CarListCursor + 1, 1);
}
