#include "game/track.h"

static TrackRoute ActiveRoute(void) {
    const TrackRoute route = {.points = g_TrackPoints, .count = g_TrackPointCount};
    return route;
}

void InterpolateTrackPoint(s32 pointIndex, LVec *out, s32 weight) {
    const TrackRoute route = ActiveRoute();
    InterpolateRoutePoint(&route, pointIndex, out, weight);
}

s32 InterpolateTrackAngle(s32 pointIndex, s32 weight) {
    const TrackRoute route = ActiveRoute();
    return InterpolateRouteAngle(&route, pointIndex, weight);
}

s32 SmoothTrackAngle(s32 pointIndex, s32 weight) {
    const TrackRoute route = ActiveRoute();
    return SmoothRouteAngle(&route, pointIndex, weight);
}
