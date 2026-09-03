#include "game/car.h"
#include "game/car_track_internal.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"

/* Counts the cars whose lap progress is ahead of the player and publishes the
 * result as g_PlayerCar.drive.racePosition (1 = leader). Once the player has
 * completed every lap, the finishing position stays fixed. */
void UpdateRacePosition(void) {
    s32 playerProgress;
    s32 position = 1;
    s32 carIndex;

    if (g_PlayerCar.lap > g_LapCount) {
        return;
    }

    playerProgress = CarRaceProgress(AsRivalCar(&g_PlayerCar));
    for (carIndex = 0; carIndex < RACE_CAR_SLOT_COUNT; carIndex++) {
        const GameCarRuntime *car = &g_Cars[carIndex];

        if (car->activeFlag != -1 &&
            CarRaceProgress(car) > playerProgress) {
            position++;
        }
    }

    g_PlayerCar.drive.racePosition = position;
}
