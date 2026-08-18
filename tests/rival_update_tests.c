#include "game/rival_update.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    RivalUpdatePolicy race = {1, 1, 0, 1};
    RivalUpdatePolicy attract = {0, 0, 1, 0};
    EXPECT_EQ(1, RivalShouldUpdateTraffic(0, 0, &race));
    EXPECT_EQ(1, RivalShouldUpdateTraffic(4, 0, &race));
    EXPECT_EQ(0, RivalShouldUpdateTraffic(5, 0, &race));
    EXPECT_EQ(1, RivalShouldUpdateTraffic(5, 1, &race));
    EXPECT_EQ(1, RivalShouldUpdateTraffic(10, 1, &attract));
    return 0;
}
