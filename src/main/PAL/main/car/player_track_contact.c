#include "game/car.h"
#include "game/car_internal.h"
#include "game/render.h"
#include "game/track_internal.h"

#include "rage/trace.h"

s32 ResolvePlayerTrackContact(PlayerCarRuntime *car) {
    Matrix toTrack;
    SVec trackRotation = {
        .vx = 0,
        .vy = (s16)((car->bodyYaw - 0xC00 +
                     TrackPoint(car->trackPointIndex)->angle) & 0xFFF),
        .vz = 0,
    };
    CarTrackLimits limits;
    s32 skid;

    BuildRotMatrixY(&toTrack, trackRotation.vy);
    MeasurePlayerTrackLimits(&toTrack, &limits);

    if (car->motionActive) {
        ApplyCarKnockback(AsRivalCar(car));
    }
    TraceCarMotion("post-knockback", car);
    skid = UpdateCarTrackState(
        AsRivalCar(car), car->trackPointIndex, &limits);
    TraceCarMotion("post-track", car);

    if ((u32)(skid - 2) < 2U && car->speed < 64) {
        return 0;
    }
    return skid;
}
