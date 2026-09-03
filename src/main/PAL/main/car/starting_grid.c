#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <string.h>

enum {
    RACE_SCENE_ID = 11,
};

static void DisableRivalCar(GameCarRuntime *car) {
    memset(car, 0, sizeof(*car));
    car->activeFlag = -1;
    car->facingBackwards = (s16)g_RaceSeries;
}

void BuildStartingGrid(void) {
    const RaceGridSlot *grid =
        g_SceneId == RACE_SCENE_ID ? g_RaceGridSlots : g_AttractGridSlots;
    s32 index;

    g_ClosestRivalRank = 3;
    g_RaceSeries = g_GrandPrixSeries & (TRACK_SERIES_COUNT - 1);

    if (g_TrackEventData == NULL || g_TrackPoints == NULL ||
        g_TrackPointCount <= 0) {
        for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
            DisableRivalCar(&g_Cars[index]);
        }
        return;
    }

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (grid[index].value < 0) {
            DisableRivalCar(car);
            continue;
        }

        InitRivalCar(car, index, grid);
        if (car->activeFlag != -1) {
            InitRivalCarAi(car, index, grid);
        }
    }

    SeedCarRouteMarkers();
}
