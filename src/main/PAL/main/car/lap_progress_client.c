#include "game/car_track_internal.h"
#include "game/race.h"

static TrackRoute ClientRoute(void) {
    const TrackRoute route = {
        .points = g_TrackPoints,
        .count = g_TrackPointCount,
    };
    return route;
}

void SeedCarLapProgress(GameCarRuntime *car, s32 seedSelector) {
    const TrackRoute route = ClientRoute();
    if (g_TrackEventData == NULL) {
        car->progressA = 0;
        return;
    }
    SeedCarTrackProgress(car, &route, g_TrackEventData->trackWalkStart,
                         seedSelector, g_RaceSeries != 0);
}

void AccumulateLapProgress(GameCarRuntime *car) {
    const TrackRoute route = ClientRoute();
    const s32 target = FindCarTrackSegment(car, &route, car->trackPointIndex);
    MoveCarTrackProgress(car, &route, target, g_RaceSeries != 0);
}
