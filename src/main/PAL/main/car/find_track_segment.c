#include "game/car.h"
#include "game/angle.h"
#include "game/render.h"
#include "game/track.h"

/*
 * Finds the track segment whose rotated, half-width quad contains the car.
 * The search starts at the caller's best guess and alternates forward and
 * backward over neighbouring segments.
 */
s32 FindTrackSegment(GameCarRuntime *car, s32 startIndex) {
    DVecValue corners[5];
    s32 index;
    s32 stride = 0;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return -1;
    }

    startIndex %= g_TrackPointCount;
    if (startIndex < 0) {
        startIndex += g_TrackPointCount;
    }
    index = startIndex;

    do {
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
        corners[1].components.vx = nearLeft * (s16)nearCos / 4096;
        corners[1].components.vy = -nearLeft * (s16)nearSin / 4096;
        corners[2].components.vx = -nearRight * (s16)nearCos / 4096;
        corners[2].components.vy = nearRight * (s16)nearSin / 4096;
        corners[3].components.vx = segmentX + farLeft * (s16)farCos / 4096;
        corners[3].components.vy = segmentZ - farLeft * (s16)farSin / 4096;
        corners[4].components.vx = segmentX - farRight * (s16)farCos / 4096;
        corners[4].components.vy = segmentZ + farRight * (s16)farSin / 4096;

        if (NormalClip(corners[1].packed, corners[2].packed,
                       corners[0].packed) >= 0 &&
            NormalClip(corners[2].packed, corners[4].packed,
                       corners[0].packed) >= 0 &&
            NormalClip(corners[4].packed, corners[3].packed,
                       corners[0].packed) > 0 &&
            NormalClip(corners[3].packed, corners[1].packed,
                       corners[0].packed) >= 0) {
            return index;
        }

        /* Preserve the recovered one-step wrapping. Properly normalising a
         * large negative stride changes which segment an off-track car gets
         * and has been observed to prevent a race from finishing. */
        stride++;
        index += (stride % 2) != 0 ? stride : -stride;
        index = index >= 0 ? index % g_TrackPointCount
                           : (index + g_TrackPointCount) % g_TrackPointCount;
    } while (index != startIndex);

    car->x = TrackPoint(index)->x;
    car->z = TrackPoint(index)->z;
    return -1;
}
