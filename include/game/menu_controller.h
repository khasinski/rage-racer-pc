#ifndef GAME_MENU_CONTROLLER_H
#define GAME_MENU_CONTROLLER_H

#include "common.h"
#include "game/pad.h"

typedef enum MenuAction {
    MENU_ACTION_NONE,
    MENU_ACTION_CONFIRM,
    MENU_ACTION_CANCEL
} MenuAction;

typedef struct MenuCursorResult {
    s32 selection;
    u8 moved;
} MenuCursorResult;

typedef struct MenuSession {
    s32 selection;
    s32 itemCount;
    u32 enabledMask;
} MenuSession;

typedef struct MenuSessionCommands {
    MenuAction action;
    u8 moved;
    u8 moveCount;
} MenuSessionCommands;

/* enabledMask uses one bit per row.  Passing 0 enables every row. */
MenuCursorResult MenuCursorMove(s32 selection, s32 itemCount,
                                s32 direction, u32 enabledMask);
MenuAction MenuResolveAction(u16 pressed, u16 confirmMask, u16 cancelMask);
MenuSessionCommands MenuSessionStep(MenuSession *session, s32 direction,
                                    u16 pressed);
MenuSessionCommands MenuSessionStepVertical(MenuSession *session,
                                            u16 pressed);
s32 MenuViewIsSettled(s32 current, s32 target, s32 tolerance);
s32 MenuExitIsReady(s32 outgoingProgress, s32 viewOffset,
                    s32 minimumViewOffset);

#endif
