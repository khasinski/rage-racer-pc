#include "game/car.h"
#include "game/angle.h"
#include "game/render.h"
#include "game/track.h"

static int TrackSegmentContainsCar(const GameCarRuntime *car, s32 index) {
    DVecValue corners[5];
    const GameTrackPoint *near = TrackPoint(index);
    const GameTrackPoint *far = TrackPoint(index + 1);
    s32 segmentX = far->x - near->x;
    s32 segmentZ = far->z - near->z;
    s32 nearCos = rcos(ANGLE_THREE_QUARTER_TURN - near->angle);
    s32 nearSin = rsin(ANGLE_THREE_QUARTER_TURN - near->angle);
    s32 farCos = rcos(ANGLE_THREE_QUARTER_TURN - far->angle);
    s32 farSin = rsin(ANGLE_THREE_QUARTER_TURN - far->angle);
    s32 nearLeft = (s16)(near->leftHalfWidth * 2);
    s32 nearRight = (s16)(near->rightHalfWidth * 2);
    s32 farLeft = (s16)(far->leftHalfWidth * 2);
    s32 farRight = (s16)(far->rightHalfWidth * 2);

    /* All corners use the near centre-line point as their origin. */
    corners[0].components.vx = car->x - near->x;
    corners[0].components.vy = car->z - near->z;
    corners[1].components.vx = nearLeft * (s16)nearCos / ANGLE_FULL_TURN;
    corners[1].components.vy = -nearLeft * (s16)nearSin / ANGLE_FULL_TURN;
    corners[2].components.vx = -nearRight * (s16)nearCos / ANGLE_FULL_TURN;
    corners[2].components.vy = nearRight * (s16)nearSin / ANGLE_FULL_TURN;
    corners[3].components.vx =
        segmentX + farLeft * (s16)farCos / ANGLE_FULL_TURN;
    corners[3].components.vy =
        segmentZ - farLeft * (s16)farSin / ANGLE_FULL_TURN;
    corners[4].components.vx =
        segmentX - farRight * (s16)farCos / ANGLE_FULL_TURN;
    corners[4].components.vy =
        segmentZ + farRight * (s16)farSin / ANGLE_FULL_TURN;

    return NormalClip(corners[1].packed, corners[2].packed,
                      corners[0].packed) >= 0 &&
           NormalClip(corners[2].packed, corners[4].packed,
                      corners[0].packed) >= 0 &&
           NormalClip(corners[4].packed, corners[3].packed,
                      corners[0].packed) > 0 &&
           NormalClip(corners[3].packed, corners[1].packed,
                      corners[0].packed) >= 0;
}

/*
 * Finds the track segment whose rotated, half-width quad contains the car.
 * The search starts at the caller's best guess and alternates forward and
 * backward over neighbouring segments.
 */
s32 FindTrackSegment(GameCarRuntime *car, s32 startIndex) {
    s32 attempts;
    s32 index;
    s32 stride = 0;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return -1;
    }

    startIndex = WrapTrackPointIndex(startIndex);
    index = startIndex;

    for (attempts = 0; attempts < g_TrackPointCount; attempts++) {
        if (TrackSegmentContainsCar(car, index)) {
            return index;
        }

        /* Preserve the recovered alternating order: start, +1, -1, +2, -2,
         * ... . Its first point-count entries visit every segment once. */
        stride++;
        index += (stride % 2) != 0 ? stride : -stride;
        index = WrapTrackPointIndex(index);
    }

    car->x = TrackPoint(startIndex)->x;
    car->z = TrackPoint(startIndex)->z;
    return -1;
}
