#include <assert.h>

#include "game/course_index.h"

s32 g_CourseIndex;

int main(void) {
    s32 physical;
    for (physical = 0; physical < 8; physical++) {
        g_CourseIndex = physical;
        assert(RagePhysicalCourseIndex() == physical);
        assert(RageSeriesCourseIndex() == physical % 4);
        assert(RageCourseSlot(physical) == physical % 4);
        assert(RageCourseSeries(physical) == physical / 4);
    }
    return 0;
}
