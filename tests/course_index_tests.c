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
    }
    return 0;
}
