#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/track.h"

/*
 * Pick the first authored speed threshold already reached by each car. Some
 * retail tables contain local progress reversals and some use all 48 slots,
 * so neither sorting nor a -1 terminator is required. A terminator, when
 * present before a match, selects the first key.
 */
void SeedCarAiSpeedKeys(void) {
    const s32 series = g_RaceSeries != 0;
    s32 carIndex;

    if (g_TrackEventData == NULL) {
        return;
    }

    for (carIndex = 0; carIndex < RACE_CAR_SLOT_COUNT; carIndex++) {
        GameCarRuntime *car = &g_Cars[carIndex];
        s32 position;
        s32 index;

        if (car->activeFlag == -1) {
            continue;
        }

        position = car->trackProgress >> 4;
        car->slideActive = 1;
        car->speedKeyIndex = 0;
        for (index = 0; index < TRACK_AI_SPEED_KEY_COUNT; index++) {
            const s32 progress =
                g_TrackEventData->aiSpeedKeys[series][index].progress;

            if (progress == -1) {
                car->speedKeyIndex = 0;
                break;
            }
            if (position >= progress) {
                car->speedKeyIndex = index;
                break;
            }
        }
    }
}
