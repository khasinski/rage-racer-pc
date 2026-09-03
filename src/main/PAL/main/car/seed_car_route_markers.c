#include "game/car.h"
#include "game/race.h"
#include "game/track.h"

/*
 * Put every car on the speed key it has already reached at race start. The
 * list is ordered along the track and ends with a -1 sentinel.
 */
void SeedCarRouteMarkers(void) {
    const s32 series = g_RaceSeries != 0;
    s32 carIndex;

    if (g_TrackEventData == NULL) {
        return;
    }

    for (carIndex = 0; carIndex < RACE_CAR_SLOT_COUNT; carIndex++) {
        const s32 position = g_Cars[carIndex].trackProgress >> 4;
        s32 index;

        g_Cars[carIndex].routeMarkerActive = 1;
        g_Cars[carIndex].routeMarkerIndex = 0;
        for (index = 0; index < TRACK_AI_SPEED_KEY_COUNT; index++) {
            const s32 progress =
                g_TrackEventData->aiSpeedKeys[series][index].progress;

            if (progress == -1) {
                g_Cars[carIndex].routeMarkerIndex = 0;
                break;
            }
            if (position >= progress) {
                g_Cars[carIndex].routeMarkerIndex = index;
                break;
            }
        }
    }
}
