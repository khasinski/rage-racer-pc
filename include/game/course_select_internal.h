#ifndef GAME_COURSE_SELECT_INTERNAL_H
#define GAME_COURSE_SELECT_INTERNAL_H

#include "common.h"

typedef union CourseSelectScrollState {
    s32 value;
} CourseSelectScrollState;

extern CourseSelectScrollState g_CourseSelectScrollState;
/* Host storage. Retail aliased this onto g_CourseSelectScrollState with an
 * asm label, which the port never honoured: common.h defined asm() away, so
 * the two have always been separate words here. */
extern s32 g_CourseSelectScrollValue;

#endif
