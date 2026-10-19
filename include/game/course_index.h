#ifndef GAME_COURSE_INDEX_H
#define GAME_COURSE_INDEX_H

#include "common.h"

extern s32 g_CourseIndex;

enum {
    COURSE_SLOT_COUNT = 4,
    PHYSICAL_COURSE_COUNT = COURSE_SLOT_COUNT * 2,
    EXTRA_COURSE_SLOT_BASE = PHYSICAL_COURSE_COUNT,
    EXTRA_COURSE_SLOT_COUNT = 32,
    EXTRA_COURSE_SLOT_END = EXTRA_COURSE_SLOT_BASE + EXTRA_COURSE_SLOT_COUNT,
    CUSTOM_PHYSICAL_COURSE_COUNT =
        EXTRA_COURSE_SLOT_BASE + EXTRA_COURSE_SLOT_COUNT * 2,
    COURSE_STANDARD_LAPS = 3,
    COURSE_LONG_SLOT = 3,
    COURSE_LONG_LAPS = 6,
};

static inline s32 CourseSlot(s32 physicalCourse) {
    if (physicalCourse >= EXTRA_COURSE_SLOT_BASE) {
        return EXTRA_COURSE_SLOT_BASE +
               ((physicalCourse - EXTRA_COURSE_SLOT_BASE) &
                (EXTRA_COURSE_SLOT_COUNT - 1));
    }
    return physicalCourse & (COURSE_SLOT_COUNT - 1);
}
static inline s32 CourseSeries(s32 physicalCourse) {
    if (physicalCourse >= EXTRA_COURSE_SLOT_BASE) {
        return ((physicalCourse - EXTRA_COURSE_SLOT_BASE) >> 5) & 1;
    }
    return (physicalCourse >> 2) & 1;
}
static inline s32 CourseLapCount(s32 physicalCourse) {
    return CourseSlot(physicalCourse) == COURSE_LONG_SLOT
               ? COURSE_LONG_LAPS
               : COURSE_STANDARD_LAPS;
}
static inline s32 PhysicalCourseIndex(void) { return g_CourseIndex; }
static inline s32 SeriesCourseIndexFor(s32 physicalCourse) {
    s32 slot = CourseSlot(physicalCourse);
    return (u32)slot < COURSE_SLOT_COUNT ? slot : 0;
}
static inline s32 SeriesCourseIndex(void) {
    return SeriesCourseIndexFor(g_CourseIndex);
}

#endif
