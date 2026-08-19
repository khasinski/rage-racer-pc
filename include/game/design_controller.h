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

typedef enum DesignModePhase {
    DESIGN_MODE_PAINT_DENIED = -1,
    DESIGN_MODE_ACTIVE = 0,
    DESIGN_MODE_TO_TEAM_LOGO = 1,
    DESIGN_MODE_TO_TEAM_NAME = 2,
    DESIGN_MODE_TO_PAINT = 3,
    DESIGN_MODE_BACK = 4
} DesignModePhase;

typedef struct DesignModeState {
    DesignModePhase phase;
    s32 selection;
} DesignModeState;

typedef struct DesignModeInput {
    u16 pressed;
    u8 paintAllowed;
} DesignModeInput;

typedef enum DesignModeEffect {
    DESIGN_MODE_EFFECT_NONE,
    DESIGN_MODE_EFFECT_MOVE,
    DESIGN_MODE_EFFECT_OPEN_TEAM_LOGO,
    DESIGN_MODE_EFFECT_OPEN_TEAM_NAME,
    DESIGN_MODE_EFFECT_OPEN_PAINT,
    DESIGN_MODE_EFFECT_PAINT_DENIED,
    DESIGN_MODE_EFFECT_BACK,
    DESIGN_MODE_EFFECT_DISMISS_DENIED
} DesignModeEffect;

typedef struct DesignModeResult {
    DesignModeEffect effect;
} DesignModeResult;

DesignMenuInputResult DesignMenuHandleInput(s32 selection, u16 pressed);
DesignModeResult DesignModeReduce(
    DesignModeState *state, const DesignModeInput *input);

#endif
