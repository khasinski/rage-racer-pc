#include "game/race_clock.h"

#define EXPECT_EQ(expected, actual) \
    do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    RaceLapClock clock;

    EXPECT_EQ(1000, RaceClockFramesToMilliseconds(25, 0));
    EXPECT_EQ(1039, RaceClockFramesToMilliseconds(25, 39));
    EXPECT_EQ(9, RaceClockTickCountdown(10, 1));
    EXPECT_EQ(10, RaceClockTickCountdown(10, 0));

    clock = RaceClockTickLap(24, 7);
    EXPECT_EQ(25, clock.frameCount);
    EXPECT_EQ(1007, clock.milliseconds);
    EXPECT_EQ(0, clock.saturated);

    clock = RaceClockTickLap(0xFFFF, 39);
    EXPECT_EQ(0x10000, clock.frameCount);
    EXPECT_EQ(1, clock.saturated);
    EXPECT_EQ(RACE_CLOCK_MAX_TIME_MS, clock.milliseconds);
    EXPECT_EQ(RACE_CLOCK_MAX_TIME_MS,
              RaceClockSaturateMilliseconds(RACE_CLOCK_MAX_TIME_MS + 1));
    return 0;
}
