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
s32 g_CarShopScreenProgress;
s32 g_CarShopUnlockAll;
s16 g_NextOwnedCarIndex;
s16 g_PrevOwnedCarIndex;
s32 g_ShopCarIndex;
static GameRaceProgress s_progress;
GameRaceProgress *g_RaceProgress = &s_progress;

static s32 s_engineStep;
static s32 s_enginePhase;
static s32 s_unlockLevelOverride = INT_MIN;

void DrawCarEngineSpec(s32 step, s32 phase) {
    s_engineStep = step;
    s_enginePhase = phase;
}

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
    g_CarShopScreenProgress = 100;
    CHECK(DrawCarShopScreen(0) == 0);
    CHECK(DrawCarShopScreen(600) == MENU_FADE_MAX);
    CHECK(s_engineStep == 0);
    CHECK(s_enginePhase == MENU_FADE_MAX / 4);
    CHECK(DrawCarShopScreen(-MENU_FADE_MAX) == 0);
    CHECK(s_engineStep == MENU_FADE_MAX * MENU_FADE_MAX / 2048);
    CHECK(s_enginePhase == 0);

    g_CarShopScreenProgress = INT_MAX;
    CHECK(DrawCarShopScreen(-1) == MENU_FADE_MAX);
    CHECK(s_engineStep == 0 && s_enginePhase == MENU_FADE_MAX / 4);
    g_CarShopScreenProgress = INT_MIN;
    CHECK(DrawCarShopScreen(1) == 0);
    CHECK(s_enginePhase == 0);
    CHECK(AdvanceCarSpecPanel(NULL, 1) == 0);

    memset(s_cars, 0, sizeof(s_cars));
    s_cars[2].enabled = 1;
    s_cars[5].enabled = 1;
    g_CarShopUnlockAll = 1;
    g_CarListCursor = 4;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == 3);
    CHECK(g_NextOwnedCarIndex == 6);

    memset(s_cars, 0, sizeof(s_cars));
    s_progress.maxClassReached = 1;
    g_CarShopUnlockAll = 0;
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
    g_CarShopUnlockAll = 1;
    g_CarListCursor = 0;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == -1);
    CHECK(g_NextOwnedCarIndex == -1);

    memset(s_cars, 0, sizeof(s_cars));
    g_CarShopUnlockAll = 1;
    RefreshCarUnlockState();
    CHECK(g_ShopCarIndex == 0);
    s_cars[0].enabled = 1;
    RefreshCarUnlockState();
    CHECK(g_ShopCarIndex == 1);

    g_CarShopUnlockAll = 0;
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

    g_CarShopUnlockAll = 1;
    g_CarListCursor = INT_MIN;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == -1 && g_NextOwnedCarIndex == -1);
    g_CarListCursor = INT_MAX;
    UpdateCarListCursor();
    CHECK(g_PrevOwnedCarIndex == -1 && g_NextOwnedCarIndex == -1);

    g_CarShopUnlockAll = 0;
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
