#include "game/car.h"
#include "game/car_internal.h"
#include "game/state.h"

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

    if (car->steeringAngle >= 4096) {
        car->steeringAngle = 4096;
        if (!usesNegcon || drive->steerPos < -4096) {
            g_SteerHoldFrames++;
        }
    } else if (car->steeringAngle < -4095) {
        car->steeringAngle = -4096;
        if (!usesNegcon || drive->steerPos > 4096) {
            g_SteerHoldFrames++;
        }
    } else {
        g_SteerHoldFrames = usesNegcon ? -10 : 0;
    }
}

void UpdatePlayerControlFeedback(PlayerCarRuntime *car) {
    UpdateCarWheelRotation(GetPlayerCarRuntime(car));
    ClampPlayerSteeringAngle(car);
}
