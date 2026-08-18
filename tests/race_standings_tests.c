#include "game/race_standings.h"

#define EXPECT_EQ(expected, actual) \
    do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    const RaceCompetitorProgress competitors[] = {
        {1200, 1}, {900, 1}, {1500, 0}, {1000, 1}, {1001, 1}
    };

    EXPECT_EQ(3, RaceStandingsCalculatePosition(1000, competitors, 5));
    EXPECT_EQ(1, RaceStandingsCalculatePosition(2000, competitors, 5));
    EXPECT_EQ(5, RaceStandingsCalculatePosition(0, competitors, 5));
    EXPECT_EQ(1, RaceStandingsCalculatePosition(1000, competitors, 0));
    return 0;
}
