#include "game/angle.h"
#include "game/car_drive.h"
#include "game/integer.h"

enum {
    STANDING_START_YAW_RESPONSE = 5,
    STANDING_START_LOW_RPM = 2000,
    STANDING_START_LOW_THROTTLE = 127,
    STANDING_START_BASE_GRIP = 32,
    STANDING_START_LOW_RPM_GRIP_BONUS = 1000,
    STANDING_START_SPEED_DAMPING = 10,
    TRIG_FIXED_ONE = 4096,
    TRAVEL_VELOCITY_DIVISOR = 256,
    BODY_VELOCITY_DIVISOR = 16384,
    PEDAL_INPUT_FULL = 256,
    BRAKE_SPIN_REDUCTION_SCALE = 2,
    VERTICAL_BOUNCE_RANDOM_MASK = 3,
    LATERAL_BOUNCE_RANDOM_MASK = 7,
};

static void AlignStandingStartVelocity(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 bodySin;
    s32 bodyCos;
    s32 alongBody;

    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        GetAngleDelta(car->bodyYaw, drive->targetHeading) /
            STANDING_START_YAW_RESPONSE);
    UpdateCarTravelVelocity(AsRivalCar(car));

    bodySin = SinAngle(car->bodyYaw);
    bodyCos = CosAngle(car->bodyYaw);
    drive->accelPos = WrapSigned32(
        (int64_t)SinAngle(car->headingAngle) * car->speed) /
        TRAVEL_VELOCITY_DIVISOR;
    drive->brakePos = WrapSigned32(
        (int64_t)CosAngle(car->headingAngle) * car->speed) /
        TRAVEL_VELOCITY_DIVISOR;
    alongBody = WrapSigned32(
        (int64_t)WrapSigned32((int64_t)bodySin * drive->accelPos) +
        WrapSigned32((int64_t)bodyCos * drive->brakePos)) /
        TRIG_FIXED_ONE;
    drive->accelPos = WrapSigned32(
        (int64_t)bodySin * alongBody) / BODY_VELOCITY_DIVISOR;
    drive->brakePos = WrapSigned32(
        (int64_t)bodyCos * alongBody) / BODY_VELOCITY_DIVISOR;
}

static int UpdateStandingStartWheelspin(GameCarDrive *drive,
                                         s32 verticalRandom, s32 lateralRandom) {
    s32 throttle;
    s32 rpm;
    s32 grip;

    if (drive->standingStartSpin < CAR_STANDING_START_MIN_SPIN) {
        return 0;
    }

    throttle = drive->acceleratorInput.value;
    rpm = drive->engineRpm;
    grip = throttle + STANDING_START_BASE_GRIP;

    drive->standingStartSpin = WrapSigned32(
        (int64_t)drive->standingStartSpin -
        drive->brakeInput * BRAKE_SPIN_REDUCTION_SCALE);
    if (rpm < STANDING_START_LOW_RPM) {
        grip += STANDING_START_LOW_RPM_GRIP_BONUS;
    } else if (rpm > STANDING_START_LOW_RPM &&
               throttle < STANDING_START_LOW_THROTTLE) {
        grip += STANDING_START_LOW_THROTTLE;
    }

    drive->standingStartBounceY =
        (verticalRandom & VERTICAL_BOUNCE_RANDOM_MASK) * grip /
        PEDAL_INPUT_FULL;
    drive->standingStartBounceX =
        (lateralRandom & LATERAL_BOUNCE_RANDOM_MASK) * grip /
        PEDAL_INPUT_FULL;
    drive->standingStartSpin = WrapSigned32(
        (int64_t)drive->standingStartSpin - grip);
    return drive->standingStartSpin > 0;
}

static void FinishStandingStart(GameCarDrive *drive) {
    drive->standingStartBounceY = 0;
    drive->standingStartBounceX = 0;
    drive->motionState = CAR_MOTION_DRIVING;
}

int StepCarStandingStart(PlayerCarRuntime *car, s32 verticalRandom,
                          s32 lateralRandom) {
    GameCarDrive *drive = &car->drive;

    AlignStandingStartVelocity(car);

    car->speed /= STANDING_START_SPEED_DAMPING;

    if (UpdateStandingStartWheelspin(drive, verticalRandom, lateralRandom)) {
        return 0;
    }
    FinishStandingStart(drive);
    return 1;
}
