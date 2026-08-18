#include "game/car_physics.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    EXPECT_EQ(0, CarUpdatePedalLatch(0, 0x84));
    EXPECT_EQ(1, CarUpdatePedalLatch(0, 0x85));
    EXPECT_EQ(2, CarUpdatePedalLatch(1, 0));
    EXPECT_EQ(2, CarUpdatePedalLatch(2, 0x7C));
    EXPECT_EQ(0, CarUpdatePedalLatch(2, 0x7B));

    EXPECT_EQ(380, CarCalculateGripBudget(0, 0));
    EXPECT_EQ(380, CarCalculateGripBudget(256, 256));
    EXPECT_EQ(380, CarCalculateGripBudget(-1, 0));

    EXPECT_EQ(2, CarCalculateLoadResistance(1, 0x2000, 0));
    EXPECT_EQ(-2, CarCalculateLoadResistance(1, -0x2000, 0));
    EXPECT_EQ(0, CarCalculateLoadResistance(0, 100000, 0));
    EXPECT_EQ(273, CarCalculateLoadResistance(0, 200000, 0));
    EXPECT_EQ(-16, CarCalculateLoadResistance(3, -13000, 0));
    EXPECT_EQ(-6, CarCalculateLoadResistance(0, -13000, 0));

    EXPECT_EQ(100, CarCalculateThrottleAcceleration(200, 128, 1));
    EXPECT_EQ(-100, CarCalculateThrottleAcceleration(-200, 128, 1));
    EXPECT_EQ(0, CarCalculateThrottleAcceleration(200, 128, 0));

    EXPECT_EQ(1100, CarIntegrateEngineRpm(1000, 200, 50, 50, 0, 0));
    EXPECT_EQ(1000, CarIntegrateEngineRpm(1000, 200, 50, 50, 1, 0));
    EXPECT_EQ(0, CarIntegrateEngineRpm(10, 0, 20, 0, 0, 0));
    EXPECT_EQ(0x3A98,
              CarIntegrateEngineRpm(0x3A98, 1, 0, 0, 0, 0));
    return 0;
}
