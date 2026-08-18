#include "game/rival_update.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    RivalUpdatePolicy race = {1, 1, 0, 1};
    RivalUpdatePolicy attract = {0, 0, 1, 0};
    GameCarRuntime car = {0};
    GameCarAiBlock *ai = GetCarAiBlock(&car);
    EXPECT_EQ(1, RivalShouldUpdateTraffic(0, 0, &race));
    EXPECT_EQ(1, RivalShouldUpdateTraffic(4, 0, &race));
    EXPECT_EQ(0, RivalShouldUpdateTraffic(5, 0, &race));
    EXPECT_EQ(1, RivalShouldUpdateTraffic(5, 1, &race));
    EXPECT_EQ(1, RivalShouldUpdateTraffic(10, 1, &attract));

    car.activeFlag = 1;
    car.speed = 1000;
    car.acceleration = 10;
    car.accelerationStep = 3;
    car.accelerationLimit = 12;
    ai->targetYaw = 100;
    IntegrateRivalSpeed(&car, &race);
    EXPECT_EQ(13, car.acceleration);
    EXPECT_EQ(953, car.speed);
    EXPECT_EQ(20, car.bodyYaw);

    car.speed = 0x321;
    car.acceleration = 7;
    car.boostTimer = 3;
    car.boostAccelerationThreshold = 2;
    IntegrateRivalSpeed(&car, &race);
    EXPECT_EQ(0, car.acceleration);
    EXPECT_EQ(2, car.boostTimer);
    return 0;
}
