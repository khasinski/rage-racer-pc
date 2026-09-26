#include "game/car_track_internal.h"

void SteerCarToTrackLine(PlayerCarRuntime *car) {
    const TrackRoute route = {.points = g_TrackPoints, .count = g_TrackPointCount};
    SteerCarOnRoute(car, g_CarSpec, &route);
}
