#include "game/rival_update.h"
#include "game/rival_motion.h"

void IntegrateRivalSpeed(GameCarRuntime *car,
                         const RivalUpdatePolicy *policy) {
    GameCarAiBlock *ai = GetCarAiBlock(car);
    RivalMotionState motion = {
        car->speed,
        car->acceleration,
        car->accelerationStep,
        car->accelerationLimit,
        car->boostTimer,
        car->boostAccelerationThreshold,
        ai->boostAcceleration,
        ai->accelerationLimit,
        car->bodyYaw,
        ai->targetYaw
    };

    RivalMotionStep(&motion, policy->enableRaceBoost);
    car->speed = motion.speed;
    car->acceleration = motion.acceleration;
    ai->boostTimer = motion.boostTimer;
    car->bodyYaw = motion.bodyYaw;
}

void IntegrateRivalSpeeds(CarSimulation *simulation,
                          const RivalUpdatePolicy *policy) {
    s32 index;
    for (index = 0; index < simulation->carCount; index++) {
        GameCarRuntime *car = &simulation->cars[index];
        if (car->activeFlag != -1) IntegrateRivalSpeed(car, policy);
    }
}
