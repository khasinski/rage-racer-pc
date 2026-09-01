#include "game/car.h"
#include "game/render.h"
#include "game/track.h"

/* Aim at an offset point on the centre-line and turn towards it. */
void SteerCarToTrackLine(PlayerCarRuntime *car) {
    const GameCarSpec *spec = g_CarSpec;
    s32 lateral = car->trackLateralOffset;
    s32 aheadIndex;
    s32 target[3];
    s32 lineAngle;
    s32 sideways;
    s32 wantedHeading;

    /* A backwards-launched car follows the centre-line in reverse. */
    aheadIndex = car->drive.launchDirection != 0 ? car->trackPointIndex + 2
                                                 : car->trackPointIndex - 2;
    if (aheadIndex < 0) aheadIndex += g_TrackPointCount;
    aheadIndex %= g_TrackPointCount;

    InterpolateTrackPoint(aheadIndex, target, car->segmentFraction);
    lineAngle = ANGLE_FULL_TURN -
                SmoothTrackAngle(aheadIndex, car->segmentFraction);

    /* Bias negative fixed-point products so the shift truncates towards zero. */
    sideways = rsin(lineAngle) * lateral;
    if (sideways < 0) sideways += 0xFFF;
    target[0] += sideways >> 12;

    sideways = rcos(lineAngle) * lateral;
    if (sideways < 0) sideways += 0xFFF;
    target[2] += sideways >> 12;

    wantedHeading = ANGLE_QUARTER_TURN -
                    Atan2(target[0] - car->x, target[2] - car->z);

    if (car->verticalMotionState == 0) {
        /* Preserve the recovered signed 16-bit view of the response. */
        s32 response = (s16)spec->steerResponse;
        s32 towards;

        if (response <= 0) response = 1;
        towards = GetAngleDelta(car->headingAngle, wantedHeading);
        car->headingAngle += towards * 20 / response;
    }
}
