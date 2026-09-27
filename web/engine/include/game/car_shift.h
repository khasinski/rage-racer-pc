#ifndef GAME_CAR_SHIFT_H
#define GAME_CAR_SHIFT_H

#include "game/car.h"
#include "game/integer.h"

enum { CAR_AIRBORNE_SHIFT_FRAMES = 20 };

/* Shift commands are edges for this step, independent of controller layout. */
void ShiftCarGears(PlayerCarRuntime *car, const GameCarSpec *spec,
                    int shiftUp, int shiftDown);

static inline s32 CalculateAirborneEngineRpm(const GameCarSpec *spec,
                                             s32 gear, s32 speed) {
    s32 gearRatio = GetPositiveCarGearRatio(spec, gear);
    s32 wheelRpm = WrapSigned32((int64_t)speed * 160) / 1168;

    return WrapSigned32((int64_t)wheelRpm * 10000) / gearRatio;
}

static inline s16 CalculateCarRpmDelta(s32 targetRpm, s32 currentRpm) {
    return WrapSigned16((u16)targetRpm - (u16)currentRpm);
}

static inline s16 ClampCarGear(s32 gear, s32 topGear) {
    if (topGear < CAR_FIRST_FORWARD_GEAR) {
        topGear = CAR_FIRST_FORWARD_GEAR;
    } else if (topGear > CAR_FORWARD_GEAR_COUNT) {
        topGear = CAR_FORWARD_GEAR_COUNT;
    }
    if (gear < CAR_FIRST_FORWARD_GEAR) {
        return CAR_FIRST_FORWARD_GEAR;
    }
    return (s16)(gear > topGear ? topGear : gear);
}

/* No pad, track, audio or shared mutable state. Road grade is sampled by the
 * caller; all ongoing shift state belongs to car->drive. */
void UpdateCarGearShiftState(PlayerCarRuntime *car, const GameCarSpec *spec,
                             s32 roadGrade, s32 *acceleration);
void PrepareAirborneDrivetrain(PlayerCarRuntime *car, const GameCarSpec *spec);

#endif
