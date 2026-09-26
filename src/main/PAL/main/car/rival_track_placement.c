#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/rival.h"
#include "game/race.h"
#include "game/track_internal.h"


/*
 * This stays two passes: every active car updates its lap progress before any
 * car applies knockback and resamples its track-relative pose.
 */
void PlaceRivalCarsOnTrack(void) {
    const TrackRoute route = {.points = g_TrackPoints, .arcs = g_TrackArcCenters,
        .count = g_TrackPointCount, .length = g_TrackLength};
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        if (g_Cars[index].activeFlag != -1) {
            AccumulateLapProgress(&g_Cars[index]);
        }
    }
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        PlaceRival(&g_Cars[index], &route, g_RaceSeries != 0);
    }
}
