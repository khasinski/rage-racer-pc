#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"

void UpdateCarGearShiftState(PlayerCarRuntime *car, const GameCarSpec *spec,
                             s32 *acceleration) {
    GameCarDrive *drive = &car->drive;
    s16 targetGear;
    s32 countdown;

    if (drive->motionState == CAR_MOTION_TAKEOFF ||
        drive->motionState == CAR_MOTION_STANDING_START) {
        drive->jumpTimer = 0;
        drive->clutch = 0;
        return;
    }

    if (drive->motionState == CAR_MOTION_AIRBORNE && drive->jumpTimer >= 0) {
        s16 nextTimer = drive->jumpTimer - 1;

        drive->jumpTimer = nextTimer < 0 ? 0 : nextTimer;
        *acceleration = 0;
        targetGear = drive->gear;
        if (drive->gearDisp != targetGear) {
            s32 targetRpm =
                CalculateAirborneEngineRpm(spec, targetGear, car->speed);
            u16 currentRpm = (u16)drive->engineRpm;

            g_ShiftTargetRpm = targetRpm;
            drive->shiftRpmDelta = (s16)((u16)targetRpm - currentRpm);
        }
        drive->engineRpm = drive->shiftRpmDelta * drive->jumpTimer / 20 +
                           g_ShiftTargetRpm;
        return;
    }

    targetGear = drive->gear;
    if (drive->gearDisp != targetGear) {
        s32 wheelSpeed = (u16)car->acceleration;
        s32 targetRatio = GetPositiveCarGearRatio(spec, targetGear);
        s32 targetSpeed = (car->speed * 0x2710) /
                          (targetRatio * 0x490 / 160);

        drive->engineLoad = wheelSpeed;
        g_ShiftTargetSpeed = targetSpeed;
        if (drive->manual != 0 && drive->gearDisp < targetGear &&
            g_RoadGrade < 0 && targetGear >= 4) {
            s32 gradePenalty;
            s32 gradeScale;

            if (targetGear == 4) {
                gradePenalty = -g_RoadGrade / 120;
            } else if (targetGear == 5) {
                gradePenalty = -g_RoadGrade / 48;
            } else {
                gradePenalty = g_RoadGrade * -7 / 240;
            }
            wheelSpeed = (s16)wheelSpeed;
            gradeScale = 0x64 - gradePenalty;
            drive->engineLoad = (u16)(wheelSpeed * gradeScale / 100);
            g_ShiftTargetSpeed = gradeScale * targetSpeed / 100;
        }

        *acceleration = 0;
        if (drive->gearDisp > targetGear) {
            g_ShiftTargetSpeed += 0x1F4;
        }
        drive->clutch = 0xA;
        drive->drivetrainCoupled = 0;
        drive->shiftSpeedDelta =
            (s16)((u16)g_ShiftTargetSpeed - (u16)drive->engineRpm);
        return;
    }

    countdown = --drive->clutch;
    if ((s16)countdown <= 0) {
        drive->drivetrainCoupled = 1;
        drive->engineLoad = 0;
        drive->clutch = 0;
    } else if (drive->manual != 0) {
        drive->engineRpm = g_ShiftTargetSpeed -
                           drive->shiftSpeedDelta * (s16)countdown / 15;
    } else {
        drive->engineRpm = g_ShiftTargetSpeed -
                           drive->shiftSpeedDelta * (s16)countdown / 10;
    }
}
