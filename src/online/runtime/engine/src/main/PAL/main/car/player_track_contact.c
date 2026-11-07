#include "game/car_track_internal.h"
#include "game/car_motion_internal.h"
#include "game/hull_rotation.h"

enum {
    SLOW_SKID_SPEED_LIMIT = 64,
};

static int ShouldSuppressSlowSkid(s32 skid, s32 speed) {
    return speed < SLOW_SKID_SPEED_LIMIT &&
           skid >= CAR_TRACK_CONTACT_FRONT_RIGHT &&
           skid <= CAR_TRACK_CONTACT_REAR_LEFT;
}

s32 ResolveCarTrackContact(PlayerCarRuntime *car, const TrackRoute *route,
                            const CarHullPoint corners[CAR_HULL_CORNER_COUNT],
                            int reverse) {
    Matrix toTrack;
    SVec trackRotation;
    CarTrackLimits limits;
    s32 skid;

    if (car == NULL || route == NULL || route->points == NULL ||
        route->count <= 0 || route->length <= 0 || corners == NULL) {
        return 0;
    }

    trackRotation.vx = 0;
    trackRotation.vy = (s16)(
        ((u32)car->bodyYaw - ANGLE_THREE_QUARTER_TURN +
         (u32)(s32)RoutePoint(route, car->trackPointIndex)->angle) &
        ANGLE_MASK);
    trackRotation.vz = 0;
    /* Retail builds this through the GTE convention, which rotates the
     * opposite way from BuildRotMatrixY: the limits are measured in the
     * track's frame, not the car's. Using the game builder here sent the
     * car down a different racing line. */
    const HullAxes axes = BuildHullAxes(0, trackRotation.vy, 0);
    toTrack = (Matrix){0};
    toTrack.m[0][0] = axes.xx;
    toTrack.m[0][2] = axes.xz;
    MeasureCarTrackLimits(&toTrack, corners, &limits);

    if (car->motionActive) {
        ApplyCarKnockback(AsRivalCar(car));
    }
    skid = StepCarTrackState(
        AsRivalCar(car), route, car->trackPointIndex, &limits, reverse, 1);

    if (ShouldSuppressSlowSkid(skid, car->speed)) {
        return 0;
    }
    return skid;
}
