#include "common.h"
#include "game/track.h"

/*
 * Linearly interpolates the centre-line XYZ between GameTrackPoint[pointIndex]
 * and its successor by `weight` (0..0x400), writing the result to out[0..2].
 * The +0x3FF/+0x7FF bias before the >>10 / >>11 shifts rounds toward zero.
 */
void InterpolateTrackPoint(s32 pointIndex, s32 *out, s32 weight) {
    s32 next = (pointIndex + 1) % g_TrackPointCount;
    s32 inv = 0x400 - weight;
    GameTrackPoint *cur = TrackPoint(pointIndex);
    GameTrackPoint *nxt = TrackPoint(next);
    s32 sum;

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

