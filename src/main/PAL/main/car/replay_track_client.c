#include "game/car_track_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/track_internal.h"

void ReconstructReplayCarTrackState(GameCarRuntime *car) {
    const TrackRoute route = {
        .points = g_TrackPoints,
        .arcs = g_TrackArcCenters,
        .count = g_TrackPointCount,
        .length = g_TrackLength,
    };
    ReconstructCarTrackState(car, &route, g_RaceSeries != 0);
}
