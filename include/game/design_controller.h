#ifndef GAME_DESIGN_CONTROLLER_H
#define GAME_DESIGN_CONTROLLER_H

#include "common.h"

typedef enum DesignMenuCommand {
    DESIGN_MENU_NONE,
    DESIGN_MENU_PRIMARY,
    DESIGN_MENU_SECONDARY,
    DESIGN_MENU_BACK,
    DESIGN_MENU_CANCEL
} DesignMenuCommand;

typedef struct DesignMenuInputResult {
    s32 selection;
    u8 moveCount;
    DesignMenuCommand command;
} DesignMenuInputResult;

DesignMenuInputResult DesignMenuHandleInput(s32 selection, u16 pressed);

#endif
