#include "game/rival_update.h"
#include "game/angle.h"

void IntegrateRivalSpeed(GameCarRuntime *car,
                         const RivalUpdatePolicy *policy) {
    GameCarAiBlock *ai = GetCarAiBlock(car);

    if (policy->enableRaceBoost && car->boostTimer > 0) {
        if (car->boostAccelerationThreshold < car->boostTimer &&
            car->speed >= 0x321) {
            car->acceleration = 0;
        } else if (ai->accelerationLimit >= car->acceleration) {
            car->acceleration += ai->boostAcceleration;
        } else {
            car->acceleration = ai->accelerationLimit;
        }
        ai->boostTimer--;
    } else if ((policy->enableRaceBoost &&
                car->acceleration <= car->accelerationLimit) ||
               (!policy->enableRaceBoost &&
                car->acceleration < car->accelerationLimit)) {
        car->acceleration += car->accelerationStep;
    } else {
        car->acceleration = car->accelerationLimit;
    }
    car->speed = car->speed * 94 / 100 + car->acceleration;
    car->bodyYaw += GetAngleDelta(car->bodyYaw, ai->targetYaw) / 5;
}

void IntegrateRivalSpeeds(CarSimulation *simulation,
                          const RivalUpdatePolicy *policy) {
    s32 index;
    for (index = 0; index < simulation->carCount; index++) {
        GameCarRuntime *car = &simulation->cars[index];
        if (car->activeFlag != -1) IntegrateRivalSpeed(car, policy);
    }
}
