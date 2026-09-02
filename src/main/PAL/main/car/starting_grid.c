#include "game/car.h"
#include "game/race.h"
#include "game/state.h"

enum {
    RACE_SCENE_ID = 11,
    RIVAL_CAR_COUNT = 11,
};

void BuildStartingGrid(void) {
    RaceGridSlot *grid =
        g_SceneId == RACE_SCENE_ID ? g_RaceGridSlots : g_AttractGridSlots;
    s32 index;

    g_ClosestRivalRank = 3;
    g_RaceSeries = g_GrandPrixSeries;

    for (index = 0; index < RIVAL_CAR_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        car->activeFlag = 0;
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
