#ifndef GAME_COURSE_SELECT_INTERNAL_H
#define GAME_COURSE_SELECT_INTERNAL_H

#include "common.h"

extern s32 g_CourseSelectScrollProgress;

typedef struct CourseClassHeaderSprite {
    u16 width;
    u16 textureU;
    u16 textureV;
} CourseClassHeaderSprite;

/* Returns zero and clears the descriptor when the class has no header in the
 * selected Grand Prix series. */
int GetCourseClassHeaderSprite(s32 seriesSelection, s32 classIndex,
                               CourseClassHeaderSprite *sprite);

typedef struct CourseSelectScrollFrame {
    s32 progress;
    u16 slide;
} CourseSelectScrollFrame;

static inline CourseSelectScrollFrame AdvanceCourseSelectScroll(
    s32 progress, s32 step) {
    CourseSelectScrollFrame frame;

    progress += step;
    if (progress < 0) progress = 0;
    if (progress > 0x1FC) progress = 0x1FC;

    frame.progress = progress;
    if (step < 0) {
        u32 remaining = (u32)(0x1FC - progress);
        frame.slide = (u16)(remaining * remaining / 2048);
    } else {
        frame.slide = 0;
    }
    return frame;
}

#endif
