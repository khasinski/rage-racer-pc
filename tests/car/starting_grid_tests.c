#include "game/car.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
RaceGridSlot g_RaceGridSlots[RACE_GRID_STORAGE_COUNT];
RaceGridSlot g_AttractGridSlots[RACE_GRID_STORAGE_COUNT];
s32 g_ClosestRivalRank;
s32 g_RaceSeries;
s16 g_GrandPrixSeries;
s32 g_SceneId;
s32 g_TrackPointCount;
const GameTrackPoint *g_TrackPoints;
const TrackEventData *g_TrackEventData;

static s32 s_initCalls[RACE_CAR_SLOT_COUNT];
static s32 s_aiCalls[RACE_CAR_SLOT_COUNT];
static s32 s_routeSeedCalls;
static const RaceGridSlot *s_expectedGrid;

static s32 CarIndex(GameCarRuntime *car) {
    return (s32)(car - g_Cars);
}

void InitRivalCar(GameCarRuntime *car, s32 index,
                  const RaceGridSlot *grid) {
    if (grid == s_expectedGrid && index == CarIndex(car)) {
        s_initCalls[index]++;
        car->activeFlag = 1;
        car->aiEnabled = 1;
        car->facingBackwards = (s16)g_RaceSeries;
    }
}

void InitRivalCarAi(GameCarRuntime *car, s32 index,
                    const RaceGridSlot *grid) {
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
    static GameTrackPoint trackPoint;
    static TrackEventData trackEvents;
    s32 index;

    memset(g_Cars, 0x5A, sizeof(g_Cars));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].aiEnabled = 1;
        g_RaceGridSlots[index].value = index % 3 == 0 ? -1 : index;
        g_AttractGridSlots[index].value = index;
    }
    g_GrandPrixSeries = 1;
    g_SceneId = 11;
    g_TrackPointCount = 1;
    g_TrackPoints = &trackPoint;
    g_TrackEventData = &trackEvents;
    s_expectedGrid = g_RaceGridSlots;

    BuildStartingGrid();

    CHECK(g_ClosestRivalRank == 3 && g_RaceSeries == 1);
    CHECK(s_routeSeedCalls == 1);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 active = g_RaceGridSlots[index].value >= 0;

        CHECK(g_Cars[index].activeFlag == (active ? 1 : -1));
        CHECK(g_Cars[index].aiEnabled == active);
        CHECK(g_Cars[index].facingBackwards == 1);
        if (!active) {
            CHECK(g_Cars[index].reservedCC == 0);
            CHECK(g_Cars[index].motionX == 0);
        }
        CHECK(s_initCalls[index] == active);
        CHECK(s_aiCalls[index] == active);
    }

    g_TrackEventData = NULL;
    BuildStartingGrid();
    CHECK(s_routeSeedCalls == 1);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        CHECK(g_Cars[index].activeFlag == -1);
        CHECK(g_Cars[index].aiEnabled == 0);
    }

    memset(s_initCalls, 0, sizeof(s_initCalls));
    memset(s_aiCalls, 0, sizeof(s_aiCalls));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_RaceGridSlots[index].value = -1;
        g_AttractGridSlots[index].value = index == 4 ? 4 : -1;
    }
    g_GrandPrixSeries = 0;
    g_SceneId = 3;
    g_TrackEventData = &trackEvents;
    s_expectedGrid = g_AttractGridSlots;

    BuildStartingGrid();

    CHECK(g_RaceSeries == 0);
    CHECK(s_routeSeedCalls == 2);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 active = index == 4;

        CHECK(g_Cars[index].activeFlag == (active ? 1 : -1));
        CHECK(g_Cars[index].aiEnabled == active);
        CHECK(g_Cars[index].facingBackwards == 0);
        if (!active) {
            CHECK(g_Cars[index].reservedCC == 0);
            CHECK(g_Cars[index].motionX == 0);
        }
        CHECK(s_initCalls[index] == active);
        CHECK(s_aiCalls[index] == active);
    }

    memset(s_initCalls, 0, sizeof(s_initCalls));
    memset(s_aiCalls, 0, sizeof(s_aiCalls));
    g_GrandPrixSeries = 3;
    g_SceneId = 11;
    g_RaceGridSlots[4].value = 4;
    s_expectedGrid = g_RaceGridSlots;
    BuildStartingGrid();
    CHECK(g_RaceSeries == 1);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 active = g_RaceGridSlots[index].value >= 0;

        CHECK(s_initCalls[index] == active);
        CHECK(s_aiCalls[index] == active);
        CHECK(g_Cars[index].facingBackwards == 1);
    }

    puts("starting grid initialization tests passed");
    return 0;
}
