#include "game/angle.h"
#include "game/car.h"
#include "game/integer.h"

enum {
    RIVAL_SPEED_RETENTION_PERCENT = 94,
    RIVAL_BOOST_COAST_SPEED = 0x321,
};

static void TurnRivalBodyTowardsTarget(GameCarRuntime *car) {
    car->bodyYaw = WrapSigned32(
        (int64_t)car->bodyYaw +
        GetAngleDelta(car->bodyYaw, car->targetYaw) / 5);
}

static void ApplyRivalSpeedDrag(GameCarRuntime *car) {
    car->speed = WrapSigned32(
        (int64_t)car->speed * RIVAL_SPEED_RETENTION_PERCENT) / 100;
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

void AccelerateRaceRivals(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag == -1) {
            continue;
        }

        UpdateRaceRivalAcceleration(car);
        AdvanceRivalSpeedAndYaw(car);
    }
}

void AccelerateAttractRivals(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag == -1) {
            continue;
        }
        if (car->acceleration < car->accelerationLimit) {
            car->acceleration = WrapSigned32(
                (int64_t)car->acceleration + car->accelerationStep);
        } else {
            car->acceleration = car->accelerationLimit;
        }
        AdvanceRivalSpeedAndYaw(car);
    }
}
