#include "common.h"
#include "game/car.h"
#include "game/rival.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

static GameCarRuntime *Activate(GameCarRuntime *cars, s32 index, s32 acceleration, s32 speed) {
    GameCarRuntime *car = &cars[index];

    car->activeFlag = 0;
    car->acceleration = acceleration;
    car->accelerationStep = 5;
    car->accelerationLimit = 20;
    car->boostAcceleration = 3;
    car->speed = speed;
    car->bodyYaw = 0;
    car->targetYaw = 50;
    return car;
}

static void ResetCars(GameCarRuntime cars[RACE_CAR_SLOT_COUNT]) {
    s32 index;

    memset(cars, 0, sizeof(*cars) * RACE_CAR_SLOT_COUNT);
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        cars[index].activeFlag = -1;
    }
}

int main(void) {
    GameCarRuntime cars[RACE_CAR_SLOT_COUNT];
    GameCarRuntime *normal;
    GameCarRuntime *limited;
    GameCarRuntime *coastingBoost;
    GameCarRuntime *pullingBoost;
    GameCarRuntime *limitedBoost;
    GameCarRuntime *equalRace;
    GameCarRuntime *equalAttract;

    ResetCars(cars);
    normal = Activate(cars, 0, 10, 100);
    limited = Activate(cars, 1, 30, 100);
    coastingBoost = Activate(cars, 2, 10, 0x321);
    coastingBoost->boostTimer = 10;
    coastingBoost->boostAccelerationThreshold = 5;
    pullingBoost = Activate(cars, 3, 10, 0x321);
    pullingBoost->boostTimer = 4;
    pullingBoost->boostAccelerationThreshold = 5;
    limitedBoost = Activate(cars, 4, 30, 100);
    limitedBoost->boostTimer = 4;
    limitedBoost->boostAccelerationThreshold = 5;
    equalRace = Activate(cars, 5, 20, 100);

    for (int i = 0; i < RACE_CAR_SLOT_COUNT; i++) StepRivalAcceleration(&cars[i], 1);
    CHECK_EQ(normal->acceleration, 15);
    CHECK_EQ(normal->speed, 109);
    CHECK_EQ(normal->bodyYaw, 10);
    CHECK_EQ(limited->acceleration, 20);
    CHECK_EQ(limited->speed, 114);
    CHECK_EQ(coastingBoost->acceleration, 0);
    CHECK_EQ(coastingBoost->speed, 752);
    CHECK_EQ(coastingBoost->boostTimer, 9);
    CHECK_EQ(coastingBoost->brakeInput, 0);
    CHECK_EQ(normal->brakeInput, 0);
    CHECK_EQ(pullingBoost->acceleration, 13);
    CHECK_EQ(pullingBoost->boostTimer, 3);
    CHECK_EQ(limitedBoost->acceleration, 20);
    CHECK_EQ(limitedBoost->boostTimer, 3);
    CHECK_EQ(equalRace->acceleration, 25);
    CHECK_EQ(equalRace->speed, 119);
    CHECK_EQ(cars[6].speed, 0);
    CHECK_EQ(cars[6].bodyYaw, 0);

    ResetCars(cars);
    normal = Activate(cars, 0, 30, INT_MAX);
    for (int i = 0; i < RACE_CAR_SLOT_COUNT; i++) StepRivalAcceleration(&cars[i], 1);
    CHECK_EQ(normal->acceleration, 20);
    CHECK_EQ(normal->speed, 20);

    ResetCars(cars);
    normal = Activate(cars, 0, 10, 100);
    equalAttract = Activate(cars, 1, 20, 100);
    equalAttract->boostTimer = 10;
    for (int i = 0; i < RACE_CAR_SLOT_COUNT; i++) StepRivalAcceleration(&cars[i], 0);
    CHECK_EQ(normal->acceleration, 15);
    CHECK_EQ(normal->speed, 109);
    CHECK_EQ(normal->bodyYaw, 10);
    CHECK_EQ(equalAttract->acceleration, 20);
    CHECK_EQ(equalAttract->speed, 114);
    CHECK_EQ(equalAttract->boostTimer, 10);

    ResetCars(cars);
    normal = Activate(cars, 0, 20, 1000);
    for (int i = 0; i < RACE_CAR_SLOT_COUNT; i++) StepRivalAcceleration(&cars[i], 1);
    CHECK_EQ(normal->brakeInput, 256);
    normal->accelerationLimit = 100;
    for (int i = 0; i < RACE_CAR_SLOT_COUNT; i++) StepRivalAcceleration(&cars[i], 1);
    CHECK_EQ(normal->brakeInput, 0);
    normal->speed = 1000;
    normal->accelerationLimit = 20;
    for (int i = 0; i < RACE_CAR_SLOT_COUNT; i++) StepRivalAcceleration(&cars[i], 0);
    CHECK_EQ(normal->brakeInput, 256);
    normal->speed = 0;
    for (int i = 0; i < RACE_CAR_SLOT_COUNT; i++) StepRivalAcceleration(&cars[i], 0);
    CHECK_EQ(normal->brakeInput, 0);

    /* A car step has no access to other rooms; restoring it repeats the step. */
    const GameCarRuntime untouched = cars[1];
    GameCarRuntime restored = *normal;
    StepRivalAcceleration(normal, 1);
    StepRivalAcceleration(&restored, 1);
    CHECK_EQ(memcmp(normal, &restored, sizeof(restored)), 0);
    CHECK_EQ(memcmp(&cars[1], &untouched, sizeof(untouched)), 0);
    normal->activeFlag = -1;
    restored = *normal;
    StepRivalAcceleration(normal, 1);
    StepRivalAcceleration(NULL, 1);
    CHECK_EQ(memcmp(normal, &restored, sizeof(restored)), 0);
    normal->activeFlag = 0;
    normal->bodyYaw = 4090;
    normal->targetYaw = 10;
    StepRivalAcceleration(normal, 1);
    CHECK_EQ(normal->bodyYaw, 4093);

    puts("rival acceleration preserves thresholds, boost branches, and attract");
    return 0;
}
