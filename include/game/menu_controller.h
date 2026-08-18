#ifndef GAME_MENU_CONTROLLER_H
#define GAME_MENU_CONTROLLER_H

#include "common.h"

typedef enum MenuAction {
    MENU_ACTION_NONE,
    MENU_ACTION_CONFIRM,
    MENU_ACTION_CANCEL
} MenuAction;

typedef struct MenuCursorResult {
    s32 selection;
    u8 moved;
} MenuCursorResult;

/* enabledMask uses one bit per row.  Passing 0 enables every row. */
MenuCursorResult MenuCursorMove(s32 selection, s32 itemCount,
                                s32 direction, u32 enabledMask);
MenuAction MenuResolveAction(u16 pressed, u16 confirmMask, u16 cancelMask);

#endif
