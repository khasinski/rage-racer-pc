#ifndef GAME_COURSE_SELECT_INTERNAL_H
#define GAME_COURSE_SELECT_INTERNAL_H

#include "common.h"
#include "game/prize_money.h"

extern s32 g_CourseSelectScrollProgress;

enum {
    COURSE_SELECT_SCROLL_MAX = 0x1FC,
    COURSE_SELECT_REST_SLIDE = -0x28,
};

typedef struct CourseClassHeaderSprite {
    u16 width;
    u16 textureU;
    u16 textureV;
} CourseClassHeaderSprite;

typedef struct CourseLabelSprites {
    u16 prefixTextureU;
    u16 nameWidth;
    u16 nameTextureU;
    u16 nameTextureV;
    u16 distanceWidth;
    u16 distanceTextureU;
} CourseLabelSprites;

/* Returns zero and clears the descriptor when the class has no header in the
 * selected Grand Prix series. */
int GetCourseClassHeaderSprite(s32 seriesSelection, s32 classIndex,
                               CourseClassHeaderSprite *sprite);
int GetCourseLabelSprites(s32 courseIndex, CourseLabelSprites *sprites);

static inline int CourseSelectClassIndexValid(s32 classIndex) {
    return (u32)classIndex < GRAND_PRIX_PRIZE_CLASS_COUNT;
}

typedef struct CourseSelectScrollFrame {
    s32 progress;
    s32 slide;
} CourseSelectScrollFrame;

static inline CourseSelectScrollFrame AdvanceCourseSelectScroll(
    s32 progress, s32 step) {
    CourseSelectScrollFrame frame;
    int64_t updated = (int64_t)progress + step;

    if (updated < 0) updated = 0;
    if (updated > COURSE_SELECT_SCROLL_MAX) {
        updated = COURSE_SELECT_SCROLL_MAX;
    }
    progress = (s32)updated;

    frame.progress = progress;
    if (step < 0) {
        u32 remaining = (u32)(COURSE_SELECT_SCROLL_MAX - progress);
        frame.slide = (s32)(remaining * remaining / 2048) +
                      COURSE_SELECT_REST_SLIDE;
    } else {
        frame.slide = COURSE_SELECT_REST_SLIDE;
    }
    return frame;
}

#endif
