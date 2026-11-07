#include "game/angle.h"
#include "game/car.h"
#include "game/car_track_internal.h"
#include "game/track.h"

/* A heading is backwards when it is more than a quarter-turn, but less than
 * three quarters of a turn, away from the course direction. */
s32 CarFacesBackwards(const PlayerCarRuntime *car, const TrackRoute *route) {
    const GameTrackPoint *point;
    s32 trackHeading;
    u32 headingDelta;

    if (car == NULL || route == NULL || route->points == NULL || route->count <= 0) {
        return 0;
    }

    point = RoutePoint(route, car->trackPointIndex);
    trackHeading = ANGLE_THREE_QUARTER_TURN - point->angle;
    headingDelta = ((u32)car->headingAngle - (u32)trackHeading) & ANGLE_MASK;

    return headingDelta > ANGLE_QUARTER_TURN &&
           headingDelta < ANGLE_THREE_QUARTER_TURN;
}
