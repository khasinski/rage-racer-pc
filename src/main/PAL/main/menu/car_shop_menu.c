#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"

s32 DrawCarShopScreen(s32 step) {
    return AdvanceCarSpecPanel(&g_CarShopScreenProgress, step);
}
/* Below the last class, the shop previews cars from the class coming next. */
static s32 IsCarOnOffer(s32 index) {
    s32 progress;
    s32 unlockLevel;

    if (g_CarTable == NULL || g_RaceProgress == NULL) {
        return 0;
    }
    if (g_CarTable[index].enabled != 0) {
        return 0;
    }
    if (g_CarShopUnlockAll != 0) {
        return 1;
    }

    unlockLevel = GetCarUnlockLevel(index);
    if (unlockLevel < 0) {
        return 0;
    }
    progress = g_RaceProgress->maxClassReached;
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

static s32 FindAdjacentCarOnOffer(s32 cursor, s32 step) {
    int64_t from = (int64_t)cursor + step;

    if (from < INT32_MIN || from > INT32_MAX) {
        return -1;
    }
    return FindCarOnOffer((s32)from, step);
}

void RefreshCarUnlockState(void) {
    g_ShopCarIndex = FindCarOnOffer(0, 1);
}

void UpdateCarListCursor(void) {
    g_PrevOwnedCarIndex = FindAdjacentCarOnOffer(g_CarListCursor, -1);
    g_NextOwnedCarIndex = FindAdjacentCarOnOffer(g_CarListCursor, 1);
}
