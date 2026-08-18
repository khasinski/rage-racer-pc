#include "game/race_result.h"

#define EXPECT_EQ(expected, actual) \
    do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    EXPECT_EQ(RACE_RESULT_IN_PROGRESS, RaceResultFromFinish(0, 1));
    EXPECT_EQ(RACE_RESULT_WON, RaceResultFromFinish(1, 1));
    EXPECT_EQ(RACE_RESULT_WON, RaceResultFromFinish(1, 3));
    EXPECT_EQ(RACE_RESULT_LOST, RaceResultFromFinish(1, 4));
    EXPECT_EQ(RACE_RESULT_LOST, RaceResultFromFinish(1, 12));
    return 0;
}
