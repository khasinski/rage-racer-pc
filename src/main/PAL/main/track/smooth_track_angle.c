#include "game/track.h"


/*
 * Smooths the track angle at `pointIndex` by blending it (half weight, 0x200)
 * with the angles two points behind and two points ahead (wrap-aware).
 */
s32 SmoothTrackAngle(s32 pointIndex, s32 weight) {
    s32 center;
    s32 prev_index;
    s32 prev;
    s32 left;
    s32 next_index;
    s32 next;
    s32 right;

    center = InterpolateTrackAngle(pointIndex, weight);

    prev_index = pointIndex - 2;
    if (prev_index < 0) {
        s32 tmp;
        tmp = g_TrackPointCount;
        tmp -= 2;
        prev_index = tmp + pointIndex;
    }

    prev = InterpolateTrackAngle(prev_index, weight);
    left = BlendAngle(center, prev, 0x200);

    next_index = (pointIndex + 2) % g_TrackPointCount;
    next = InterpolateTrackAngle(next_index, weight);
    right = BlendAngle(center, next, 0x200);

    return BlendAngle(left, right, 0x200);
}
