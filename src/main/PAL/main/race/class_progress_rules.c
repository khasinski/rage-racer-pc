#include "game/race_internal.h"

enum {
    BEGINNER_CLASS_COURSE_COUNT = 3,
    ADVANCED_CLASS_COURSE_COUNT = 4,
    COURSE_UNLOCK_CLASS = 2,
    STANDARD_SERIES_FINAL_CLASS = 4,
    EXTRA_SERIES_FINAL_CLASS = 5,
    CLASS_RECORD_NO_UNLOCK = -1,
    CLASS_GRADE_COURSE_COUNT = 4,
    CLASS_GRADE_DISQUALIFIED = 0,
    UNUSED_COURSE_PLACE = 0xFF,
};

s32 GrandPrixCourseCount(s32 classIndex) {
    return classIndex < COURSE_UNLOCK_CLASS
        ? BEGINNER_CLASS_COURSE_COUNT
        : ADVANCED_CLASS_COURSE_COUNT;
}

s32 NextUnlockedClassRecord(s32 classRecordIndex) {
    /* Finishing standard GP class 4 opens extra GP class 0 (record 6).
     * Finishing extra GP class 4 opens standard GP class 5 (record 5), while
     * standard class 5 is the branch point and opens no single next record. */
    if (classRecordIndex == 4) {
        return 6;
    }
    if (classRecordIndex == 5) {
        return CLASS_RECORD_NO_UNLOCK;
    }
    if (classRecordIndex == 10) {
        return 5;
    }
    return classRecordIndex + 1;
}

s32 IsFinalGrandPrixClass(s32 extraSeries, s32 classIndex) {
    return classIndex == (extraSeries
        ? EXTRA_SERIES_FINAL_CLASS
        : STANDARD_SERIES_FINAL_CLASS);
}

s32 PrizeCountStep(s32 amount, s32 frameCount) {
    s32 step = amount / frameCount;
    return step > 0 ? step : 1;
}

s32 CountClassWins(const ScoreRecord *records, s32 recordCount) {
    s32 wins = 0;
    s32 record;

    for (record = 0; record < recordCount; record++) {
        if (records[record].place == 1) {
            wins++;
        }
    }
    return wins;
}

s32 ComputeClassGradeForPlaces(const u8 bestPlaces[4], s32 unlockPending) {
    s32 placeTotal = 0;
    s32 course;

    if (unlockPending != 0) {
        return CLASS_GRADE_DISQUALIFIED;
    }

    for (course = 0; course < CLASS_GRADE_COURSE_COUNT; course++) {
        s32 place = bestPlaces[course];

        placeTotal += place == UNUSED_COURSE_PLACE ? 1 : place;
    }

    placeTotal -= CLASS_GRADE_COURSE_COUNT - 1;
    return placeTotal < CLASS_GRADE_COURSE_COUNT
        ? placeTotal
        : CLASS_GRADE_DISQUALIFIED;
}
