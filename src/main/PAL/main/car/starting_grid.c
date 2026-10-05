#include "game/car.h"
#include "game/car_internal.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

#include <string.h>

enum {
    RACE_SCENE_ID = 11,
};

s32 g_DuelEnabled = 0;
s32 g_DuelRaceActive = 0;
s32 g_DuelRivalCar = 0;

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
    g_DuelRaceActive = 0;
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

        if (grid[index] < 0) {
            DisableRivalCar(car);
            continue;
        }

        InitRivalCar(car, index, grid);
        if (car->activeFlag != -1) {
            InitRivalCarAi(car, index, grid);
        }
    }

    if (g_DuelEnabled && g_SceneId == RACE_SCENE_ID &&
        (u32)g_DuelRivalCar < RACE_CAR_SLOT_COUNT &&
        g_Cars[g_DuelRivalCar].activeFlag != -1) {
        for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
            if (index != g_DuelRivalCar) {
                DisableRivalCar(&g_Cars[index]);
            }
        }
        g_PlayerCar.drive.racePosition = 2;
        g_ClosestRivalRank = 0;
        g_DuelRaceActive = 1;
    }

    SeedCarAiSpeedKeys();
}
