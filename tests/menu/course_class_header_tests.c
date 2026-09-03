#include "game/course_select_internal.h"

#include <limits.h>
#include <stdio.h>

#define CHECK_HEADER(series, classIndex, expectedWidth, expectedU, expectedV)  \
    do {                                                                       \
        CourseClassHeaderSprite sprite;                                        \
        if (!GetCourseClassHeaderSprite(series, classIndex, &sprite) ||        \
            sprite.width != expectedWidth || sprite.textureU != expectedU ||   \
            sprite.textureV != expectedV) {                                    \
            fprintf(stderr, "wrong header for series %d class %d\n", series, \
                    classIndex);                                                \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    CourseClassHeaderSprite sprite = {1, 2, 3};

    CHECK_HEADER(0, 0, 0x24, 0x00, 0x38);
    CHECK_HEADER(0, 1, 0x20, 0x24, 0x38);
    CHECK_HEADER(0, 2, 0x28, 0x44, 0x38);
    CHECK_HEADER(0, 3, 0x30, 0x6C, 0x38);
    CHECK_HEADER(0, 4, 0x30, 0x9C, 0x38);
    CHECK_HEADER(1, 0, 0x30, 0xCC, 0x38);
    CHECK_HEADER(1, 1, 0x40, 0x00, 0x48);
    CHECK_HEADER(1, 2, 0x3C, 0x40, 0x48);
    CHECK_HEADER(1, 3, 0x28, 0x7C, 0x48);
    CHECK_HEADER(1, 4, 0x20, 0xA4, 0x48);
    CHECK_HEADER(1, 5, 0x28, 0xC4, 0x48);
    CHECK_HEADER(7, 5, 0x28, 0xC4, 0x48);

    if (GetCourseClassHeaderSprite(0, 5, &sprite) || sprite.width != 0 ||
        GetCourseClassHeaderSprite(0, -1, &sprite) || sprite.width != 0 ||
        GetCourseClassHeaderSprite(1, 6, &sprite) || sprite.width != 0 ||
        GetCourseClassHeaderSprite(0, 0, NULL)) {
        puts("invalid class unexpectedly has a header");
        return 1;
    }
    if (!CourseSelectClassIndexValid(0) ||
        !CourseSelectClassIndexValid(GRAND_PRIX_PRIZE_CLASS_COUNT - 1) ||
        CourseSelectClassIndexValid(-1) ||
        CourseSelectClassIndexValid(GRAND_PRIX_PRIZE_CLASS_COUNT) ||
        CourseSelectClassIndexValid(INT_MAX)) {
        puts("invalid class unexpectedly indexes the prize table");
        return 1;
    }

    puts("course class header tests passed");
    return 0;
}
