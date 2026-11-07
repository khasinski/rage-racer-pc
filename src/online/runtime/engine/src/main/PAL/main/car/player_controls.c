#include "game/car_control.h"
#include "game/integer.h"

enum {
    STEERING_FULL_LOCK = 0x1000,
    NEGCON_STEERING_RELEASE_FRAMES = -10,
    STEERING_INPUT_SCALE_NUMERATOR = 6,
    STEERING_INPUT_SCALE_DENOMINATOR = 5,
    LOW_SPEED_STEERING_THRESHOLD = 256,
    STANDING_START_STEERING_THRESHOLD = 512,
    LOW_SPEED_PRODUCT_DIVISOR = 256,
    DRIVING_HEADING_DIVISOR = 0x10000,
    STANDING_START_HEADING_DIVISOR = 0x20000,
};

void UpdatePlayerSteeringTarget(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 speed = car->speed;
    s32 turn = WrapSigned32(
        (int64_t)(WrapSigned32(
                      (int64_t)drive->steerPos *
                      STEERING_INPUT_SCALE_NUMERATOR) /
                  STEERING_INPUT_SCALE_DENOMINATOR) *
        drive->steeringGrip);
    s32 headingChange;

    if (speed < LOW_SPEED_STEERING_THRESHOLD &&
        drive->motionState == CAR_MOTION_DRIVING) {
        headingChange = WrapSigned32(
                            (int64_t)(turn / LOW_SPEED_PRODUCT_DIVISOR) *
                            speed) /
                        DRIVING_HEADING_DIVISOR;
    } else if (speed < STANDING_START_STEERING_THRESHOLD &&
               drive->motionState == CAR_MOTION_STANDING_START) {
        headingChange = WrapSigned32(
                            (int64_t)(turn / LOW_SPEED_PRODUCT_DIVISOR) *
                            speed) /
                        STANDING_START_HEADING_DIVISOR;
    } else {
        headingChange = turn / DRIVING_HEADING_DIVISOR;
    }
    drive->targetHeading = WrapSigned32(
        (int64_t)drive->targetHeading + headingChange);
}

static void ClampPlayerSteeringAngle(PlayerCarRuntime *car, int usesNegcon) {
    GameCarDrive *drive = &car->drive;

    if (car->steeringAngle >= STEERING_FULL_LOCK) {
        car->steeringAngle = STEERING_FULL_LOCK;
        if (!usesNegcon || drive->steerPos < -STEERING_FULL_LOCK) {
            drive->steerHoldFrames = WrapSigned16(
                (int64_t)drive->steerHoldFrames + 1);
        }
    } else if (car->steeringAngle <= -STEERING_FULL_LOCK) {
        car->steeringAngle = -STEERING_FULL_LOCK;
        if (!usesNegcon || drive->steerPos > STEERING_FULL_LOCK) {
            drive->steerHoldFrames = WrapSigned16(
                (int64_t)drive->steerHoldFrames + 1);
        }
    } else {
        drive->steerHoldFrames = usesNegcon ? NEGCON_STEERING_RELEASE_FRAMES : 0;
    }
}

void UpdateCarControlFeedback(PlayerCarRuntime *car, int analogSteering) {
    UpdateCarWheelRotation(AsRivalCar(car));
    ClampPlayerSteeringAngle(car, analogSteering);
}
