#include "game/course_select_internal.h"
#include "game/course_index.h"

static const CourseLabelSprites s_courseLabels[COURSE_SLOT_COUNT] = {
    {0x08, 0x54, 0x00, 0x9C, 0x20, 0x44},
    {0x10, 0x4C, 0x54, 0x9C, 0x20, 0x64},
    {0x18, 0x48, 0x00, 0xAC, 0x20, 0x84},
    {0x20, 0x5C, 0xA4, 0x9C, 0x1E, 0xA4},
};

_Static_assert(sizeof(s_courseLabels) / sizeof(s_courseLabels[0]) ==
               COURSE_SLOT_COUNT,
               "course label table must cover every course slot");

int GetCourseLabelSprites(s32 courseIndex, CourseLabelSprites *sprites) {
    if (sprites == NULL) {
        return 0;
    }
    *sprites = (CourseLabelSprites){0};
    if ((u32)courseIndex >= COURSE_SLOT_COUNT) {
        return 0;
    }
    *sprites = s_courseLabels[courseIndex];
    return 1;
}
