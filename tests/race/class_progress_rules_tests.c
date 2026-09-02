#include "common.h"
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
    static const s32 expectedUnlocks[] = {1, 2, 3, 4, 6, -1, 7, 8, 9, 10, 5};
    ScoreRecord records[11] = {0};
    s32 i;

    Check("class 0 courses", GrandPrixCourseCount(0), 3);
    Check("class 1 courses", GrandPrixCourseCount(1), 3);
    Check("class 2 courses", GrandPrixCourseCount(2), 4);
    Check("class 5 courses", GrandPrixCourseCount(5), 4);

    for (i = 0; i < (s32)(sizeof(expectedUnlocks) / sizeof(expectedUnlocks[0]));
         i++) {
        Check("next class record", NextUnlockedClassRecord(i),
              expectedUnlocks[i]);
    }

    Check("standard class 3", IsFinalGrandPrixClass(0, 3), 0);
    Check("standard class 4", IsFinalGrandPrixClass(0, 4), 1);
    Check("standard class 5", IsFinalGrandPrixClass(0, 5), 0);
    Check("extra class 4", IsFinalGrandPrixClass(1, 4), 0);
    Check("extra class 5", IsFinalGrandPrixClass(1, 5), 1);

    Check("zero prize", PrizeCountStep(0, 80), 1);
    Check("small prize", PrizeCountStep(79, 80), 1);
    Check("exact prize", PrizeCountStep(80, 80), 1);
    Check("large prize", PrizeCountStep(801, 80), 10);
    Check("negative prize", PrizeCountStep(-80, 80), 1);

    records[0].place = 1;
    records[3].place = 2;
    records[5].place = 1;
    records[10].place = -1;
    Check("class wins", CountClassWins(records, 11), 2);
    Check("empty class record range", CountClassWins(records, 0), 0);

    return s_failures != 0;
}
