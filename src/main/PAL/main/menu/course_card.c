#include "game/menu.h"
#include "game/render_internal.h"

enum {
    COURSE_CARD_CENTER_X = 0xE4,
    COURSE_CARD_CENTER_Y = 0x58,
    COURSE_CARD_MIN_ANGLE = 11,
    COURSE_CARD_FACE_CHANGE_ANGLE = 1024,
};

static s32 CourseCardDepth(s32 face) {
    switch (face) {
    case 1:
        return 0x1F8;
    case 2:
        return 0x20B;
    case 3:
        return 0x1F9;
    default:
        return -1;
    }
}

static s32 AdvanceCourseCardSpin(void) {
    s32 delta = g_CourseCardSpinTarget - g_CourseCardSpin;
    s32 step = 0;

    if (delta > 0) {
        step = (delta + 12) / 12;
    } else if (delta < 0) {
        step = (delta - 12) / 12;
    }
    g_CourseCardSpin += step;
    return g_CourseCardSpin / 1000;
}

void UpdateAndDrawCourseCard(void) {
    SVec vertices[4];
    MenuProjectedVertex projected[4];
    Matrix rotation;
    s32 angle;
    s32 depth;
    s32 i;

    angle = AdvanceCourseCardSpin();
    if (angle < COURSE_CARD_MIN_ANGLE) {
        angle = COURSE_CARD_MIN_ANGLE;
    }
    if (angle < COURSE_CARD_FACE_CHANGE_ANGLE &&
        g_CourseCardPendingGrade >= 0) {
        g_CourseCardFace = g_CourseCardPendingGrade;
        g_CourseCardPendingGrade = -1;
    }

    depth = CourseCardDepth(g_CourseCardFace);
    if (depth < 0) {
        return;
    }

    for (i = 0; i < 4; i++) {
        vertices[i] = g_CourseCardVerts[i];
    }
    BuildRotMatrixY(&rotation, angle);
    for (i = 0; i < 4; i++) {
        ApplyMatrixSV(&rotation, &vertices[i], projected[i].components);
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
