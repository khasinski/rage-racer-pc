#include "game/menu.h"
#include "game/race.h"
#include "game/state.h"

enum {
    STANDARD_SERIES_FIRST_COURSE = 0,
    EXTRA_SERIES_FIRST_COURSE = 4,
    STANDARD_SERIES_LAST_COURSE_LOCKED = 2,
    STANDARD_SERIES_LAST_COURSE_UNLOCKED = 3,
    EXTRA_SERIES_LAST_COURSE_LOCKED = 6,
    EXTRA_SERIES_LAST_COURSE_UNLOCKED = 7,
    COURSE_UNLOCK_CLASS = 2,
};

static s32 LastSelectableCourse(s32 extraSeries, s32 maxClassReached) {
    if (extraSeries) {
        return maxClassReached < COURSE_UNLOCK_CLASS
            ? EXTRA_SERIES_LAST_COURSE_LOCKED
            : EXTRA_SERIES_LAST_COURSE_UNLOCKED;
    }
    return maxClassReached < COURSE_UNLOCK_CLASS
        ? STANDARD_SERIES_LAST_COURSE_LOCKED
        : STANDARD_SERIES_LAST_COURSE_UNLOCKED;
}

s32 CanSelectPrevCourse(void) {
    s32 firstCourse = STANDARD_SERIES_FIRST_COURSE;
    s32 lastCourse;

    if (g_GrandPrixMode && g_SeriesSelection) {
        firstCourse = EXTRA_SERIES_FIRST_COURSE;
    }
    if (g_GrandPrixMode) {
        lastCourse = LastSelectableCourse(g_SeriesSelection != 0,
                                          g_GrandPrixClass);
    } else {
        s32 extraSeries = g_ExtraGrandPrixUnlocked != 0;

        lastCourse = LastSelectableCourse(
            extraSeries, g_MaxClassReached[extraSeries]);
    }
    return g_CourseIndex > firstCourse && g_CourseIndex <= lastCourse;
}

s32 CanSelectNextCourse(void) {
    s32 extraSeries;
    s32 firstCourse;
    s32 maxClassReached;

    if (g_GrandPrixMode) {
        extraSeries = g_SeriesSelection != 0;
        firstCourse = extraSeries ? EXTRA_SERIES_FIRST_COURSE
                                  : STANDARD_SERIES_FIRST_COURSE;
        maxClassReached = g_GrandPrixClass;
    } else {
        extraSeries = g_ExtraGrandPrixUnlocked != 0;
        firstCourse = STANDARD_SERIES_FIRST_COURSE;
        maxClassReached = g_MaxClassReached[extraSeries];
    }

    return g_CourseIndex >= firstCourse &&
           g_CourseIndex < LastSelectableCourse(extraSeries, maxClassReached);
}
