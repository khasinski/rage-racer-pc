#include "game/car.h"
#include "game/race.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
RaceGridSlot g_RaceGridSlots[RACE_CAR_SLOT_COUNT];
RaceGridSlot g_AttractGridSlots[RACE_CAR_SLOT_COUNT];
s32 g_ClosestRivalRank;
s32 g_RaceSeries;
s16 g_GrandPrixSeries;
s32 g_SceneId;

static s32 s_clearCalls[RACE_CAR_SLOT_COUNT];
static s32 s_initCalls[RACE_CAR_SLOT_COUNT];
static s32 s_aiCalls[RACE_CAR_SLOT_COUNT];
static s32 s_routeSeedCalls;
static RaceGridSlot *s_expectedGrid;

static s32 CarIndex(GameCarRuntime *car) {
    return (s32)(car - g_Cars);
}

void ClearCarMotionState(GameCarRuntime *car) {
    s_clearCalls[CarIndex(car)]++;
}

void InitRivalCar(GameCarRuntime *car, s32 index, RaceGridSlot *grid) {
    if (grid == s_expectedGrid && index == CarIndex(car) &&
        car->activeFlag == 1) {
        s_initCalls[index]++;
        car->aiEnabled = 1;
    }
}

void InitRivalCarAi(GameCarRuntime *car, s32 index, RaceGridSlot *grid) {
    if (grid == s_expectedGrid && index == CarIndex(car)) {
        s_aiCalls[index]++;
    }
}

void SeedCarRouteMarkers(void) {
    s_routeSeedCalls++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    s32 index;

    memset(g_Cars, 0x5A, sizeof(g_Cars));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].aiEnabled = 1;
        g_RaceGridSlots[index].value = index % 3 == 0 ? -1 : index;
        g_AttractGridSlots[index].value = index;
    }
    g_GrandPrixSeries = 1;
    g_SceneId = 11;
    s_expectedGrid = g_RaceGridSlots;

    BuildStartingGrid();

    CHECK(g_ClosestRivalRank == 3 && g_RaceSeries == 1);
    CHECK(s_routeSeedCalls == 1);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 active = g_RaceGridSlots[index].value >= 0;

        CHECK(g_Cars[index].activeFlag == (active ? 1 : -1));
        CHECK(g_Cars[index].aiEnabled == active);
        CHECK(g_Cars[index].facingBackwards == 1);
        CHECK(s_clearCalls[index] == active);
        CHECK(s_initCalls[index] == active);
        CHECK(s_aiCalls[index] == active);
    }

    memset(s_clearCalls, 0, sizeof(s_clearCalls));
    memset(s_initCalls, 0, sizeof(s_initCalls));
    memset(s_aiCalls, 0, sizeof(s_aiCalls));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_RaceGridSlots[index].value = -1;
        g_AttractGridSlots[index].value = index == 4 ? 4 : -1;
    }
    g_GrandPrixSeries = 0;
    g_SceneId = 3;
    s_expectedGrid = g_AttractGridSlots;

    BuildStartingGrid();

    CHECK(g_RaceSeries == 0);
    CHECK(s_routeSeedCalls == 2);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 active = index == 4;

        CHECK(g_Cars[index].activeFlag == (active ? 1 : -1));
        CHECK(g_Cars[index].aiEnabled == active);
        CHECK(g_Cars[index].facingBackwards == 0);
        CHECK(s_clearCalls[index] == active);
        CHECK(s_initCalls[index] == active);
        CHECK(s_aiCalls[index] == active);
    }

    puts("starting grid initialization tests passed");
    return 0;
}
