#include "game/track.h"

/* Interpolates the track angle between point `pointIndex` and its successor by `weight`. */
s32 InterpolateTrackAngle(s32 pointIndex, s32 weight) {
    s32 next;

    if (g_TrackPoints == 0 || g_TrackPointCount <= 0) {
        return 0;
    }
    next = WrapTrackPointIndex(pointIndex + 1);

    return BlendAngle(TrackPoint(pointIndex)->angle, TrackPoint(next)->angle, weight);
}
