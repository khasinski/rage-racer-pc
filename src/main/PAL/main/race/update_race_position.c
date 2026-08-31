#include "game/player_car_internal.h"
#include "game/race.h"

/* Counts the cars whose lap progress is ahead of the player and publishes the
 * result as g_RacePosition (1 = leader). Only runs on the final lap. */
void UpdateRacePosition(void) {
    s32 position;
    s32 i;
    s32 total;
    CarProgressWindow *cars;

    position = 1;
    if (g_LapCount >= g_PlayerCar.lap) {
        total = g_PlayerCar.progressA + g_PlayerCar.progressB;
        cars = GetCarProgressWindow(&g_Cars[0]);

        for (i = 0; i < 0xB; i++) {
            CarProgressWindow *entry = &cars[i];
            if (entry->activeFlag != -1) {
                if ((GetCarProgressWindowProgressA(entry) + entry->progressB) - total > 0) {
                    position++;
                }
            }
        }

        g_RacePosition = position;
    }
}
