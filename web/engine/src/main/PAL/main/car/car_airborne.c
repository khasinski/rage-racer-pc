#include "game/angle.h"
#include "game/car_drive.h"
#include "game/integer.h"

enum {
    AIRBORNE_YAW_RESPONSE = 5,
    AIRBORNE_LARGE_YAW = 1537,
    AIRBORNE_INPUT_RELEASED = 128,
    AIRBORNE_DECAY_NUMERATOR = 31,
    AIRBORNE_DECAY_DENOMINATOR = 32,
    AIRBORNE_VELOCITY_SCALE = 256,
    FIXED_TRIG_SCALE = 4096,
    LARGE_YAW_SPEED_NUMERATOR = 4,
    LARGE_YAW_SPEED_DENOMINATOR = 5,
    BODY_LIFT_DECAY_NUMERATOR = 2,
    BODY_LIFT_DECAY_DENOMINATOR = 3,
};

static s32 AbsoluteYawOffset(s32 yawOffset) {
    return yawOffset < 0
        ? WrapSigned32(-(int64_t)yawOffset)
        : yawOffset;
}

static s32 AirborneVelocityComponent(s32 trig, s32 speed) {
    return WrapSigned32((int64_t)trig * speed) / AIRBORNE_VELOCITY_SCALE;
}

static void UpdateAirborneVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 motionHeading;
    s32 alongBody;

    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        GetAngleDelta(car->bodyYaw, drive->targetHeading) /
            AIRBORNE_YAW_RESPONSE);
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = SinAngle(car->bodyYaw);
    bodyCos = CosAngle(car->bodyYaw);
    motionHeading = WrapSigned32(
        (int64_t)car->headingAngle + drive->yawOffset);
    drive->accelPos = AirborneVelocityComponent(
        SinAngle(motionHeading), car->speed);
    drive->brakePos = AirborneVelocityComponent(
        CosAngle(motionHeading), car->speed);
    alongBody = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)bodySin * drive->accelPos) +
        WrapSigned32((int64_t)bodyCos * drive->brakePos)) /
        FIXED_TRIG_SCALE;

    drive->accelPos = WrapSigned32(
        (int64_t)AirborneVelocityComponent(
            SinAngle(drive->launchHeading), drive->launchSpeed) +
        WrapSigned32((int64_t)bodySin * alongBody) / FIXED_TRIG_SCALE);
    drive->brakePos = WrapSigned32(
        (int64_t)AirborneVelocityComponent(
            CosAngle(drive->launchHeading), drive->launchSpeed) +
        WrapSigned32((int64_t)bodyCos * alongBody) / FIXED_TRIG_SCALE);
}

static void UpdateAirborneCoastFrames(GameCarDrive *drive) {
    if (drive->acceleratorLatch != 1 && drive->brakeLatch != 1 &&
        drive->acceleratorInput.value < AIRBORNE_INPUT_RELEASED) {
        drive->coastFrames = WrapSigned32(
            (int64_t)drive->coastFrames + 1);
    } else {
        drive->coastFrames = 0;
    }
}

static void DecayAirborneMotion(GameCarDrive *drive) {
    drive->spinRate = WrapSigned32(
        (int64_t)drive->spinRate * AIRBORNE_DECAY_NUMERATOR) /
        AIRBORNE_DECAY_DENOMINATOR;
    drive->launchSpeed = WrapSigned32(
        (int64_t)drive->launchSpeed * AIRBORNE_DECAY_NUMERATOR) /
        AIRBORNE_DECAY_DENOMINATOR;
    drive->yawOffset = WrapSigned32(
        (int64_t)drive->yawOffset * AIRBORNE_DECAY_NUMERATOR) /
        AIRBORNE_DECAY_DENOMINATOR;
    drive->bodyLiftOffset =
        drive->bodyLiftOffset * BODY_LIFT_DECAY_NUMERATOR /
        BODY_LIFT_DECAY_DENOMINATOR;
}

static void FinishAirborneMotion(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw - drive->spinRate);
    drive->shiftSoundLevel = 0;
    drive->shiftRpmDelta = 0;
    drive->yawOffset = 0;
    drive->launchSpeed = 0;
    drive->motionState = CAR_MOTION_DRIVING;
    drive->bodyLiftOffset = 0;
}

int StepCarAirborne(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;

    UpdateAirborneVelocity(car);
    UpdateAirborneCoastFrames(drive);
    DecayAirborneMotion(drive);

    if (AbsoluteYawOffset(drive->yawOffset) >= AIRBORNE_LARGE_YAW) {
        car->speed = WrapSigned32(
            (int64_t)car->speed * LARGE_YAW_SPEED_NUMERATOR) /
            LARGE_YAW_SPEED_DENOMINATOR;
    }
    if (drive->jumpTimer <= 0) {
        FinishAirborneMotion(car);
        return 1;
    }
    return 0;
}
