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
    CarBrowse browse = {0};

    memset(s_cars, 0, sizeof(s_cars));
    s_cars[2].enabled = 1;
    s_cars[5].enabled = 1;
    s_progress.maxClassReached = GAME_CAR_COUNT;
    browse.cursor = 4;
    UpdateCarListCursor(&browse);
    CHECK(browse.previous == 3);
    CHECK(browse.next == 6);

    memset(s_cars, 0, sizeof(s_cars));
    s_progress.maxClassReached = 1;
    browse.cursor = 3;
    UpdateCarListCursor(&browse);
    CHECK(browse.previous == 2);
    CHECK(browse.next == -1);

    s_progress.maxClassReached = 4;
    browse.cursor = 4;
    UpdateCarListCursor(&browse);
    CHECK(browse.previous == 3);
    CHECK(browse.next == -1);

    memset(s_cars, 1, sizeof(s_cars));
    browse.cursor = 0;
    UpdateCarListCursor(&browse);
    CHECK(browse.previous == -1);
    CHECK(browse.next == -1);

    memset(s_cars, 0, sizeof(s_cars));
    s_progress.maxClassReached = GAME_CAR_COUNT;
    RefreshCarUnlockState(&browse);
    CHECK(browse.shopIndex == 0);
    s_cars[0].enabled = 1;
    RefreshCarUnlockState(&browse);
    CHECK(browse.shopIndex == 1);

    s_unlockLevelOverride = -1;
    RefreshCarUnlockState(&browse);
    CHECK(browse.shopIndex == -1);
    s_unlockLevelOverride = INT_MIN;

    g_RaceProgress = NULL;
    RefreshCarUnlockState(&browse);
    UpdateCarListCursor(&browse);
    CHECK(browse.shopIndex == -1 && browse.previous == -1 &&
          browse.next == -1);
    g_RaceProgress = &s_progress;

    g_CarTable = NULL;
    RefreshCarUnlockState(&browse);
    UpdateCarListCursor(&browse);
    CHECK(browse.shopIndex == -1 && browse.previous == -1 &&
          browse.next == -1);
    g_CarTable = s_cars;

    browse.cursor = INT_MIN;
    UpdateCarListCursor(&browse);
    CHECK(browse.previous == -1 && browse.next == -1);
    browse.cursor = INT_MAX;
    UpdateCarListCursor(&browse);
    CHECK(browse.previous == -1 && browse.next == -1);

    s_progress.maxClassReached = 1;
    s_cars[1].enabled = 1;
    RefreshCarUnlockState(&browse);
    CHECK(browse.shopIndex == 2);

    memset(s_cars, 1, sizeof(s_cars));
    RefreshCarUnlockState(&browse);
    CHECK(browse.shopIndex == -1);

    puts("car shop menu tests passed");
    return 0;
}
