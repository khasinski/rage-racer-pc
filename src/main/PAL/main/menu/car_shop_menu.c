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
/*
 * The nearest car the player does not already own, walking `step` at a time.
 * -1 when there is none.
 */
static s32 FindCarNotOwned(s32 from, s32 step) {
    s32 index;

    for (index = from; index >= 0 && index < GAME_CAR_COUNT; index += step) {
        if (g_CarTable[index].enabled == 0) {
            return index;
        }
    }
    return -1;
}

/*
 * The same walk, but only over cars the player's progress has reached. Below
 * the last class the class above counts as reached, so the shop shows what is
 * coming next; in the last class it does not.
 */
static s32 FindCarOnOffer(s32 from, s32 step) {
    s32 progress = g_RaceProgress->maxClassReached;
    s32 index;

    for (index = from; index >= 0 && index < GAME_CAR_COUNT; index += step) {
        s32 unlockLevel = GetCarUnlockLevel(index);

        if (g_CarTable[index].enabled != 0) {
            continue;
        }
        if (progress < 4 ? progress + 1 >= unlockLevel
                         : progress >= unlockLevel) {
            return index;
        }
    }
    return -1;
}

void UpdateCarListCursor(void) {
    if (g_CarShopUnlockAll != 0) {
        g_PrevOwnedCarIndex = FindCarNotOwned(g_CarListCursor - 1, -1);
        g_NextOwnedCarIndex = FindCarNotOwned(g_CarListCursor + 1, 1);
    } else {
        g_PrevOwnedCarIndex = FindCarOnOffer(g_CarListCursor - 1, -1);
        g_NextOwnedCarIndex = FindCarOnOffer(g_CarListCursor + 1, 1);
    }
}
