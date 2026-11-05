#ifndef GAME_FRONTEND_INTERNAL_H
#define GAME_FRONTEND_INTERNAL_H

#include "common.h"

typedef struct Frontend {
    s32 state;
    u32 idleTimer;
    s32 attractCycle;
    s32 attractTimer;
    s32 exitTimer;
    s32 pulse;
    s32 fade;
    s32 menuSlide;
    s32 selection;
} Frontend;

Frontend *MenuFrontend(void);

#include "rage/title_menu.h"


TitleMenuItem MoveTitleMenuSelection(s32 selection, s32 direction,
                                     int extraGrandPrixUnlocked);

#endif
