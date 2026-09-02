#include "game/course_select_internal.h"

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
    CHECK(frame.progress == 12 && frame.slide == 0);

    frame = AdvanceCourseSelectScroll(0x1F8, 16);
    CHECK(frame.progress == 0x1FC && frame.slide == 0);

    frame = AdvanceCourseSelectScroll(0x1FC, -4);
    CHECK(frame.progress == 0x1F8 && frame.slide == 0);

    frame = AdvanceCourseSelectScroll(100, -200);
    CHECK(frame.progress == 0);
    CHECK(frame.slide == (u16)(0x1FC * 0x1FC / 2048));

    puts("course select scroll tests passed");
    return 0;
}
