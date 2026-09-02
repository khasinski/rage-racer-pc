#include "game/car.h"
#include "game/track.h"


/*
 * Clamps the car's lateral offset (aiLateralOffset) to a fraction of the track
 * half-width at its current point: `leftHalfWidth` when offset is negative,
 * `rightHalfWidth` otherwise. `carIndex` selects the
 * scaling: <4 uses 5/8 of the half-width, else 4/7. Writes the clamped value
 * back into `aiLateralOffset` only if it would exceed the
 * limit.
 */
void ClampCarLateralOffset(GameCarRuntime *car, s32 carIndex) {
    s32 current = car->aiLateralOffset;
    s32 magnitude;
    s32 halfWidth;
    s32 limit;
    GameTrackPoint *point;

    if (g_TrackPointCount <= 0 || g_TrackPoints == NULL) {
        return;
    }

    magnitude = current < 0 ? -current : current;
    point = TrackPoint(car->trackPointIndex);
    halfWidth = current < 0 ? point->leftHalfWidth : point->rightHalfWidth;
    limit = carIndex < 4 ? (halfWidth * 5) / 8 : (halfWidth * 4) / 7;

    if (limit < magnitude) {
        car->aiLateralOffset = (s16)(current > 0 ? limit : -limit);
    }
}
