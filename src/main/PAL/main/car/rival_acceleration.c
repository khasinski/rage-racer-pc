#include "game/angle.h"
#include "game/car.h"

enum {
    RIVAL_SPEED_RETENTION_PERCENT = 94,
    RIVAL_BOOST_COAST_SPEED = 0x321,
};

static void TurnRivalBodyTowardsTarget(GameCarRuntime *car,
                                       const GameCarAiBlock *ai) {
    car->bodyYaw += GetAngleDelta(car->bodyYaw, ai->targetYaw) / 5;
}

void AccelerateRaceRivals(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        GameCarAiBlock *ai = GetCarAiBlock(car);

        if (car->activeFlag == -1) {
            continue;
        }

        if (car->boostTimer > 0) {
            if (car->boostAccelerationThreshold < car->boostTimer &&
                car->speed >= RIVAL_BOOST_COAST_SPEED) {
                car->acceleration = 0;
            } else if (ai->accelerationLimit >= car->acceleration) {
                car->acceleration += ai->boostAcceleration;
            } else {
                car->acceleration = ai->accelerationLimit;
            }
            ai->boostTimer--;
        } else if (car->accelerationLimit >= car->acceleration) {
            car->acceleration += car->accelerationStep;
        } else {
            car->acceleration = car->accelerationLimit;
        }

        car->speed = car->speed * RIVAL_SPEED_RETENTION_PERCENT / 100;
        car->speed += car->acceleration;
        TurnRivalBodyTowardsTarget(car, ai);
    }
}

void AccelerateAttractRivals(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        GameCarAiBlock *ai = GetCarAiBlock(car);

        if (car->activeFlag == -1) {
            continue;
        }
        if (car->acceleration < car->accelerationLimit) {
            car->acceleration += car->accelerationStep;
        } else {
            car->acceleration = car->accelerationLimit;
        }
        car->speed = car->speed * RIVAL_SPEED_RETENTION_PERCENT / 100 +
                     car->acceleration;
        TurnRivalBodyTowardsTarget(car, ai);
    }
}
