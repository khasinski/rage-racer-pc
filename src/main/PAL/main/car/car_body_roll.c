#include "game/car.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

enum SteeringDirection {
    STEERING_RIGHT = 1,
    STEERING_LEFT = 2
};

static s32 CurveModeForDriver(const PlayerCarRuntime *car,
                              enum SteeringDirection direction) {
    if (car->facingBackwards != 0) {
        return direction == STEERING_LEFT ? STEERING_RIGHT : STEERING_LEFT;
    }
    return direction;
}

static void DampBodyRoll(PlayerCarRuntime *car) {
    if (car->bodyRollVelocity != 0) {
        car->bodyRollVelocity = (car->bodyRollVelocity * 7) / 8;
    }
}

static void UpdateDigitalSteering(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    u16 held = ReadStablePadHeld();
    s32 steerPosition = drive->steerPos;

    if (held & g_PadButtonMapping[0]) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_LEFT);
        if (steerPosition > 0) {
            drive->steerPos = 0;
        } else if (steerPosition >= -4095) {
            drive->steerPos = steerPosition - 1536;
        }
        car->bodyRollVelocity -= 6;
    } else if (held & g_PadButtonMapping[1]) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_RIGHT);
        if (steerPosition < 0) {
            drive->steerPos = 0;
        } else if (steerPosition < 4096) {
            drive->steerPos = steerPosition + 1536;
        }
        car->bodyRollVelocity += 6;
    } else {
        drive->trackCurveMode = 0;
        drive->steerPos /= 3;
    }

    car->steeringAngle = -drive->steerPos;
    DampBodyRoll(car);
}

static void UpdateNegconSteering(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 requestedSteer = (g_NegconSteer * 13 * 512) /
                         g_NegconSteerRange[g_NegconMaxTwist];
    s32 steerPosition = drive->steerPos;

    if (requestedSteer < 0) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_LEFT);
        if (steerPosition > 0) {
            drive->steerPos = 0;
            car->steeringAngle = 0;
        } else if (requestedSteer - 256 < steerPosition) {
            drive->steerPos -= rcos(steerPosition / 8) / 4;
            car->steeringAngle += 1536;
        } else {
            drive->steerPos = steerPosition / 3;
        }
        car->bodyRollVelocity -= 6;
    } else if (requestedSteer > 0) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_RIGHT);
        if (steerPosition < 0) {
            drive->steerPos = 0;
            car->steeringAngle = 0;
        } else if (steerPosition < requestedSteer + 256) {
            drive->steerPos += rcos(steerPosition / 8) / 4;
            car->steeringAngle -= 1536;
        } else {
            drive->steerPos = steerPosition / 3;
        }
        car->bodyRollVelocity += 6;
    } else {
        drive->trackCurveMode = 0;
        car->steeringAngle /= 2;
        drive->steerPos /= 6;
    }

    DampBodyRoll(car);
}

static void UpdateAutomaticSteering(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 wantedHeading = (car->facingBackwards << 11) + 3072 -
                        car->trackHeading.value;
    s32 headingCorrection = GetAngleDelta(car->bodyYaw, wantedHeading) * 32;
    s32 lateralCorrection = 4096 - rcos(car->trackLateralOffset * 2);
    s32 steerPosition;

    lateralCorrection *= car->speed < 800 ? 6 : 4;
    if (car->speed >= 81) {
        if ((car->facingBackwards != 0 && car->trackLateralOffset < 0) ||
            (car->facingBackwards == 0 && car->trackLateralOffset > 0)) {
            lateralCorrection = -lateralCorrection;
        }
        steerPosition = lateralCorrection + headingCorrection;
    } else {
        drive->steerPos = 0;
        steerPosition = 0;
    }

    if (steerPosition < -4096) {
        steerPosition = -4096;
    } else if (steerPosition > 4096) {
        steerPosition = 4096;
    }
    drive->steerPos = steerPosition;
    car->steeringAngle = steerPosition;
    car->bodyRollVelocity = steerPosition / 128;
}

void UpdateCarBodyRoll(PlayerCarRuntime *car) {
    if (g_RacePhase < 2) {
        car->drive.steerPos = 0;
        car->steeringAngle = 0;
    } else if (g_RacePhase < 4 && g_PlayerAutoSteer == 0) {
        if (g_PadType == 0x41) {
            UpdateDigitalSteering(car);
        } else if (g_PadType == 0x23) {
            UpdateNegconSteering(car);
        } else {
            car->bodyRollVelocity = 0;
            car->drive.steerPos = 0;
        }
    } else {
        UpdateAutomaticSteering(car);
    }

    if (car->speed < 800) {
        car->bodyRollVelocity =
            (car->bodyRollVelocity * car->speed) / 800;
    }
}
