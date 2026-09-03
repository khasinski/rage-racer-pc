#include "game/race_internal.h"
#include "game/state.h"

#include <limits.h>
#include <stdio.h>

static int s_failures;

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

int main(void) {
    Check("negative brightness", RaceEndBrightness(-2), 0);
    Check("half brightness", RaceEndBrightness(128), 64);
    Check("saturated brightness", RaceEndBrightness(256), 127);
    Check("large brightness", RaceEndBrightness(555), 127);

    Check("up selects retry", UpdateLostRaceChoice(1, PAD_UP), 0);
    Check("down selects exit", UpdateLostRaceChoice(0, PAD_DOWN), 1);
    Check("up does not wrap", UpdateLostRaceChoice(0, PAD_UP), 0);
    Check("down does not wrap", UpdateLostRaceChoice(1, PAD_DOWN), 1);
    Check("opposite directions cancel",
          UpdateLostRaceChoice(1, PAD_UP | PAD_DOWN), 1);
    Check("negative choice resets", UpdateLostRaceChoice(-1, 0), 0);
    Check("large choice resets", UpdateLostRaceChoice(INT_MAX, 0), 0);

    Check("negative retries use zero digit", LostRaceRetryDigitIndex(-1), 0);
    Check("ordinary retries keep their digit", LostRaceRetryDigitIndex(3), 3);
    Check("excess retries use last digit", LostRaceRetryDigitIndex(6), 5);
    Check("large retries use last digit", LostRaceRetryDigitIndex(99), 5);

    Check("retry returns to race", LostRaceExitScene(0), 11);
    Check("exit returns to title", LostRaceExitScene(1), 6);

    Check("skip before threshold", CanSkipRaceEndScreen(260, PAD_CONFIRM), 0);
    Check("skip at threshold", CanSkipRaceEndScreen(261, PAD_CONFIRM), 1);
    Check("skip needs confirmation", CanSkipRaceEndScreen(555, PAD_UP), 0);

    Check("lost fade starts at zero", NextLostRaceFadeTimer(INT_MIN), 0);
    Check("lost fade advances", NextLostRaceFadeTimer(100), 102);
    Check("lost fade reaches opaque", NextLostRaceFadeTimer(254), 256);
    Check("lost fade stays opaque", NextLostRaceFadeTimer(INT_MAX), 256);

    Check("race end timer stops at zero", NextRaceEndScreenTimer(INT_MIN), 0);
    Check("race end timer advances", NextRaceEndScreenTimer(100), 99);
    Check("race end timer bounds corrupt values",
          NextRaceEndScreenTimer(INT_MAX), 554);

    return s_failures != 0;
}
