#include "game/render.h"

#include "game/track_camera_internal.h"

s32 FindNearestTrackCamera(GameRenderObject *car) {
    s32 best;
    GameTrackCameraNode *entry;
    s32 index;
    s32 selected = 0;
    s32 span;
    s32 halfSpan;
    s32 target;
    s32 dist;
    s32 tmp;
    u32 spanSign;
    s32 candidate;
    u16 rawValue;

    entry = g_TrackCameras;
    best = 0x7FFFFFFF;
    rawValue = entry[0].trackSection.raw;
    dist = entry[0].trackSection.value;
    index = 0;

    if (dist != -1) {
        tmp = g_TrackSectionCount;
        target = car->trackSection;
        tmp <<= 16;
        span = tmp >> 16;
        spanSign = tmp;
        tmp = spanSign >> 31;
        halfSpan = (span + tmp) >> 1;

        do {
            dist = (s16)rawValue;
            tmp = dist < target;
            if (tmp) {
                dist = target - dist;
            } else {
                dist = dist - target;
            }

            candidate = halfSpan < dist;
            if (candidate) {
                candidate = span - dist;
            } else {
                candidate = dist;
            }

            dist = candidate;
            tmp = dist < best;
            if (tmp) {
                selected = index;
                best = dist;
            }

            tmp = -1;
            index++;
            rawValue = entry[index].trackSection.raw;
            dist = entry[index].trackSection.value;
        } while (dist != tmp);
    }

    return selected;
}
