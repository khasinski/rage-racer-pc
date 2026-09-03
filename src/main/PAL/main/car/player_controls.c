#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/state.h"

enum {
    STEERING_FULL_LOCK = 0x1000,
    NEGCON_STEERING_RELEASE_FRAMES = -10,
};

void UpdatePlayerSteeringTarget(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 speed = car->speed;
    s32 turn = WrapSigned32(
        (int64_t)(WrapSigned32((int64_t)drive->steerPos * 6) / 5) *
        drive->steeringGrip);
    s32 headingChange;

    if (speed < 256 && drive->motionState == CAR_MOTION_DRIVING) {
        headingChange = WrapSigned32((int64_t)(turn / 256) * speed) /
                        0x10000;
    } else if (speed < 512 &&
               drive->motionState == CAR_MOTION_STANDING_START) {
        headingChange = WrapSigned32((int64_t)(turn / 256) * speed) /
                        0x20000;
    } else {
        headingChange = turn / 0x10000;
    }
    drive->targetHeading = WrapSigned32(
        (int64_t)drive->targetHeading + headingChange);
}

static void ClampPlayerSteeringAngle(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    int usesNegcon = g_PadType == PAD_TYPE_NEGCON;

    if (car->steeringAngle >= STEERING_FULL_LOCK) {
        car->steeringAngle = STEERING_FULL_LOCK;
        if (!usesNegcon || drive->steerPos < -STEERING_FULL_LOCK) {
            g_SteerHoldFrames = WrapSigned16(
                (int64_t)g_SteerHoldFrames + 1);
        }
    } else if (car->steeringAngle <= -STEERING_FULL_LOCK) {
        car->steeringAngle = -STEERING_FULL_LOCK;
        if (!usesNegcon || drive->steerPos > STEERING_FULL_LOCK) {
            g_SteerHoldFrames = WrapSigned16(
                (int64_t)g_SteerHoldFrames + 1);
        }
    } else {
        g_SteerHoldFrames = usesNegcon ? NEGCON_STEERING_RELEASE_FRAMES : 0;
    }
}

void UpdatePlayerControlFeedback(PlayerCarRuntime *car) {
    UpdateCarWheelRotation(AsRivalCar(car));
    ClampPlayerSteeringAngle(car);
}
