#include "game/angle.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "psyq/gte.h"

static void UpdateLaunchTyreVoice(const PlayerCarRuntime *car, s32 skid) {
    if (car->verticalMotionState != CAR_VERTICAL_GROUNDED) {
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

    if (skid < 0x80 && spinMagnitude < 0x800) {
        s32 deficit = WrapSigned32(
            INT64_C(0x800) - spinMagnitude);
        s32 cost = WrapSigned32((int64_t)deficit * 4000) / 256;

        drive->launchEnergy = WrapSigned32(
            (int64_t)drive->launchEnergy - cost);
    }
    if (car->speed < 0x190) {
        s32 deficit = WrapSigned32(INT64_C(0x190) - car->speed);
        s32 cost = WrapSigned32((int64_t)deficit * 100);

        drive->launchEnergy = WrapSigned32(
            (int64_t)drive->launchEnergy - cost);
    }
}

static void UpdatePoweredLaunch(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    s32 towardsTarget;
    s32 skid;

    drive->bodyLiftOffset = WrapSigned16(
        (s32)drive->bodyLiftOffset + 10);
    if (drive->bodyLiftOffset >= 100) drive->bodyLiftOffset = 100;

    towardsTarget =
        GetAngleDelta(car->bodyYaw, drive->targetHeading) * 98 / 100;
    towardsTarget = WrapSigned32(
        (int64_t)towardsTarget * WrapSigned32(
            (int64_t)drive->steeringLoadAngle + 0x800)) / 2048;
    drive->spinRate = WrapSigned32(
        (int64_t)drive->spinRate +
        WrapSigned32((int64_t)towardsTarget * 16));

    if ((u32)WrapSigned32((int64_t)drive->steerPos + 127) < 255) {
        if (GetAngleDistance(car->bodyYaw, car->headingAngle) < 0x200) {
            drive->spinRate = WrapSigned32(
                (int64_t)drive->spinRate * 31) / 32;
            drive->spinRate = WrapSigned32(
                (int64_t)drive->spinRate +
                GetAngleDelta(car->bodyYaw, car->headingAngle));
        } else if (spinMagnitude < 0x800) {
            drive->spinRate = WrapSigned32(
                (int64_t)drive->spinRate + towardsTarget / 2);
        }
    }

    if (drive->spinRate > 0x3600) drive->spinRate = 0x3600;
    if (drive->spinRate < -0x3600) drive->spinRate = -0x3600;
    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw + drive->spinRate / 256);

    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - 64);
    skid = GetAngleDistance(car->bodyYaw, car->headingAngle);
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - skid * skid / 65536);
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy -
        WrapSigned32(INT64_C(0x3600) - spinMagnitude) / 64);
    if (car->speed < drive->speedScale / 2) {
        s32 speedDeficit = WrapSigned32(
            (int64_t)(drive->speedScale / 2) - car->speed);

        drive->launchEnergy = WrapSigned32(
            (int64_t)drive->launchEnergy - speedDeficit / 8);
    }
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy - drive->brakeInput * 4);
    drive->launchEnergy = WrapSigned32(
        (int64_t)drive->launchEnergy -
        (0x100 - drive->acceleratorInput.value) * 4);

    car->speed = WrapSigned32(
        (int64_t)car->speed - drive->brakeInput * 10 / 256);
    car->speed = WrapSigned32(
        (int64_t)car->speed -
        (0x100 - drive->acceleratorInput.value) * 10 / 256);
}

static void UpdateDepletedLaunch(PlayerCarRuntime *car, s32 spinMagnitude) {
    GameCarDrive *drive = &car->drive;
    s32 offAxis;

    drive->spinRate = WrapSigned32(
        (int64_t)drive->spinRate * 15) / 16;
    if (spinMagnitude >= 0x1000) return;

    drive->yawOffset = GetAngleDelta(car->headingAngle, car->bodyYaw);
    drive->launchHeading = car->headingAngle;
    car->headingAngle = car->bodyYaw;

    offAxis = drive->yawOffset < 0
        ? WrapSigned32(-(int64_t)drive->yawOffset)
        : drive->yawOffset;
    if (offAxis < 0x401) {
        offAxis = WrapSigned32((int64_t)offAxis * offAxis);
    } else {
        s32 distance = WrapSigned32(INT64_C(0x800) - offAxis);

        offAxis = WrapSigned32((int64_t)distance * distance);
    }
    drive->launchSpeed = WrapSigned32(
        (int64_t)offAxis * car->speed) / 0x100000;
    drive->spinRate = 0;

    PrepareAirborneDrivetrain(car);
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
        s32 scaled = WrapSigned32(INT64_C(0x3600) - spinMagnitude);

        scaled = WrapSigned32((int64_t)scaled * 4);
        scaled = WrapSigned32((int64_t)scaled * push);
        scaled = WrapSigned32((int64_t)scaled * (skid - 0x400));
        car->acceleration = WrapSigned32(
            (int64_t)(push / 2) + scaled / 14155776);
    } else {
        car->acceleration = WrapSigned32(
            (int64_t)(0x200 - skid) * car->acceleration) / 512;
    }

    launchedHeading = car->headingAngle;
    UpdateCarTravelVelocity(AsRivalCar(car));
    car->headingAngle = launchedHeading;
    drive->accelPos = WrapSigned32(
        (int64_t)rsin(car->headingAngle) * car->speed) / 256;
    drive->brakePos = WrapSigned32(
        (int64_t)rcos(car->headingAngle) * car->speed) / 256;
}

/* Update the takeoff state until its energy is spent, then hand the car to
 * UpdateCarAirborne. */
void UpdateCarLaunch(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 startYaw = car->bodyYaw;
    s32 startHeading = car->headingAngle;
    s32 spinMagnitude = drive->spinRate < 0
        ? WrapSigned32(-(int64_t)drive->spinRate)
        : drive->spinRate;
    s32 skid;

    skid = GetAngleDistance(startYaw, startHeading);
    if (skid >= 0x600) {
        car->speed = WrapSigned32((int64_t)car->speed * 990) / 1000;
    }
    UpdateLaunchTyreVoice(car, skid);
    ConsumeInitialLaunchEnergy(car, skid, spinMagnitude);

    if (drive->launchEnergy > 0) {
        UpdatePoweredLaunch(car, spinMagnitude);
    } else {
        UpdateDepletedLaunch(car, spinMagnitude);
    }

    FinishLaunchFrame(car, spinMagnitude);
}
