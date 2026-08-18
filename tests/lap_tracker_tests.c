#include "game/lap_tracker.h"

#define EXPECT_EQ(expected, actual) \
    do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    LapTrackerInput input = {1, 3, 999, 1000};
    LapTrackerDecision decision = LapTrackerEvaluate(&input);

    EXPECT_EQ(0, decision.crossedLine);
    EXPECT_EQ(1, decision.nextLap);
    input.totalProgress = 1000;
    decision = LapTrackerEvaluate(&input);
    EXPECT_EQ(1, decision.crossedLine);
    EXPECT_EQ(2, decision.nextLap);
    EXPECT_EQ(0, decision.finished);
    input.currentLap = 3;
    input.totalProgress = 3000;
    decision = LapTrackerEvaluate(&input);
    EXPECT_EQ(1, decision.crossedLine);
    EXPECT_EQ(4, decision.nextLap);
    EXPECT_EQ(1, decision.finished);
    input.currentLap = 4;
    input.totalProgress = 5000;
    decision = LapTrackerEvaluate(&input);
    EXPECT_EQ(0, decision.crossedLine);
    return 0;
}
