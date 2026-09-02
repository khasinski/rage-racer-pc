#include "game/track.h"

/*
 * Linearly interpolates the centre-line XYZ between GameTrackPoint[pointIndex]
 * and its successor by `weight` (0..0x400), writing the result to out[0..2].
 * The +0x3FF/+0x7FF bias before the >>10 / >>11 shifts rounds toward zero.
 */
void InterpolateTrackPoint(s32 pointIndex, s32 *out, s32 weight) {
    s32 next;
    s32 inv = 0x400 - weight;
    GameTrackPoint *cur;
    GameTrackPoint *nxt;
    s32 sum;

    if (out == 0) {
        return;
    }
    if (g_TrackPoints == 0 || g_TrackPointCount <= 0) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        return;
    }

    next = WrapTrackPointIndex(pointIndex + 1);
    cur = TrackPoint(pointIndex);
    nxt = TrackPoint(next);
    sum = (cur->x * inv) + (nxt->x * weight);
    if (sum < 0) {
        sum += 0x3FF;
    }
    out[0] = sum >> 10;

    sum = (inv * cur->y) + (weight * nxt->y);
    if (sum < 0) {
        sum += 0x7FF;
    }
    out[1] = sum >> 11;

    sum = (cur->z * inv) + (nxt->z * weight);
    if (sum < 0) {
        sum += 0x3FF;
    }
    out[2] = sum >> 10;
}
