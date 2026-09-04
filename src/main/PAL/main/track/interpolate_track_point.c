#include "game/track.h"

/*
 * Linearly interpolates the centre-line XYZ between GameTrackPoint[pointIndex]
 * and its successor by `weight` (0..0x400), writing the result to out[0..2].
 * The +0x3FF/+0x7FF bias before the >>10 / >>11 shifts rounds toward zero.
 */
void InterpolateTrackPoint(s32 pointIndex, s32 *out, s32 weight) {
    s32 next;
    s32 inv = (s32)(0x400u - (u32)weight);
    const GameTrackPoint *cur;
    const GameTrackPoint *nxt;
    s32 sum;

    if (out == NULL) {
        return;
    }
    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 0;
        return;
    }

    next = WrapTrackPointIndex((s32)((u32)pointIndex + 1U));
    cur = TrackPoint(pointIndex);
    nxt = TrackPoint(next);
    sum = (s32)((u32)cur->x * (u32)inv +
                (u32)nxt->x * (u32)weight);
    if (sum < 0) {
        sum += 0x3FF;
    }
    out[0] = sum >> 10;

    sum = (s32)((u32)inv * (u32)cur->y +
                (u32)weight * (u32)nxt->y);
    if (sum < 0) {
        sum += 0x7FF;
    }
    out[1] = sum >> 11;

    sum = (s32)((u32)cur->z * (u32)inv +
                (u32)nxt->z * (u32)weight);
    if (sum < 0) {
        sum += 0x3FF;
    }
    out[2] = sum >> 10;
}
