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
    return 0;
}
