#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"

enum {
    PLAYER_VERTICAL_GROUNDED = 0,
    PLAYER_VERTICAL_RISING = 1,
    PLAYER_VERTICAL_AT_CREST = 2,
    PLAYER_VERTICAL_FALLING = 3,
};

static void AdvancePlayerJumpArc(PlayerCarRuntime *car, s32 groundHeight) {
    s32 tick = car->verticalMotionTimer + 1;

    car->verticalMotionTimer = tick;
    if (car->verticalMotionState == PLAYER_VERTICAL_RISING) {
        s32 rise = (s16)tick;

        car->y += car->verticalMotionRate * rise + rise * rise * 72 / 100;
        if (car->y >= groundHeight) {
            car->verticalMotionState = PLAYER_VERTICAL_GROUNDED;
        }
    } else if (car->verticalMotionState == PLAYER_VERTICAL_AT_CREST) {
        car->y = car->verticalTargetY;
        if (groundHeight - car->verticalMotionRate > car->verticalTargetY) {
            car->verticalMotionState = PLAYER_VERTICAL_FALLING;
            car->verticalMotionRate = car->verticalMotionTimer;
        }
    } else {
        s32 fall = (s16)tick - car->verticalMotionRate;

        car->y = car->verticalTargetY + fall * fall * 216 / 100;
        if (car->y >= groundHeight) {
            car->verticalMotionState = PLAYER_VERTICAL_GROUNDED;
        }
    }
}

static void ReconnectPlayerDrivetrain(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    const GameCarSpec *spec = g_CarSpec;
    s32 gearRatio = GetPositiveCarGearRatio(spec, drive->gear);
    s32 rpm;

    drive->drivetrainTorque =
        ((100 - (drive->gear - 1) * 4) * 10000) * car->speed / 100;
    g_ShiftSoundLevel = car->verticalMotionTimer & 0x3F;
    drive->yawOffset = 0;
    drive->launchHeading = car->headingAngle;
    drive->launchSpeed = car->speed / 0x100000;
    drive->spinRate = 0;
    rpm = car->speed * 160 / 1168 * 10000 / gearRatio;
    drive->jumpTimer = 0x14;
    drive->motionState = CAR_MOTION_AIRBORNE;
    g_ShiftTargetRpm = rpm;
    drive->shiftRpmDelta = (u16)rpm - (u16)drive->engineRpm;
    drive->engineLoad = rpm * GetCarGearLoad(spec, drive->gear) / 0x20000;
    if (drive->manual == 0) {
        drive->engineLoad = drive->engineLoad * 985 / 1000;
    }
}

static void LandPlayerCar(PlayerCarRuntime *car, s32 groundHeight) {
    GameCarDrive *drive = &car->drive;

    car->y = groundHeight + 8;
    car->verticalPitch = 0;
    car->verticalRoll = 0;
    StartCarBodyKick(1, AsRivalCar(car));
    g_ShiftSoundLevel = 0;
    if (car->verticalMotionTimer >= 19 && g_RacePhase < 3) {
        PlaySoundCue(0xE);
    }
    if (drive->motionState == CAR_MOTION_DRIVING &&
        car->verticalMotionTimer >= 3) {
        ReconnectPlayerDrivetrain(car);
    }
}

void UpdatePlayerJump(PlayerCarRuntime *car, s32 groundHeight) {
    if (car->verticalMotionState == PLAYER_VERTICAL_GROUNDED) {
        return;
    }

    AdvancePlayerJumpArc(car, groundHeight);
    if (car->verticalMotionState == PLAYER_VERTICAL_GROUNDED) {
        LandPlayerCar(car, groundHeight);
    }
}
