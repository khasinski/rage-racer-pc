#include "game/track.h"

/* Interpolates the track angle between point `pointIndex` and its successor by `weight`. */
s32 InterpolateRouteAngle(const TrackRoute *route, s32 pointIndex, s32 weight) {
    s32 next;

    if (route == NULL || route->points == NULL || route->count <= 0) {
        return 0;
    }
    next = RouteIndex(route, (s32)((u32)pointIndex + 1U));

    return BlendAngle(RoutePoint(route, pointIndex)->angle, RoutePoint(route, next)->angle,
                      weight);
}
