#include "game/design_controller.h"
#include "game/pad.h"

DesignMenuInputResult DesignMenuHandleInput(s32 selection, u16 pressed) {
    DesignMenuInputResult result;

    result.selection = selection;
    result.moveCount = 0;
    result.command = DESIGN_MENU_NONE;
    if ((pressed & PAD_UP) != 0) {
        result.selection = result.selection > 0 ? result.selection - 1 : 2;
        result.moveCount++;
    }
    if ((pressed & PAD_DOWN) != 0) {
        result.selection = result.selection < 2 ? result.selection + 1 : 0;
        result.moveCount++;
    }
    if ((pressed & PAD_CONFIRM) != 0) {
        result.command = result.selection == 0 ? DESIGN_MENU_PRIMARY
            : result.selection == 1 ? DESIGN_MENU_SECONDARY
                                    : DESIGN_MENU_BACK;
    } else if ((pressed & PAD_CANCEL) != 0) {
        result.command = DESIGN_MENU_CANCEL;
    }
    return result;
}

DesignModeResult DesignModeReduce(
    DesignModeState *state, const DesignModeInput *input) {
    DesignModeResult result = {DESIGN_MODE_EFFECT_NONE};

    if (state->phase == DESIGN_MODE_PAINT_DENIED) {
        if ((input->pressed & (PAD_CONFIRM | PAD_CANCEL)) != 0) {
            state->phase = DESIGN_MODE_ACTIVE;
            result.effect = DESIGN_MODE_EFFECT_DISMISS_DENIED;
        }
        return result;
    }
    if (state->phase != DESIGN_MODE_ACTIVE) return result;

    if ((input->pressed & PAD_UP) != 0 &&
        (input->pressed & PAD_DOWN) == 0) {
        state->selection = state->selection > 0 ? state->selection - 1 : 3;
        result.effect = DESIGN_MODE_EFFECT_MOVE;
    } else if ((input->pressed & PAD_DOWN) != 0 &&
               (input->pressed & PAD_UP) == 0) {
        state->selection = state->selection < 3 ? state->selection + 1 : 0;
        result.effect = DESIGN_MODE_EFFECT_MOVE;
    }

    if ((input->pressed & PAD_CONFIRM) != 0) {
        switch (state->selection) {
        case 0:
            state->phase = DESIGN_MODE_TO_TEAM_LOGO;
            result.effect = DESIGN_MODE_EFFECT_OPEN_TEAM_LOGO;
            break;
        case 1:
            state->phase = DESIGN_MODE_TO_TEAM_NAME;
            result.effect = DESIGN_MODE_EFFECT_OPEN_TEAM_NAME;
            break;
        case 2:
            if (input->paintAllowed) {
                state->phase = DESIGN_MODE_TO_PAINT;
                result.effect = DESIGN_MODE_EFFECT_OPEN_PAINT;
            } else {
                state->phase = DESIGN_MODE_PAINT_DENIED;
                result.effect = DESIGN_MODE_EFFECT_PAINT_DENIED;
            }
            break;
        default:
            state->phase = DESIGN_MODE_BACK;
            result.effect = DESIGN_MODE_EFFECT_BACK;
            break;
        }
    } else if ((input->pressed & PAD_CANCEL) != 0) {
        state->phase = DESIGN_MODE_BACK;
        result.effect = DESIGN_MODE_EFFECT_BACK;
    }
    return result;
}
