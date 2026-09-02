#include "game/car.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

enum {
    RACE_SCENE_ID = 11,
};

void BuildStartingGrid(void) {
    RaceGridSlot *grid =
        g_SceneId == RACE_SCENE_ID ? g_RaceGridSlots : g_AttractGridSlots;
    s32 index;

    g_ClosestRivalRank = 3;
    g_RaceSeries = g_GrandPrixSeries;

    if (g_TrackEventData == NULL || g_TrackPoints == NULL ||
        g_TrackPointCount <= 0) {
        for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
            g_Cars[index].activeFlag = -1;
            g_Cars[index].aiEnabled = 0;
        }
        return;
    }

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        car->activeFlag = -1;
        car->aiEnabled = 0;
        car->facingBackwards = (s16)ReadRaceTrackDirection();
        if (grid[index].value < 0) {
            continue;
        }

        ClearCarMotionState(car);
        car->activeFlag = 1;
        InitRivalCar(car, index, grid);
        InitRivalCarAi(car, index, grid);
    }

    SeedCarRouteMarkers();
}
