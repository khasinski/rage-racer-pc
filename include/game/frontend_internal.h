#ifndef GAME_FRONTEND_INTERNAL_H
#define GAME_FRONTEND_INTERNAL_H

#include "common.h"

extern u32 g_FrontendIdleTimer;

typedef enum TitleMenuItem {
    TITLE_MENU_GRAND_PRIX,
    TITLE_MENU_EXTRA_GRAND_PRIX,
    TITLE_MENU_TIME_ATTACK,
    TITLE_MENU_LOAD_SAVE,
    TITLE_MENU_OPTIONS,
    TITLE_MENU_ITEM_COUNT,
} TitleMenuItem;

s32 MoveTitleMenuSelection(s32 selection, s32 direction,
                           int extraGrandPrixUnlocked);

#endif
