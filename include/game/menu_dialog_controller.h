#ifndef GAME_MENU_DIALOG_CONTROLLER_H
#define GAME_MENU_DIALOG_CONTROLLER_H

#include "common.h"

typedef struct MenuDialogInputResult {
    s32 value;
    u8 moveCount;
    u8 confirmed;
    u8 cancelled;
} MenuDialogInputResult;

MenuDialogInputResult MenuDialogHandleBinary(s32 value, s32 leftValue,
                                             s32 rightValue, u16 pressed);
MenuDialogInputResult MenuDialogHandleRange(s32 value, s32 minimum,
                                            s32 maximum, s32 leftStep,
                                            s32 wrap, u16 buttons,
                                            u16 pressed);

#endif
