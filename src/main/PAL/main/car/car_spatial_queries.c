#include "game/car.h"
#include "game/track.h"
#include "psyq/gte.h"

/* A heading is backwards when it is more than a quarter-turn, but less than
 * three quarters of a turn, away from the course direction. */
s32 IsCarFacingBackwards(PlayerCarRuntime *car) {
    s32 trackHeading = 0xC00 - TrackPoint(car->trackPointIndex)->angle;
    u32 headingDelta = (car->headingAngle - trackHeading) & 0xFFF;

    return headingDelta > 0x400 && headingDelta < 0xC00;
}

s32 IsPointInQuad(s32 p0, s32 p1, s32 p2, s32 p3, s32 point) {
    return NormalClip(p0, p1, point) >= 0 &&
           NormalClip(p1, p3, point) >= 0 &&
           NormalClip(p3, p2, point) >= 0 &&
           NormalClip(p2, p0, point) >= 0;
}
