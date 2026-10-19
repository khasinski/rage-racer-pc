#include <assert.h>

#include "game/course_index.h"

s32 g_CourseIndex;

int main(void) {
    s32 physical;
    for (physical = 0; physical < 8; physical++) {
        g_CourseIndex = physical;
        assert(PhysicalCourseIndex() == physical);
        assert(SeriesCourseIndex() == physical % 4);
        assert(CourseSlot(physical) == physical % 4);
        assert(CourseSeries(physical) == physical / 4);
        assert(CourseLapCount(physical) ==
               (physical % 4 == 3 ? 6 : 3));
    }
    for (physical = EXTRA_COURSE_SLOT_BASE;
         physical < CUSTOM_PHYSICAL_COURSE_COUNT;
         physical++) {
        s32 expectedSlot = physical < EXTRA_COURSE_SLOT_END
                               ? physical
                               : physical - EXTRA_COURSE_SLOT_COUNT;
        s32 expectedSeries = physical < EXTRA_COURSE_SLOT_END ? 0 : 1;
        g_CourseIndex = physical;
        assert(PhysicalCourseIndex() == physical);
        assert(CourseSlot(physical) == expectedSlot);
        assert(CourseSlot(expectedSlot) == expectedSlot);
        assert(CourseSeries(physical) == expectedSeries);
        assert(SeriesCourseIndex() == 0);
        assert(CourseLapCount(physical) == 3);
    }
    return 0;
}
