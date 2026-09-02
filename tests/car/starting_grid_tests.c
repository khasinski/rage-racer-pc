#include "game/car.h"
#include "game/race.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[11];
RaceGridSlot g_RaceGridSlots[11];
RaceGridSlot g_AttractGridSlots[11];
s32 g_ClosestRivalRank;
s32 g_RaceSeries;
s16 g_GrandPrixSeries;
s32 g_SceneId;

static s32 s_clearCalls[11];
static s32 s_initCalls[11];
static s32 s_aiCalls[11];
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
    for (index = 0; index < 11; index++) {
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
    for (index = 0; index < 11; index++) {
        s32 active = g_RaceGridSlots[index].value >= 0;

        CHECK(g_Cars[index].activeFlag == active);
        CHECK(g_Cars[index].aiEnabled == active);
        CHECK(g_Cars[index].facingBackwards == 1);
        CHECK(s_clearCalls[index] == active);
        CHECK(s_initCalls[index] == active);
        CHECK(s_aiCalls[index] == active);
    }

    puts("starting grid initialization tests passed");
    return 0;
}
