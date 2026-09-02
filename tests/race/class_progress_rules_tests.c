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
    static const u8 firstPlace[4] = {1, 1, 1, 1};
    static const u8 shortFirstPlace[4] = {1, 1, 1, 0xFF};
    static const u8 secondPlace[4] = {1, 1, 1, 2};
    static const u8 thirdPlace[4] = {1, 1, 1, 3};
    static const u8 noGrade[4] = {1, 1, 2, 3};
    ScoreRecord records[11] = {0};
    const s32 promotionBonuses[5] = {500, 4800, 20000, 100000, 500000};
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

    Check("standard asset series", GrandPrixAssetSeries(0, 4), 0);
    Check("extra asset series", GrandPrixAssetSeries(1, 4), 1);
    Check("shared finale asset series", GrandPrixAssetSeries(1, 5), 0);

    Check("zero prize", PrizeCountStep(0, 80), 1);
    Check("small prize", PrizeCountStep(79, 80), 1);
    Check("exact prize", PrizeCountStep(80, 80), 1);
    Check("large prize", PrizeCountStep(801, 80), 10);
    Check("negative prize", PrizeCountStep(-80, 80), 1);
    Check("no promotion bonus", PrizeCountStep(0, 250), 1);

    Check("class promotion bonus",
          PromotionBonusForClass(promotionBonuses, 5, 3, 1), 100000);
    Check("no bonus without promotion",
          PromotionBonusForClass(promotionBonuses, 5, 3, 0), 0);
    Check("shared finale has no bonus entry",
          PromotionBonusForClass(promotionBonuses, 5, 5, 1), 0);

    records[0].place = 1;
    records[3].place = 2;
    records[5].place = 1;
    records[10].place = -1;
    Check("class wins", CountClassWins(records, 11), 2);
    Check("empty class record range", CountClassWins(records, 0), 0);

    Check("first-place grade", ComputeClassGradeForPlaces(firstPlace, 0), 1);
    Check("unused fourth course",
          ComputeClassGradeForPlaces(shortFirstPlace, 0), 1);
    Check("second-place grade", ComputeClassGradeForPlaces(secondPlace, 0), 2);
    Check("third-place grade", ComputeClassGradeForPlaces(thirdPlace, 0), 3);
    Check("no class grade", ComputeClassGradeForPlaces(noGrade, 0), 0);
    Check("pending unlock blocks grade",
          ComputeClassGradeForPlaces(firstPlace, 1), 0);

    return s_failures != 0;
}
