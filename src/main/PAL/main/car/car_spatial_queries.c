#include "game/angle.h"
#include "game/car.h"
#include "game/track.h"

/* A heading is backwards when it is more than a quarter-turn, but less than
 * three quarters of a turn, away from the course direction. */
s32 IsCarFacingBackwards(const PlayerCarRuntime *car) {
    s32 trackHeading = ANGLE_THREE_QUARTER_TURN -
                       TrackPoint(car->trackPointIndex)->angle;
    u32 headingDelta =
        (car->headingAngle - trackHeading) & ANGLE_MASK;

    return headingDelta > ANGLE_QUARTER_TURN &&
           headingDelta < ANGLE_THREE_QUARTER_TURN;
}
