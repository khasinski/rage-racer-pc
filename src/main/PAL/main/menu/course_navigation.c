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

typedef struct CourseSelectionRange {
    s32 first;
    s32 last;
} CourseSelectionRange;

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

static CourseSelectionRange CurrentCourseSelectionRange(void) {
    CourseSelectionRange range;
    s32 extraSeries;
    s32 maxClassReached;

    if (g_GrandPrixMode) {
        extraSeries = g_SeriesSelection != 0;
        range.first = extraSeries ? EXTRA_SERIES_FIRST_COURSE
                                  : STANDARD_SERIES_FIRST_COURSE;
        maxClassReached = g_GrandPrixClass;
    } else {
        extraSeries = g_ExtraGrandPrixUnlocked != 0;
        range.first = STANDARD_SERIES_FIRST_COURSE;
        maxClassReached = g_MaxClassReached[extraSeries];
    }
    range.last = LastSelectableCourse(extraSeries, maxClassReached);
    return range;
}

/* Both arrows must use the same live range: only their edge comparison
 * differs. */
s32 CanSelectPrevCourse(void) {
    const CourseSelectionRange range = CurrentCourseSelectionRange();

    return g_CourseIndex > range.first && g_CourseIndex <= range.last;
}

s32 CanSelectNextCourse(void) {
    const CourseSelectionRange range = CurrentCourseSelectionRange();

    return g_CourseIndex >= range.first && g_CourseIndex < range.last;
}
