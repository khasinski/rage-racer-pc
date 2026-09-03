#include "game/track_internal.h"


/*
 * Smooths the track angle at `pointIndex` by blending it (half weight, 0x200)
 * with the angles two points behind and two points ahead (wrap-aware).
 */
s32 SmoothTrackAngle(s32 pointIndex, s32 weight) {
    s32 center;
    s32 prevIndex;
    s32 prev;
    s32 left;
    s32 nextIndex;
    s32 next;
    s32 right;

    center = InterpolateTrackAngle(pointIndex, weight);

    prevIndex = WrapTrackPointIndex((s32)((u32)pointIndex - 2U));

    prev = InterpolateTrackAngle(prevIndex, weight);
    left = BlendAngle(center, prev, 0x200);

    nextIndex = WrapTrackPointIndex((s32)((u32)pointIndex + 2U));
    next = InterpolateTrackAngle(nextIndex, weight);
    right = BlendAngle(center, next, 0x200);

    return BlendAngle(left, right, 0x200);
}
