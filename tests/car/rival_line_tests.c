#include "game/rival.h"
#include <stdio.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    TrackRacingLineHint hints[TRACK_RACING_LINE_HINT_COUNT] = {0};
    hints[0] = (TrackRacingLineHint){.start = 64, .end = 128,
        .minHeight = -10, .maxHeight = 10, .heightAdjustment = 3};
    hints[1].start = -1;
    GameCarRuntime car = {.trackProgress = 96 * 16};
    StepRivalLine(&car, 0, hints);
    CHECK(car.aiLateralOffset == 3);
    car.aiLateralOffset = -10;
    StepRivalLine(&car, 0, hints);
    CHECK(car.aiLateralOffset == -10);
    car.aiLateralOffset = 10;
    StepRivalLine(&car, 0, hints);
    CHECK(car.aiLateralOffset == 10);
    car.aiLateralOffset = 0;
    car.nearbyCarCount = 1;
    StepRivalLine(&car, 0, hints);
    CHECK(car.aiLateralOffset == 0);
    car.nearbyCarCount = 0;
    StepRivalLine(&car, RIVAL_CONTENDER_COUNT, hints);
    CHECK(car.aiLateralOffset == 0);
    car.trackProgress = 129 * 16;
    StepRivalLine(&car, 0, hints);
    CHECK(car.racingLineHintIndex == 0); /* Sentinel wraps the next hint. */
    car.trackProgress = 63 * 16;
    StepRivalLine(&car, 0, hints);
    CHECK(car.aiLateralOffset == 0);
    hints[1] = (TrackRacingLineHint){.start = 0, .end = 32,
        .minHeight = -10, .maxHeight = 10, .heightAdjustment = 5};
    car.trackProgress = 16 * 16;
    car.racingLineHintIndex = 1;
    StepRivalLine(&car, 0, hints);
    CHECK(car.racingLineHintIndex == 0 && car.aiLateralOffset == 5);
    GameCarRuntime saved = car;
    StepRivalLine(&car, -1, hints);
    StepRivalLine(&car, RACE_CAR_SLOT_COUNT, hints);
    StepRivalLine(&car, 0, NULL);
    StepRivalLine(NULL, 0, hints);
    CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
    StepRivalLine(&car, 0, hints);
    StepRivalLine(&saved, 0, hints);
    CHECK(memcmp(&car, &saved, sizeof(car)) == 0);
    car = (GameCarRuntime){.trackProgress = 96 * 16,
        .racingLineHintIndex = TRACK_RACING_LINE_HINT_COUNT};
    StepRivalLine(&car, 0, hints);
    CHECK(car.racingLineHintIndex == 0 && car.aiLateralOffset == 3);
    car.racingLineHintIndex = TRACK_RACING_LINE_HINT_COUNT - 1;
    car.trackProgress = 129 * 16;
    StepRivalLine(&car, 0, hints);
    CHECK(car.racingLineHintIndex == 0);
    return 0;
}
