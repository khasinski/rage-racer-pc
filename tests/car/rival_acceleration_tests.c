#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];

s32 GetAngleDelta(s32 from, s32 to) { return to - from; }

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

static GameCarRuntime *Activate(s32 index, s32 acceleration, s32 speed) {
    GameCarRuntime *car = &g_Cars[index];

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

static void ResetCars(void) {
    s32 index;

    memset(g_Cars, 0, sizeof(g_Cars));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].activeFlag = -1;
    }
}

int main(void) {
    GameCarRuntime *normal;
    GameCarRuntime *limited;
    GameCarRuntime *coastingBoost;
    GameCarRuntime *pullingBoost;
    GameCarRuntime *limitedBoost;
    GameCarRuntime *equalAttract;

    ResetCars();
    normal = Activate(0, 10, 100);
    limited = Activate(1, 30, 100);
    coastingBoost = Activate(2, 10, 0x321);
    coastingBoost->boostTimer = 10;
    coastingBoost->boostAccelerationThreshold = 5;
    pullingBoost = Activate(3, 10, 0x321);
    pullingBoost->boostTimer = 4;
    pullingBoost->boostAccelerationThreshold = 5;
    limitedBoost = Activate(4, 30, 100);
    limitedBoost->boostTimer = 4;
    limitedBoost->boostAccelerationThreshold = 5;

    AccelerateRaceRivals();
    CHECK_EQ(normal->acceleration, 15);
    CHECK_EQ(normal->speed, 109);
    CHECK_EQ(normal->bodyYaw, 10);
    CHECK_EQ(limited->acceleration, 20);
    CHECK_EQ(limited->speed, 114);
    CHECK_EQ(coastingBoost->acceleration, 0);
    CHECK_EQ(coastingBoost->speed, 752);
    CHECK_EQ(coastingBoost->boostTimer, 9);
    CHECK_EQ(pullingBoost->acceleration, 13);
    CHECK_EQ(pullingBoost->boostTimer, 3);
    CHECK_EQ(limitedBoost->acceleration, 20);
    CHECK_EQ(limitedBoost->boostTimer, 3);
    CHECK_EQ(g_Cars[5].speed, 0);

    ResetCars();
    normal = Activate(0, 10, 100);
    equalAttract = Activate(1, 20, 100);
    equalAttract->boostTimer = 10;
    AccelerateAttractRivals();
    CHECK_EQ(normal->acceleration, 15);
    CHECK_EQ(normal->speed, 109);
    CHECK_EQ(normal->bodyYaw, 10);
    CHECK_EQ(equalAttract->acceleration, 20);
    CHECK_EQ(equalAttract->speed, 114);
    CHECK_EQ(equalAttract->boostTimer, 10);

    puts("rival acceleration preserves limits, boost branches, and attract");
    return 0;
}
