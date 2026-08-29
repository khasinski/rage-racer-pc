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
    GameCarRuntime *carReg = car;
    GameCarAiBlock *state;
    s32 current;
    s32 magnitude;
    s32 limit;
    s32 trackIndex;

    current = carReg->aiLateralOffset;
    state = GetCarAiBlock(carReg);
    magnitude = current;
    if (current < 0) {
        magnitude = -current;
    }

    if (carIndex < 4) {
        GameTrackPoint *point;
        s32 scaled;

        if (current < 0) {
            trackIndex = carReg->trackPointIndex;
            point = TrackPoint(trackIndex);
            limit = point->leftHalfWidth;
        } else {
            trackIndex = carReg->trackPointIndex;
            point = TrackPoint(trackIndex);
            limit = point->rightHalfWidth;
        }
        scaled = limit * 4;
        scaled += limit;
        limit = scaled / 8;
    } else {
        GameTrackPoint *point;

        if (current < 0) {
            trackIndex = carReg->trackPointIndex;
            point = TrackPoint(trackIndex);
            limit = (point->leftHalfWidth * 4) / 7;
        } else {
            trackIndex = carReg->trackPointIndex;
            point = TrackPoint(trackIndex);
            limit = (point->rightHalfWidth * 4) / 7;
        }
    }

    if (limit < magnitude) {
        if (current > 0) {
            state->aiLateralOffset = limit;
        } else {
            state->aiLateralOffset = -limit;
        }
    }
}
