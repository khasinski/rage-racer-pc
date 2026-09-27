#include "game/angle.h"
#include "game/car_control.h"
#include "game/integer.h"

enum SteeringDirection {
    STEERING_RIGHT = 1,
    STEERING_LEFT = 2
};

enum {
    STEERING_FULL_LOCK = 4096,
    DIGITAL_STEERING_STEP = 1536,
    DIGITAL_STEERING_RELEASE_DIVISOR = 3,
    STEERING_ROLL_STEP = 6,
    BODY_ROLL_DAMPING_NUMERATOR = 7,
    BODY_ROLL_DAMPING_DIVISOR = 8,
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

static void UpdateDigitalSteering(PlayerCarRuntime *car, const SteeringInput *input) {
    GameCarDrive *drive = &car->drive;
    s32 steerPosition = drive->steerPos;

    if (input->left) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_LEFT);
        if (steerPosition > 0) {
            drive->steerPos = 0;
        } else if (steerPosition > -STEERING_FULL_LOCK) {
            drive->steerPos = steerPosition - DIGITAL_STEERING_STEP;
        }
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity - STEERING_ROLL_STEP);
    } else if (input->right) {
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

static void UpdateNegconSteering(PlayerCarRuntime *car, s32 requestedSteer) {
    GameCarDrive *drive = &car->drive;
    s32 steerPosition = drive->steerPos;

    if (requestedSteer < 0) {
        drive->trackCurveMode = CurveModeForDriver(car, STEERING_LEFT);
        if (steerPosition > 0) {
            drive->steerPos = 0;
            car->steeringAngle = 0;
        } else if (requestedSteer - NEGCON_APPROACH_WINDOW < steerPosition) {
            drive->steerPos -=
                CosAngle(steerPosition / NEGCON_RESPONSE_ANGLE_DIVISOR) /
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
                CosAngle(steerPosition / NEGCON_RESPONSE_ANGLE_DIVISOR) /
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
        ANGLE_THREE_QUARTER_TURN - car->trackHeading);
    s32 headingCorrection = GetAngleDelta(car->bodyYaw, wantedHeading) *
                            AUTO_STEER_HEADING_RESPONSE;
    s32 lateralCorrection = STEERING_FULL_LOCK - CosAngle(WrapSigned32(
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

void UpdateCarSteering(PlayerCarRuntime *car, const SteeringInput *input) {
    switch (input->mode) {
    case STEERING_DIGITAL:
        UpdateDigitalSteering(car, input);
        break;
    case STEERING_ANALOG:
        UpdateNegconSteering(car, input->angle);
        break;
    case STEERING_AUTOMATIC:
        UpdateAutomaticSteering(car);
        break;
    case STEERING_CENTER:
    default:
        CenterSteering(car);
        break;
    }

    if (car->speed < AUTO_STEER_FAST_SPEED) {
        car->bodyRollVelocity = WrapSigned32(
            (int64_t)car->bodyRollVelocity * car->speed) /
            AUTO_STEER_FAST_SPEED;
    }
}
