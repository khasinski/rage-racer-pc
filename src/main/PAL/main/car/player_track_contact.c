#include "game/car.h"
#include "game/angle.h"
#include "game/car_internal.h"
#include "game/render.h"
#include "game/track_internal.h"

#include "rage/trace.h"

enum {
    SLOW_SKID_SPEED_LIMIT = 64,
};

static int ShouldSuppressSlowSkid(s32 skid, s32 speed) {
    return speed < SLOW_SKID_SPEED_LIMIT &&
           skid >= CAR_TRACK_CONTACT_FRONT_RIGHT &&
           skid <= CAR_TRACK_CONTACT_REAR_LEFT;
}

s32 ResolvePlayerTrackContact(PlayerCarRuntime *car) {
    Matrix toTrack;
    SVec trackRotation;
    CarTrackLimits limits;
    s32 skid;

    if (g_TrackPoints == NULL || g_TrackPointCount <= 0) {
        return 0;
    }

    trackRotation.vx = 0;
    trackRotation.vy = (s16)(
        ((u32)car->bodyYaw - ANGLE_THREE_QUARTER_TURN +
         (u32)(s32)TrackPoint(car->trackPointIndex)->angle) &
        ANGLE_MASK);
    trackRotation.vz = 0;
    BuildRotMatrixY(&toTrack, trackRotation.vy);
    MeasurePlayerTrackLimits(&toTrack, &limits);

    if (car->motionActive) {
        ApplyCarKnockback(AsRivalCar(car));
    }
    TraceCarMotion("post-knockback", car);
    skid = UpdateCarTrackState(
        AsRivalCar(car), car->trackPointIndex, &limits);
    TraceCarMotion("post-track", car);

    if (ShouldSuppressSlowSkid(skid, car->speed)) {
        return 0;
    }
    return skid;
}
