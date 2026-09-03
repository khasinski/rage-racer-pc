#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"

enum {
    GEAR_SHIFT_CLUTCH_FRAMES = 10,
    MANUAL_SHIFT_INTERPOLATION_FRAMES = 15,
    AUTOMATIC_SHIFT_INTERPOLATION_FRAMES = 10,
    DOWNSHIFT_TARGET_SPEED_BONUS = 500,
};

static void UpdateAirborneGearShift(PlayerCarRuntime *car,
                                    const GameCarSpec *spec,
                                    s32 *acceleration) {
    GameCarDrive *drive = &car->drive;
    s16 nextTimer = drive->jumpTimer - 1;

    drive->jumpTimer = nextTimer < 0 ? 0 : nextTimer;
    *acceleration = 0;
    if (drive->gearDisp != drive->gear) {
        s32 targetRpm =
            CalculateAirborneEngineRpm(spec, drive->gear, car->speed);
        g_ShiftTargetRpm = targetRpm;
        drive->shiftRpmDelta = CalculateCarRpmDelta(
            targetRpm, drive->engineRpm);
    }
    drive->engineRpm = drive->shiftRpmDelta * drive->jumpTimer / 20 +
                       g_ShiftTargetRpm;
}

static void ApplyUphillManualShiftPenalty(GameCarDrive *drive,
                                          s16 targetGear,
                                          s32 wheelSpeed) {
    s32 gradePenalty;
    s32 gradeScale;

    if (drive->manual == 0 || drive->gearDisp >= targetGear ||
        g_RoadGrade >= 0 || targetGear < 4) {
        return;
    }

    if (targetGear == 4) {
        gradePenalty = -g_RoadGrade / 120;
    } else if (targetGear == 5) {
        gradePenalty = -g_RoadGrade / 48;
    } else {
        gradePenalty = g_RoadGrade * -7 / 240;
    }
    gradeScale = 100 - gradePenalty;
    drive->engineLoad = (u16)((s16)wheelSpeed * gradeScale / 100);
    g_ShiftTargetSpeed = gradeScale * g_ShiftTargetSpeed / 100;
}

static void BeginCarGearShift(PlayerCarRuntime *car,
                              const GameCarSpec *spec, s32 *acceleration) {
    GameCarDrive *drive = &car->drive;
    s16 targetGear = drive->gear;
    s32 wheelSpeed = (u16)car->acceleration;
    s32 targetRatio = GetPositiveCarGearRatio(spec, targetGear);

    drive->engineLoad = wheelSpeed;
    g_ShiftTargetSpeed = (car->speed * 10000) /
                         (targetRatio * 1168 / 160);
    ApplyUphillManualShiftPenalty(drive, targetGear, wheelSpeed);

    *acceleration = 0;
    if (drive->gearDisp > targetGear) {
        g_ShiftTargetSpeed += DOWNSHIFT_TARGET_SPEED_BONUS;
    }
    drive->clutch = GEAR_SHIFT_CLUTCH_FRAMES;
    drive->drivetrainCoupled = 0;
    drive->shiftSpeedDelta = CalculateCarRpmDelta(
        g_ShiftTargetSpeed, drive->engineRpm);
}

static void AdvanceCarGearShift(GameCarDrive *drive) {
    s32 countdown = --drive->clutch;
    s32 interpolationFrames;

    if ((s16)countdown <= 0) {
        drive->drivetrainCoupled = 1;
        drive->engineLoad = 0;
        drive->clutch = 0;
        return;
    }

    interpolationFrames = drive->manual != 0
        ? MANUAL_SHIFT_INTERPOLATION_FRAMES
        : AUTOMATIC_SHIFT_INTERPOLATION_FRAMES;
    drive->engineRpm = g_ShiftTargetSpeed -
                       drive->shiftSpeedDelta * (s16)countdown /
                           interpolationFrames;
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
