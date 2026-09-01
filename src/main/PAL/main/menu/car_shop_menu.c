#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"

s32 DrawCarShopScreen(s32 step) {
    s32 value;
    s32 limit;
    s32 amount;
    s32 phase;

    if (step == 0) {
        g_CarShopScreenProgress = 0;
        return 0;
    }

    if (step > 0) {
        value = g_CarShopScreenProgress + step;
        g_CarShopScreenProgress = value;
        if (value >= 0x1FD) {
            g_CarShopScreenProgress = 0x1FC;
        }
        value = 0;
    } else {
        u32 product;

        value = g_CarShopScreenProgress + step;
        g_CarShopScreenProgress = value;
        if (value < 0) {
            g_CarShopScreenProgress = 0;
        }

        value = g_CarShopScreenProgress;
        limit = 0x1FC;
        limit -= value;
        product = limit * limit;
        value = product >> 0xB;
    }

    amount = value << 16;
    amount >>= 16;
    phase = (u8)(g_CarShopScreenProgress / 4U);
    DrawCarEngineSpec(amount, phase);

    return g_CarShopScreenProgress;
}
/*
 * The nearest car the player does not already own, walking `step` at a time.
 * -1 when there is none.
 */
static s32 FindCarNotOwned(s32 from, s32 step) {
    s32 index;

    for (index = from; index >= 0 && index < 13; index += step) {
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
    s32 index;

    for (index = from; index >= 0 && index < 13; index += step) {
        s32 unlockLevel = GetCarUnlockLevel(index);
        s32 progress;

        if (g_CarTable[index].enabled != 0) {
            continue;
        }
        progress = g_RaceProgress->maxClassReached;
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

