#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "psyq/gte.h"

static void UpdateLaunchTyreVoice(const PlayerCarRuntime *car, s32 skid) {
    if (car->verticalMotionState != 0) {
        SetIndexedEffectVoice(-1, 0, 0);
        return;
    }

    if (skid < 513)
        SetIndexedEffectVoice(0, skid * 3 + 0x1800, skid / 8 + 0x40);
    else
        SetIndexedEffectVoice(0, 0x1E00, 0x7F);
}

static void ConsumeInitialLaunchEnergy(PlayerCarRuntime *car, s32 skid,
                                       s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;

    if (skid < 0x80 && spinMagnitude < 0x800)
        drive->launchEnergy -= (0x800 - spinMagnitude) * 4000 / 256;
    if (car->speed < 0x190)
        drive->launchEnergy -= (0x190 - car->speed) * 100;
}

static void UpdatePoweredLaunch(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    s32 towardsTarget;
    s32 skid;

    drive->bodyLiftOffset += 10;
    if (drive->bodyLiftOffset >= 100) drive->bodyLiftOffset = 100;

    towardsTarget =
        GetAngleDelta(car->bodyYaw, drive->targetHeading) * 98 / 100;
    towardsTarget =
        towardsTarget * (drive->steeringLoadAngle + 0x800) / 2048;
    drive->spinRate += towardsTarget * 16;

    if ((u32)(drive->steerPos + 127) < 255) {
        if (GetAngleDistance(car->bodyYaw, car->headingAngle) < 0x200) {
            drive->spinRate = drive->spinRate * 31 / 32;
            drive->spinRate +=
                GetAngleDelta(car->bodyYaw, car->headingAngle);
        } else if (spinMagnitude < 0x800) {
            drive->spinRate += towardsTarget / 2;
        }
    }

    if (drive->spinRate > 0x3600) drive->spinRate = 0x3600;
    if (drive->spinRate < -0x3600) drive->spinRate = -0x3600;
    car->bodyYaw += drive->spinRate / 256;

    drive->launchEnergy -= 64;
    skid = GetAngleDistance(car->bodyYaw, car->headingAngle);
    drive->launchEnergy -= skid * skid / 65536;
    drive->launchEnergy -= (0x3600 - spinMagnitude) / 64;
    if (car->speed < drive->speedScale / 2)
        drive->launchEnergy -= (drive->speedScale / 2 - car->speed) / 8;
    drive->launchEnergy -= drive->brakeInput * 4;
    drive->launchEnergy -= (0x100 - drive->acceleratorInput.value) * 4;

    car->speed -= drive->brakeInput * 10 / 256;
    car->speed -= (0x100 - drive->acceleratorInput.value) * 10 / 256;
}

static void UpdateDepletedLaunch(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    GameCarSpec *spec;
    s32 landingRpm;
    s32 offAxis;

    drive->spinRate = drive->spinRate * 15 / 16;
    if (spinMagnitude >= 0x1000) return;

    spec = g_CarSpec;
    drive->drivetrainTorque =
        (100 - (drive->gear - 1) * 4) * 10000 * car->speed / 100;
    drive->yawOffset = GetAngleDelta(car->headingAngle, car->bodyYaw);
    drive->launchHeading = car->headingAngle;
    car->headingAngle = car->bodyYaw;

    offAxis = drive->yawOffset < 0 ? -drive->yawOffset : drive->yawOffset;
    offAxis = offAxis < 0x401 ? offAxis * offAxis
                              : (0x800 - offAxis) * (0x800 - offAxis);
    drive->launchSpeed = offAxis * car->speed / 0x100000;
    drive->spinRate = 0;

    landingRpm = car->speed * 0xA0 / 1168 * 10000;
    landingRpm /= spec->gearRatio[drive->gear];
    drive->jumpTimer = 0x14;
    drive->motionState = CAR_MOTION_AIRBORNE;
    g_ShiftTargetRpm = landingRpm;
    drive->shiftRpmDelta =
        (s16)(g_ShiftTargetRpm - (u16)drive->engineRpm);
    drive->engineLoad = landingRpm * spec->gearLoad[drive->gear] / 0x20000;
    if (drive->manual == 0)
        drive->engineLoad = drive->engineLoad * 985 / 1000;
    g_ShiftSoundLevel =
        drive->shiftRpmDelta >= -99 && drive->shiftRpmDelta <= 99;
}

static void FinishLaunchFrame(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    s32 skid;
    s32 launchedHeading;

    drive->targetHeading = car->bodyYaw;
    SteerCarToTrackLine(car);

    skid = GetAngleDistance(car->bodyYaw, car->headingAngle);
    if (skid >= 0x401) {
        s32 push = car->acceleration;
        s32 scaled = (0x3600 - spinMagnitude) * 4 * push * (skid - 0x400);

        car->acceleration = push / 2 + scaled / 14155776;
    } else {
        car->acceleration = (0x200 - skid) * car->acceleration / 512;
    }

    launchedHeading = car->headingAngle;
    UpdateCarTravelVelocity(AsRivalCar(car));
    car->headingAngle = launchedHeading;
    drive->accelPos = rsin(car->headingAngle) * car->speed / 256;
    drive->brakePos = rcos(car->headingAngle) * car->speed / 256;
}

/* Update the takeoff state until its energy is spent, then hand the car to
 * UpdateCarAirborne. */
void UpdateCarLaunch(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 startYaw = car->bodyYaw;
    s32 startHeading = car->headingAngle;
    s32 spinMagnitude =
        drive->spinRate < 0 ? -drive->spinRate : drive->spinRate;
    s32 skid;

    skid = GetAngleDistance(startYaw, startHeading);
    if (skid >= 0x600) car->speed = car->speed * 990 / 1000;
    UpdateLaunchTyreVoice(car, skid);
    ConsumeInitialLaunchEnergy(car, skid, spinMagnitude);

    if (drive->launchEnergy > 0) {
        UpdatePoweredLaunch(car, spinMagnitude);
    } else {
        UpdateDepletedLaunch(car, spinMagnitude);
    }

    FinishLaunchFrame(car, spinMagnitude);
}
