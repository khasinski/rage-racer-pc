#ifndef GAME_COURSE_INDEX_H
#define GAME_COURSE_INDEX_H

#include "common.h"

extern s32 g_CourseIndex;

enum {
    COURSE_SLOT_COUNT = 4,
    COURSE_STANDARD_LAPS = 3,
    COURSE_LONG_SLOT = 3,
    COURSE_LONG_LAPS = 6,
};

static inline s32 CourseSlot(s32 physicalCourse) {
    return physicalCourse & (COURSE_SLOT_COUNT - 1);
}
static inline s32 CourseSeries(s32 physicalCourse) {
    return (physicalCourse >> 2) & 1;
}
static inline s32 CourseLapCount(s32 physicalCourse) {
    return CourseSlot(physicalCourse) == COURSE_LONG_SLOT
               ? COURSE_LONG_LAPS
               : COURSE_STANDARD_LAPS;
}
static inline s32 PhysicalCourseIndex(void) { return g_CourseIndex; }
static inline s32 SeriesCourseIndex(void) {
    return CourseSlot(g_CourseIndex);
}

#endif
