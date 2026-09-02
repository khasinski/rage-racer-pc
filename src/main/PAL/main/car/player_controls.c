#include "game/car.h"
#include "game/car_internal.h"
#include "game/state.h"

enum {
    STEERING_FULL_LOCK = 0x1000,
    NEGCON_STEERING_RELEASE_FRAMES = -10,
};

void UpdatePlayerSteeringTarget(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 speed = car->speed;
    s32 turn = (drive->steerPos * 6) / 5 * drive->steeringGrip;

    if (speed < 256 && drive->motionState == CAR_MOTION_DRIVING) {
        drive->targetHeading += (turn / 256) * speed / 0x10000;
    } else if (speed < 512 &&
               drive->motionState == CAR_MOTION_STANDING_START) {
        drive->targetHeading += (turn / 256) * speed / 0x20000;
    } else {
        drive->targetHeading += turn / 0x10000;
    }
}

static void ClampPlayerSteeringAngle(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    int usesNegcon = g_PadType == PAD_TYPE_NEGCON;

    if (car->steeringAngle >= STEERING_FULL_LOCK) {
        car->steeringAngle = STEERING_FULL_LOCK;
        if (!usesNegcon || drive->steerPos < -STEERING_FULL_LOCK) {
            g_SteerHoldFrames++;
        }
    } else if (car->steeringAngle <= -STEERING_FULL_LOCK) {
        car->steeringAngle = -STEERING_FULL_LOCK;
        if (!usesNegcon || drive->steerPos > STEERING_FULL_LOCK) {
            g_SteerHoldFrames++;
        }
    } else {
        g_SteerHoldFrames = usesNegcon ? NEGCON_STEERING_RELEASE_FRAMES : 0;
    }
}

void UpdatePlayerControlFeedback(PlayerCarRuntime *car) {
    UpdateCarWheelRotation(AsRivalCar(car));
    ClampPlayerSteeringAngle(car);
}
