#include "common.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/race.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static CarEntry s_cars[GAME_CAR_COUNT];
CarEntry *g_CarTable = s_cars;
s32 g_CarListCursor;
s16 g_NextOwnedCarIndex;
s16 g_PrevOwnedCarIndex;
s32 g_ShopCarIndex;
static GameRaceProgress s_progress;
GameRaceProgress *g_RaceProgress = &s_progress;

static s32 s_unlockLevelOverride = INT_MIN;

s32 GetCarUnlockLevel(s32 carIndex) {
    return s_unlockLevelOverride != INT_MIN ? s_unlockLevelOverride : carIndex;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    memset(s_cars, 0, sizeof(s_cars));
    s_cars[2].enabled = 1;
    s_cars[5].enabled = 1;
    s_progress.maxClassReached = GAME_CAR_COUNT;
    g_CarListCursor = 4;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == 3);
    CHECK(g_NextOwnedCarIndex == 6);

    memset(s_cars, 0, sizeof(s_cars));
    s_progress.maxClassReached = 1;
    g_CarListCursor = 3;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == 2);
    CHECK(g_NextOwnedCarIndex == -1);

    s_progress.maxClassReached = 4;
    g_CarListCursor = 4;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == 3);
    CHECK(g_NextOwnedCarIndex == -1);

    memset(s_cars, 1, sizeof(s_cars));
    g_CarListCursor = 0;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == -1);
    CHECK(g_NextOwnedCarIndex == -1);

    memset(s_cars, 0, sizeof(s_cars));
    s_progress.maxClassReached = GAME_CAR_COUNT;
    RefreshCarUnlockState();
    CHECK(g_ShopCarIndex == 0);
    s_cars[0].enabled = 1;
    RefreshCarUnlockState();
    CHECK(g_ShopCarIndex == 1);

    s_unlockLevelOverride = -1;
    RefreshCarUnlockState();
    CHECK(g_ShopCarIndex == -1);
    s_unlockLevelOverride = INT_MIN;

    g_RaceProgress = NULL;
    RefreshCarUnlockState();
    UpdateCarListCursor();
    CHECK(g_ShopCarIndex == -1 && g_PrevOwnedCarIndex == -1 &&
          g_NextOwnedCarIndex == -1);
    g_RaceProgress = &s_progress;

    g_CarTable = NULL;
    RefreshCarUnlockState();
    UpdateCarListCursor();
    CHECK(g_ShopCarIndex == -1 && g_PrevOwnedCarIndex == -1 &&
          g_NextOwnedCarIndex == -1);
    g_CarTable = s_cars;

    g_CarListCursor = INT_MIN;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == -1 && g_NextOwnedCarIndex == -1);
    g_CarListCursor = INT_MAX;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == -1 && g_NextOwnedCarIndex == -1);

    s_progress.maxClassReached = 1;
    s_cars[1].enabled = 1;
    RefreshCarUnlockState();
    CHECK(g_ShopCarIndex == 2);

    memset(s_cars, 1, sizeof(s_cars));
    RefreshCarUnlockState();
    CHECK(g_ShopCarIndex == -1);

    puts("car shop menu tests passed");
    return 0;
}
