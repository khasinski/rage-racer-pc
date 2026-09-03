#include "game/angle.h"
#include "game/car.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/input_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

enum SteeringDirection {
    STEERING_RIGHT = 1,
    STEERING_LEFT = 2
};

enum {
    PLAYER_CONTROL_RACE_PHASE = 2,
    FINISHED_RACE_PHASE = 4,
    STEERING_FULL_LOCK = 4096,
    DIGITAL_STEERING_STEP = 1536,
    DIGITAL_STEERING_RELEASE_DIVISOR = 3,
    STEERING_ROLL_STEP = 6,
    BODY_ROLL_DAMPING_NUMERATOR = 7,
    BODY_ROLL_DAMPING_DIVISOR = 8,
    NEGCON_STEERING_SCALE = 13 * 512,
    NEGCON_APPROACH_WINDOW = 256,
    NEGCON_RESPONSE_ANGLE_DIVISOR = 8,
    NEGCON_RESPONSE_DIVISOR = 4,
    NEGCON_NEUTRAL_ANGLE_DIVISOR = 2,
    NEGCON_NEUTRAL_POSITION_DIVISOR = 6,
    AUTO_STEER_MIN_SPEED = 81,
    AUTO_STEER_FAST_SPEED = 800,
    AUTO_STEER_SLOW_LATERAL_RESPONSE = 6,
    AUTO_STEER_FAST_LATERAL_RESPONSE = 4,
    AUTO_STEER_HEADING_RESPONSE = 32,
    AUTO_STEER_ROLL_DIVISOR = 128,
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
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity * BODY_ROLL_DAMPING_NUMERATOR) /
            BODY_ROLL_DAMPING_DIVISOR;
    }
}

static void CenterSteering(PlayerCarRuntime *car) {
    car->drive.trackCurveMode = 0;
    car->drive.steerPos = 0;
    car->steeringAngle = 0;
    car->bodyRollVelocity = 0;
}

static void UpdateDigitalSteering(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    u16 held = g_PadHeld;
    s32 steerPosition = drive->steerPos;

    if (held & g_PadButtonMapping[0]) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_LEFT);
        if (steerPosition > 0) {
            drive->steerPos = 0;
        } else if (steerPosition > -STEERING_FULL_LOCK) {
            drive->steerPos = steerPosition - DIGITAL_STEERING_STEP;
        }
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity - STEERING_ROLL_STEP);
    } else if (held & g_PadButtonMapping[1]) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_RIGHT);
        if (steerPosition < 0) {
            drive->steerPos = 0;
        } else if (steerPosition < STEERING_FULL_LOCK) {
            drive->steerPos = steerPosition + DIGITAL_STEERING_STEP;
        }
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity + STEERING_ROLL_STEP);
    } else {
        drive->trackCurveMode = 0;
        drive->steerPos /= DIGITAL_STEERING_RELEASE_DIVISOR;
    }

    car->steeringAngle = WrapSigned32(-(int64_t)drive->steerPos);
    DampBodyRoll(car);
}

static void UpdateNegconSteering(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 requestedSteer =
        (g_NegconSteer * NEGCON_STEERING_SCALE) / GetNegconSteerRange();
    s32 steerPosition = drive->steerPos;

    if (requestedSteer < 0) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_LEFT);
        if (steerPosition > 0) {
            drive->steerPos = 0;
            car->steeringAngle = 0;
        } else if (requestedSteer - NEGCON_APPROACH_WINDOW < steerPosition) {
            drive->steerPos -=
                rcos(steerPosition / NEGCON_RESPONSE_ANGLE_DIVISOR) /
                NEGCON_RESPONSE_DIVISOR;
            car->steeringAngle = WrapSigned32(
                (int64_t)car->steeringAngle + DIGITAL_STEERING_STEP);
        } else {
            drive->steerPos =
                steerPosition / DIGITAL_STEERING_RELEASE_DIVISOR;
        }
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity - STEERING_ROLL_STEP);
    } else if (requestedSteer > 0) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_RIGHT);
        if (steerPosition < 0) {
            drive->steerPos = 0;
            car->steeringAngle = 0;
        } else if (steerPosition < requestedSteer + NEGCON_APPROACH_WINDOW) {
            drive->steerPos +=
                rcos(steerPosition / NEGCON_RESPONSE_ANGLE_DIVISOR) /
                NEGCON_RESPONSE_DIVISOR;
            car->steeringAngle = WrapSigned32(
                (int64_t)car->steeringAngle - DIGITAL_STEERING_STEP);
        } else {
            drive->steerPos =
                steerPosition / DIGITAL_STEERING_RELEASE_DIVISOR;
        }
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity + STEERING_ROLL_STEP);
    } else {
        drive->trackCurveMode = 0;
        car->steeringAngle /= NEGCON_NEUTRAL_ANGLE_DIVISOR;
        drive->steerPos /= NEGCON_NEUTRAL_POSITION_DIVISOR;
    }

    DampBodyRoll(car);
}

static void UpdateAutomaticSteering(PlayerCarRuntime *car) {
    GameCarDrive *drive = &car->drive;
    s32 wantedHeading = WrapSigned32(
        (int64_t)car->facingBackwards * ANGLE_HALF_TURN +
        ANGLE_THREE_QUARTER_TURN - car->trackHeading.value);
    s32 headingCorrection = GetAngleDelta(car->bodyYaw, wantedHeading) *
                            AUTO_STEER_HEADING_RESPONSE;
    s32 lateralCorrection = STEERING_FULL_LOCK - rcos(WrapSigned32(
        (int64_t)car->trackLateralOffset * 2));
    s32 steerPosition;

    lateralCorrection *= car->speed < AUTO_STEER_FAST_SPEED
                             ? AUTO_STEER_SLOW_LATERAL_RESPONSE
                             : AUTO_STEER_FAST_LATERAL_RESPONSE;
    if (car->speed >= AUTO_STEER_MIN_SPEED) {
        if ((car->facingBackwards != 0 && car->trackLateralOffset < 0) ||
            (car->facingBackwards == 0 && car->trackLateralOffset > 0)) {
            lateralCorrection = -lateralCorrection;
        }
        steerPosition = lateralCorrection + headingCorrection;
    } else {
        steerPosition = 0;
    }

    if (steerPosition < -STEERING_FULL_LOCK) {
        steerPosition = -STEERING_FULL_LOCK;
    } else if (steerPosition > STEERING_FULL_LOCK) {
        steerPosition = STEERING_FULL_LOCK;
    }
    drive->steerPos = steerPosition;
    car->steeringAngle = steerPosition;
    car->bodyRollVelocity = steerPosition / AUTO_STEER_ROLL_DIVISOR;
}

void UpdateCarBodyRoll(PlayerCarRuntime *car) {
    if (g_RacePhase < PLAYER_CONTROL_RACE_PHASE) {
        CenterSteering(car);
    } else if (g_RacePhase < FINISHED_RACE_PHASE &&
               g_PlayerAutoSteer == 0) {
        if (g_PadType == PAD_TYPE_DIGITAL) {
            UpdateDigitalSteering(car);
        } else if (g_PadType == PAD_TYPE_NEGCON) {
            UpdateNegconSteering(car);
        } else {
            CenterSteering(car);
        }
    } else {
        UpdateAutomaticSteering(car);
    }

    if (car->speed < AUTO_STEER_FAST_SPEED) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity * car->speed) /
            AUTO_STEER_FAST_SPEED;
    }
}
