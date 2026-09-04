#include "game/menu.h"
#include "game/render_internal.h"

enum {
    COURSE_CARD_CENTER_X = 0xE4,
    COURSE_CARD_CENTER_Y = 0x58,
    COURSE_CARD_MIN_ANGLE = 11,
    COURSE_CARD_FACE_CHANGE_ANGLE = 1024,
    COURSE_CARD_FIRST_PLACE_FACE = 1,
    COURSE_CARD_SECOND_PLACE_FACE = 2,
    COURSE_CARD_THIRD_PLACE_FACE = 3,
};

static s32 CourseCardDepth(s32 face) {
    switch (face) {
    case COURSE_CARD_FIRST_PLACE_FACE:
        return 0x1F8;
    case COURSE_CARD_SECOND_PLACE_FACE:
        return 0x20B;
    case COURSE_CARD_THIRD_PLACE_FACE:
        return 0x1F9;
    default:
        return -1;
    }
}

static s32 AdvanceCourseCardSpin(void) {
    int64_t delta = (int64_t)g_CourseCardSpinTarget - g_CourseCardSpin;
    int64_t step = 0;

    if (delta > 0) {
        step = (delta + 12) / 12;
    } else if (delta < 0) {
        step = (delta - 12) / 12;
    }
    g_CourseCardSpin = (s32)((int64_t)g_CourseCardSpin + step);
    return g_CourseCardSpin / 1000;
}

void UpdateAndDrawCourseCard(void) {
    MenuProjectedVertex projected[4];
    Matrix rotation;
    s32 angle;
    s32 depth;
    s32 i;

    angle = AdvanceCourseCardSpin();
    if (angle < COURSE_CARD_MIN_ANGLE) {
        angle = COURSE_CARD_MIN_ANGLE;
    }
    /* A place of zero means no result yet and 0xff marks a locked course.
     * Both deliberately select no face, just like any damaged save value. */
    if (angle < COURSE_CARD_FACE_CHANGE_ANGLE &&
        g_CourseCardPendingGrade >= 0) {
        g_CourseCardFace = g_CourseCardPendingGrade;
        g_CourseCardPendingGrade = -1;
    }

    depth = CourseCardDepth(g_CourseCardFace);
    if (depth < 0) {
        return;
    }

    BuildRotMatrixY(&rotation, angle);
    for (i = 0; i < 4; i++) {
        ApplyMatrixSV(&rotation, &g_CourseCardVerts[i],
                      projected[i].components);
    }

    GameDrawTexturedQuad(
        RENDER_OT_BASE + 1,
        projected[0].position.x + COURSE_CARD_CENTER_X,
        projected[0].position.y + COURSE_CARD_CENTER_Y,
        projected[1].position.x + COURSE_CARD_CENTER_X,
        projected[1].position.y + COURSE_CARD_CENTER_Y,
        projected[2].position.x + COURSE_CARD_CENTER_X,
        projected[2].position.y + COURSE_CARD_CENTER_Y,
        projected[3].position.x + COURSE_CARD_CENTER_X,
        projected[3].position.y + COURSE_CARD_CENTER_Y,
        0xA0, 0x70, 0xDF, 0x70, 0xA0, 0xBF, 0xDF, 0xBF,
        0x7F, 0x7F, 0x7F, (u16)depth, 0, 0, 0x1C);
}
