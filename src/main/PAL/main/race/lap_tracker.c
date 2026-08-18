#include "game/lap_tracker.h"

LapTrackerDecision LapTrackerEvaluate(const LapTrackerInput *input) {
    LapTrackerDecision decision = {0, input->currentLap, 0};

    if (input->currentLap > input->targetLapCount) return decision;
    if (input->currentLap * input->trackLength > input->totalProgress)
        return decision;

    decision.crossedLine = 1;
    decision.nextLap = input->currentLap + 1;
    decision.finished = decision.nextLap == input->targetLapCount + 1;
    return decision;
}
