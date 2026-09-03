#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/race.h"

enum {
    GEAR_SHIFT_CLUTCH_FRAMES = 10,
    MANUAL_SHIFT_INTERPOLATION_FRAMES = 15,
    AUTOMATIC_SHIFT_INTERPOLATION_FRAMES = 10,
    DOWNSHIFT_TARGET_SPEED_BONUS = 500,
    FIRST_UPHILL_PENALTY_GEAR = 4,
    FOURTH_GEAR_PENALTY_DIVISOR = 120,
    FIFTH_GEAR_PENALTY_DIVISOR = 48,
    TOP_GEAR_PENALTY_NUMERATOR = -7,
    TOP_GEAR_PENALTY_DIVISOR = 240,
    PERCENT_SCALE = 100,
    GEAR_RATIO_INTERNAL_SCALE = 1168,
    GEAR_RATIO_SOURCE_SCALE = 160,
    RPM_FIXED_SCALE = 10000,
    MINIMUM_RATIO_SCALE = 1,
};

static void UpdateAirborneGearShift(PlayerCarRuntime *car,
                                    const GameCarSpec *spec,
                                    s32 *acceleration) {
    GameCarDrive *drive = &car->drive;
    s16 nextTimer = WrapSigned16((int64_t)drive->jumpTimer - 1);

    drive->jumpTimer = nextTimer < 0 ? 0 : nextTimer;
    *acceleration = 0;
    if (drive->gearDisp != drive->gear) {
        s32 targetRpm =
            CalculateAirborneEngineRpm(spec, drive->gear, car->speed);
        g_ShiftTargetRpm = targetRpm;
        drive->shiftRpmDelta = CalculateCarRpmDelta(
            targetRpm, drive->engineRpm);
    }
    drive->engineRpm = WrapSigned32(
        (int64_t)WrapSigned32(
            (int64_t)drive->shiftRpmDelta * drive->jumpTimer) /
            CAR_AIRBORNE_SHIFT_FRAMES +
        g_ShiftTargetRpm);
}

static void ApplyUphillManualShiftPenalty(GameCarDrive *drive,
                                          s16 targetGear,
                                          s32 wheelSpeed) {
    s32 gradePenalty;
    s32 gradeScale;

    if (drive->manual == 0 || drive->gearDisp >= targetGear ||
        g_RoadGrade >= 0 || targetGear < FIRST_UPHILL_PENALTY_GEAR) {
        return;
    }

    if (targetGear == FIRST_UPHILL_PENALTY_GEAR) {
        gradePenalty = WrapSigned32(-(int64_t)g_RoadGrade) /
                       FOURTH_GEAR_PENALTY_DIVISOR;
    } else if (targetGear == FIRST_UPHILL_PENALTY_GEAR + 1) {
        gradePenalty = WrapSigned32(-(int64_t)g_RoadGrade) /
                       FIFTH_GEAR_PENALTY_DIVISOR;
    } else {
        gradePenalty = WrapSigned32(
            (int64_t)g_RoadGrade * TOP_GEAR_PENALTY_NUMERATOR) /
            TOP_GEAR_PENALTY_DIVISOR;
    }
    gradeScale = WrapSigned32((int64_t)PERCENT_SCALE - gradePenalty);
    drive->engineLoad = WrapSigned16(
        WrapSigned32(
            (int64_t)WrapSigned16(wheelSpeed) * gradeScale) /
            PERCENT_SCALE);
    g_ShiftTargetSpeed = WrapSigned32(
        (int64_t)gradeScale * g_ShiftTargetSpeed) / PERCENT_SCALE;
}

static void BeginCarGearShift(PlayerCarRuntime *car,
                              const GameCarSpec *spec, s32 *acceleration) {
    GameCarDrive *drive = &car->drive;
    s16 targetGear = drive->gear;
    s32 wheelSpeed = (u16)car->acceleration;
    s32 targetRatio = GetPositiveCarGearRatio(spec, targetGear);
    s32 ratioScale = WrapSigned32(
        (int64_t)targetRatio * GEAR_RATIO_INTERNAL_SCALE) /
        GEAR_RATIO_SOURCE_SCALE;

    drive->engineLoad = WrapSigned16(wheelSpeed);
    if (ratioScale == 0) {
        ratioScale = MINIMUM_RATIO_SCALE;
    }
    g_ShiftTargetSpeed =
        WrapSigned32((int64_t)car->speed * RPM_FIXED_SCALE) / ratioScale;
    ApplyUphillManualShiftPenalty(drive, targetGear, wheelSpeed);

    *acceleration = 0;
    if (drive->gearDisp > targetGear) {
        g_ShiftTargetSpeed = WrapSigned32(
            (int64_t)g_ShiftTargetSpeed + DOWNSHIFT_TARGET_SPEED_BONUS);
    }
    drive->clutch = GEAR_SHIFT_CLUTCH_FRAMES;
    drive->drivetrainCoupled = 0;
    drive->shiftSpeedDelta = CalculateCarRpmDelta(
        g_ShiftTargetSpeed, drive->engineRpm);
}

static void AdvanceCarGearShift(GameCarDrive *drive) {
    s16 countdown = WrapSigned16((int64_t)drive->clutch - 1);
    s32 interpolationFrames;

    drive->clutch = countdown;
    if (countdown <= 0) {
        drive->drivetrainCoupled = 1;
        drive->engineLoad = 0;
        drive->clutch = 0;
        return;
    }

    interpolationFrames = drive->manual != 0
        ? MANUAL_SHIFT_INTERPOLATION_FRAMES
        : AUTOMATIC_SHIFT_INTERPOLATION_FRAMES;
    drive->engineRpm = WrapSigned32(
        (int64_t)g_ShiftTargetSpeed -
        WrapSigned32((int64_t)drive->shiftSpeedDelta * countdown) /
            interpolationFrames);
}

void UpdateCarGearShiftState(PlayerCarRuntime *car, const GameCarSpec *spec,
                             s32 *acceleration) {
    GameCarDrive *drive = &car->drive;

    if (drive->motionState == CAR_MOTION_TAKEOFF ||
        drive->motionState == CAR_MOTION_STANDING_START) {
        drive->jumpTimer = 0;
        drive->clutch = 0;
        return;
    }
    if (drive->motionState == CAR_MOTION_AIRBORNE && drive->jumpTimer >= 0) {
        UpdateAirborneGearShift(car, spec, acceleration);
        return;
    }
    if (drive->gearDisp != drive->gear) {
        BeginCarGearShift(car, spec, acceleration);
        return;
    }
    AdvanceCarGearShift(drive);
}
