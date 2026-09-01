#include "game/player_car_internal.h"
#include "game/race.h"

/* Counts the cars whose lap progress is ahead of the player and publishes the
 * result as g_RacePosition (1 = leader). Only runs on the final lap. */
void UpdateRacePosition(void) {
    s32 position;
    s32 i;
    s32 total;

    position = 1;
    if (g_LapCount >= g_PlayerCar.lap) {
        total = g_PlayerCar.progressA + g_PlayerCar.progressB;
        for (i = 0; i < 0xB; i++) {
            GameCarRuntime *car = &g_Cars[i];

            if (car->activeFlag != -1) {
                if (car->progressA + car->progressB > total) {
                    position++;
                }
            }
        }

        g_RacePosition = position;
    }
}
