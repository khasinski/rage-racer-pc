#include "game/car.h"
#include "game/car_internal.h"
#include "game/track.h"

/* Aim at an offset point on the centre-line and turn towards it. */
void SteerCarToTrackLine(PlayerCarRuntime *car) {
    const GameCarSpec *spec = g_CarSpec;
    s32 lateral = car->trackLateralOffset;
    s32 aheadIndex;
    s32 wantedHeading;

    if (spec == NULL || g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return;
    }

    /* A backwards-launched car follows the centre-line in reverse. */
    aheadIndex = car->drive.launchDirection != 0 ? car->trackPointIndex + 2
                                                 : car->trackPointIndex - 2;
    aheadIndex %= g_TrackPointCount;
    if (aheadIndex < 0) {
        aheadIndex += g_TrackPointCount;
    }

    wantedHeading = CalculateTrackOffsetHeading(
        aheadIndex, car->segmentFraction, car->x, car->z, lateral);

    if (car->verticalMotionState == CAR_VERTICAL_GROUNDED) {
        /* Preserve the recovered signed 16-bit view of the response. */
        s32 response = (s16)spec->steerResponse;
        s32 towards;

        if (response <= 0) {
            response = 1;
        }
        towards = GetAngleDelta(car->headingAngle, wantedHeading);
        car->headingAngle += towards * 20 / response;
    }
}
