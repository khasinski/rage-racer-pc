#include "game/race.h"
#include "game/race_internal.h"

#include <stdio.h>

static int s_failures;

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

int main(void) {
    Check("time attack course name", ResultCourseNameY(0), 0x39);
    Check("grand prix course name", ResultCourseNameY(1), 0x3C);

    Check("zero race place", IsValidRaceResultPlace(0), 0);
    Check("first race place", IsValidRaceResultPlace(1), 1);
    Check("third race place", IsValidRaceResultPlace(3), 1);
    Check("fourth race place", IsValidRaceResultPlace(4), 0);

    Check("banner waits for bonus",
          ShouldDrawClassPlaceBanner(
              1, PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM - 1), 0);
    Check("valid class banner",
          ShouldDrawClassPlaceBanner(
              3, PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM), 1);
    Check("invalid class banner",
          ShouldDrawClassPlaceBanner(
              4, PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM), 0);

    Check("standard first name", GrandPrixNameIndex(0, 0), 0);
    Check("standard sixth name", GrandPrixNameIndex(0, 5), 5);
    Check("standard class overflow", GrandPrixNameIndex(0, 6), -1);
    Check("extra first name", GrandPrixNameIndex(1, 0), 6);
    Check("extra fifth name", GrandPrixNameIndex(1, 4), 10);
    Check("extra class overflow", GrandPrixNameIndex(1, 5), -1);
    Check("negative class", GrandPrixNameIndex(0, -1), -1);

    return s_failures != 0;
}
