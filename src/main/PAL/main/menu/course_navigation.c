#include "game/course_index.h"
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

    if (g_RaceSession.kind == RACE_SESSION_CUSTOM) {
        range.first = STANDARD_SERIES_FIRST_COURSE;
        range.last = EXTRA_SERIES_LAST_COURSE_UNLOCKED;
        return range;
    }
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

/* A custom race may pick any class, but The Extreme Oval only has course
 * data from class 3 up: below that the disc holds no track for the slot and
 * the race falls back to the first course with a broken grid. */
static s32 CourseIsSelectable(s32 course) {
    if (g_RaceSession.kind == RACE_SESSION_CUSTOM &&
        g_GrandPrixClass < COURSE_UNLOCK_CLASS &&
        CourseSlot(course) == COURSE_LONG_SLOT) {
        return 0;
    }
    return 1;
}

/* The next selectable course in `step`'s direction, or -1 at the edge. */
s32 SelectableCourseStep(s32 course, s32 step) {
    const CourseSelectionRange range = CurrentCourseSelectionRange();

    if (course < range.first || course > range.last || step == 0) return -1;
    for (course += step; course >= range.first && course <= range.last;
         course += step) {
        if (CourseIsSelectable(course)) return course;
    }
    return -1;
}

/* Both arrows must use the same live range: only their direction differs. */
s32 CanSelectPrevCourse(void) {
    return SelectableCourseStep(g_CourseIndex, -1) >= 0;
}

s32 CanSelectNextCourse(void) {
    return SelectableCourseStep(g_CourseIndex, 1) >= 0;
}
