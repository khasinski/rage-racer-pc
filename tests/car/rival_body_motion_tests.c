#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];

static int s_startKick[RACE_CAR_SLOT_COUNT];
static int s_bodyKick[RACE_CAR_SLOT_COUNT];
static int s_crestHop[RACE_CAR_SLOT_COUNT];

static s32 CarIndex(const GameCarRuntime *car) {
    return (s32)(car - g_Cars);
}

void StartCarBodyKick(s32 strength, GameCarRuntime *car) {
    if (strength == 1) {
        s_startKick[CarIndex(car)]++;
    }
}

void UpdateCarBodyKick(GameCarRuntime *car) {
    s_bodyKick[CarIndex(car)]++;
}

void UpdateCarCrestHop(GameCarRuntime *car) {
    s_crestHop[CarIndex(car)]++;
}

#define CHECK_EQ(actual, expected) do {                                        \
    if ((actual) != (expected)) {                                               \
        fprintf(stderr, "line %d: %s = %d, expected %d\n", __LINE__, #actual, \
                (int)(actual), (int)(expected));                                \
        return 1;                                                               \
    }                                                                           \
} while (0)

static GameCarRuntime *Activate(s32 index) {
    GameCarRuntime *car = &g_Cars[index];

    car->activeFlag = 0;
    car->y = 100;
    car->bodyPitch = 10 + index;
    car->bodyYaw = 20 + index;
    car->bodyRoll = 30 + index;
    car->bodyRollVelocity = 5;
    return car;
}

int main(void) {
    GameCarRuntime *normal;
    GameCarRuntime *colliding;
    GameCarRuntime *rising;
    GameCarRuntime *crest;
    GameCarRuntime *crestHold;
    GameCarRuntime *falling;
    s32 index;

    memset(g_Cars, 0, sizeof(g_Cars));
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        g_Cars[index].activeFlag = -1;
    }

    normal = Activate(1);
    normal->speed = 100;
    normal->wheelRotation = 10;

    colliding = Activate(2);
    colliding->speed = 1000;
    colliding->collisionFlag = 1;

    rising = Activate(3);
    rising->verticalMotionState = 1;
    rising->verticalMotionRate = -20;

    crest = Activate(4);
    crest->verticalMotionState = 2;
    crest->verticalMotionRate = 10;
    crest->verticalTargetY = 70;

    crestHold = Activate(6);
    crestHold->speed = 1400;
    crestHold->verticalMotionState = 2;
    crestHold->verticalMotionRate = 10;
    crestHold->verticalTargetY = 83;

    falling = Activate(5);
    falling->verticalMotionState = 3;
    falling->verticalMotionTimer = 10;
    falling->verticalMotionRate = 0;
    falling->verticalTargetY = 80;
    falling->verticalPitch = 12;
    falling->verticalRoll = 13;

    UpdateRivalBodyMotion();

    CHECK_EQ(normal->wheelRotation, 310);
    CHECK_EQ(normal->modelPitch, normal->bodyPitch);
    CHECK_EQ(normal->modelYaw, normal->bodyYaw);
    CHECK_EQ(normal->modelRoll, 31);
    CHECK_EQ(normal->bodyRoll, 36);
    CHECK_EQ(normal->modelY, 100);
    CHECK_EQ(s_bodyKick[1], 1);
    CHECK_EQ(s_crestHop[1], 1);

    CHECK_EQ(colliding->wheelRotation, 3000 | 0x1000);
    CHECK_EQ(colliding->speed, 940);
    CHECK_EQ(s_bodyKick[2], 0);
    CHECK_EQ(s_crestHop[2], 0);

    CHECK_EQ(rising->verticalMotionState, 1);
    CHECK_EQ(rising->verticalMotionTimer, 1);
    CHECK_EQ(rising->y, 80);
    CHECK_EQ(s_startKick[3], 0);

    CHECK_EQ(crest->verticalMotionState, 3);
    CHECK_EQ(crest->verticalMotionRate, 1);
    CHECK_EQ(crest->y, 70);

    CHECK_EQ(crestHold->verticalMotionState, 2);
    CHECK_EQ(crestHold->y, 83);
    CHECK_EQ(crestHold->wheelRotation, 0x249 | 0x1000);

    CHECK_EQ(falling->verticalMotionState, 0);
    CHECK_EQ(falling->y, 100);
    CHECK_EQ(falling->verticalPitch, 0);
    CHECK_EQ(falling->verticalRoll, 0);
    CHECK_EQ(s_startKick[5], 1);
    CHECK_EQ(s_bodyKick[5], 1);
    CHECK_EQ(s_crestHop[5], 1);

    CHECK_EQ(g_Cars[0].wheelRotation, 0);
    CHECK_EQ(s_bodyKick[0], 0);

    puts("rival body motion preserves wheels, jumps, landing, and collisions");
    return 0;
}
