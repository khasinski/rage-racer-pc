#ifndef RAGE_TEST_TRACK_H
#define RAGE_TEST_TRACK_H

#include "common.h"

typedef struct GameTrackPoint {
    s32 x;
    s32 z;
    s16 y;
    s16 angle;
    s16 surfacePitch;
    s16 crossSlope;
    s16 leftHalfWidth;
    s16 rightHalfWidth;
    u16 arcRef;
    u16 segmentLength;
} GameTrackPoint;

extern GameTrackPoint *g_TrackPoints;
extern s32 g_TrackPointCount;
s32 BlendAngle(s32 angleA, s32 angleB, s32 weight);
s32 InterpolateTrackAngle(s32 pointIndex, s32 weight);
void InterpolateTrackPoint(s32 pointIndex, s32 *out, s32 weight);

#endif
