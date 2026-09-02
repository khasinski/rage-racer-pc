#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"

/* Counts the cars whose lap progress is ahead of the player and publishes the
 * result as g_PlayerCar.drive.racePosition (1 = leader). Only runs on the final lap. */
void UpdateRacePosition(void) {
    s32 playerProgress;
    s32 position = 1;
    s32 i;

    if (g_PlayerCar.lap > g_LapCount) {
        return;
    }

    playerProgress = g_PlayerCar.progressA + g_PlayerCar.progressB;
    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        GameCarRuntime *car = &g_Cars[i];

        if (car->activeFlag != -1 &&
            car->progressA + car->progressB > playerProgress) {
            position++;
        }
    }

    g_PlayerCar.drive.racePosition = position;
}
