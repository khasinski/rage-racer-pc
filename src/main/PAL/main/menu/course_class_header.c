#include "game/course_select_internal.h"

enum {
    COURSE_CLASS_HEADER_SERIES_COUNT = 2,
};

static const CourseClassHeaderSprite s_classHeaders
    [COURSE_CLASS_HEADER_SERIES_COUNT][GRAND_PRIX_PRIZE_CLASS_COUNT] = {
        {
            {0x24, 0x00, 0x38},
            {0x20, 0x24, 0x38},
            {0x28, 0x44, 0x38},
            {0x30, 0x6C, 0x38},
            {0x30, 0x9C, 0x38},
            {0, 0, 0},
        },
        {
            {0x30, 0xCC, 0x38},
            {0x40, 0x00, 0x48},
            {0x3C, 0x40, 0x48},
            {0x28, 0x7C, 0x48},
            {0x20, 0xA4, 0x48},
            {0x28, 0xC4, 0x48},
        },
};

_Static_assert(sizeof(s_classHeaders[0]) / sizeof(s_classHeaders[0][0]) ==
                   GRAND_PRIX_PRIZE_CLASS_COUNT,
               "course headers must cover every Grand Prix class");

int GetCourseClassHeaderSprite(s32 seriesSelection, s32 classIndex,
                               CourseClassHeaderSprite *sprite) {
    s32 series = seriesSelection != 0;

    if (sprite == NULL) {
        return 0;
    }
    *sprite = (CourseClassHeaderSprite){0};
    if (!CourseSelectClassIndexValid(classIndex)) {
        return 0;
    }
    *sprite = s_classHeaders[series][classIndex];
    return sprite->width != 0;
}
