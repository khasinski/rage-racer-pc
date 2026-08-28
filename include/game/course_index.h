#ifndef GAME_COURSE_INDEX_H
#define GAME_COURSE_INDEX_H

#include "common.h"

extern s32 g_CourseIndex;

static inline s32 CourseSlot(s32 physicalCourse) {
    return physicalCourse & 3;
}
static inline s32 CourseSeries(s32 physicalCourse) {
    return (physicalCourse >> 2) & 1;
}
static inline s32 PhysicalCourseIndex(void) { return g_CourseIndex; }
static inline s32 SeriesCourseIndex(void) {
    return CourseSlot(g_CourseIndex);
}

#endif
