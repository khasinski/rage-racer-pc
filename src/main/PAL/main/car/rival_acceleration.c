#include "game/angle.h"
#include "game/rival.h"
#include "game/integer.h"

enum {
    RIVAL_SPEED_RETENTION_PERCENT = 94,
    RIVAL_BOOST_COAST_SPEED = 0x321,
    RIVAL_YAW_RESPONSE = 5,
    PERCENT_SCALE = 100,
};

static void TurnRivalBodyTowardsTarget(GameCarRuntime *car) {
    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        GetAngleDelta(car->bodyYaw, car->targetYaw) /
            RIVAL_YAW_RESPONSE);
}

static void ApplyRivalSpeedDrag(GameCarRuntime *car) {
    car->speed = WrapSigned32(
        (int64_t)car->speed * RIVAL_SPEED_RETENTION_PERCENT) /
        PERCENT_SCALE;
}

static void IncreaseRivalAcceleration(GameCarRuntime *car, s32 step) {
    if (car->accelerationLimit >= car->acceleration) {
        car->acceleration = WrapSigned32(
            (int64_t)car->acceleration + step);
    } else {
        car->acceleration = car->accelerationLimit;
    }
}

static void AdvanceRivalSpeedAndYaw(GameCarRuntime *car) {
    ApplyRivalSpeedDrag(car);
    car->speed = WrapSigned32(
        (int64_t)car->speed + car->acceleration);
    TurnRivalBodyTowardsTarget(car);
}

static void UpdateRivalBrakeInput(GameCarRuntime *car, int coasting) {
    /* AI approaches its target speed by balancing acceleration against
     * 6% drag. A limit below that equilibrium is its braking command.
     * Ignore tiny rounding corrections and the explicit boost coast phase;
     * collisions and hills never become brake commands. */
    int64_t drag = (int64_t)car->speed *
        (PERCENT_SCALE - RIVAL_SPEED_RETENTION_PERCENT);
    int64_t target = (int64_t)car->accelerationLimit * PERCENT_SCALE;
    car->brakeInput = !coasting && car->speed > 0 &&
        drag > target + PERCENT_SCALE ? 0x100 : 0;
}

static void UpdateRaceRivalAcceleration(GameCarRuntime *car) {
    if (car->boostTimer <= 0) {
        IncreaseRivalAcceleration(car, car->accelerationStep);
        return;
    }

    if (car->boostAccelerationThreshold < car->boostTimer &&
        car->speed >= RIVAL_BOOST_COAST_SPEED) {
        car->acceleration = 0;
    } else {
        IncreaseRivalAcceleration(car, car->boostAcceleration);
    }
    car->boostTimer--;
}

void StepRivalAcceleration(GameCarRuntime *car, int racing) {
    if (car == NULL || car->activeFlag == -1) return;
    if (racing) {
        UpdateRivalBrakeInput(car,
            car->boostTimer > car->boostAccelerationThreshold &&
            car->boostTimer > 0 && car->speed >= RIVAL_BOOST_COAST_SPEED);
        UpdateRaceRivalAcceleration(car);
    } else {
        UpdateRivalBrakeInput(car, 0);
        if (car->acceleration < car->accelerationLimit) {
            car->acceleration = WrapSigned32(
                (int64_t)car->acceleration + car->accelerationStep);
        } else {
            car->acceleration = car->accelerationLimit;
        }
    }
    AdvanceRivalSpeedAndYaw(car);
}
