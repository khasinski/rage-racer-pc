#include "game/track.h"

/*
 * Smooths the track angle at `pointIndex` by blending it (half weight, 0x200)
 * with the angles two points behind and two points ahead (wrap-aware).
 */
s32 SmoothRouteAngle(const TrackRoute *route, s32 pointIndex, s32 weight) {
    s32 center;
    s32 prevIndex;
    s32 prev;
    s32 left;
    s32 nextIndex;
    s32 next;
    s32 right;

    if (route == NULL || route->points == NULL || route->count <= 0) {
        return 0;
    }

    center = InterpolateRouteAngle(route, pointIndex, weight);

    prevIndex = RouteIndex(route, (s32)((u32)pointIndex - 2U));

    prev = InterpolateRouteAngle(route, prevIndex, weight);
    left = BlendAngle(center, prev, 0x200);

    nextIndex = RouteIndex(route, (s32)((u32)pointIndex + 2U));
    next = InterpolateRouteAngle(route, nextIndex, weight);
    right = BlendAngle(center, next, 0x200);

    return BlendAngle(left, right, 0x200);
}
