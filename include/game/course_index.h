#ifndef GAME_COURSE_INDEX_H
#define GAME_COURSE_INDEX_H

#include "common.h"

extern s32 g_CourseIndex;

static inline s32 RageCourseSlot(s32 physicalCourse) {
    return physicalCourse & 3;
}
static inline s32 RageCourseSeries(s32 physicalCourse) {
    return (physicalCourse >> 2) & 1;
}
static inline s32 RagePhysicalCourseIndex(void) { return g_CourseIndex; }
static inline s32 RageSeriesCourseIndex(void) {
    return RageCourseSlot(g_CourseIndex);
}

#endif
