#include "game/course_select_internal.h"

#include <limits.h>
#include <stdio.h>

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CourseSelectScrollFrame frame;

    frame = AdvanceCourseSelectScroll(0, 12);
    CHECK(frame.progress == 12 && frame.slide == COURSE_SELECT_REST_SLIDE);

    frame = AdvanceCourseSelectScroll(0x1F8, 16);
    CHECK(frame.progress == COURSE_SELECT_SCROLL_MAX &&
          frame.slide == COURSE_SELECT_REST_SLIDE);

    frame = AdvanceCourseSelectScroll(0x1FC, -4);
    CHECK(frame.progress == 0x1F8 &&
          frame.slide == COURSE_SELECT_REST_SLIDE);

    frame = AdvanceCourseSelectScroll(100, -200);
    CHECK(frame.progress == 0);
    CHECK(frame.slide ==
          COURSE_SELECT_SCROLL_MAX * COURSE_SELECT_SCROLL_MAX / 2048 +
              COURSE_SELECT_REST_SLIDE);

    frame = AdvanceCourseSelectScroll(COURSE_SELECT_SCROLL_MAX, INT_MAX);
    CHECK(frame.progress == COURSE_SELECT_SCROLL_MAX);
    frame = AdvanceCourseSelectScroll(0, INT_MIN);
    CHECK(frame.progress == 0);

    puts("course select scroll tests passed");
    return 0;
}
