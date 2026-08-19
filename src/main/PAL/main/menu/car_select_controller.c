#include "game/car_select_controller.h"
#include "game/menu_dialog_controller.h"
#include "game/pad.h"

static s32 StepSelection(s32 selection, s32 lastSelection, u16 pressed,
                         u8 *moveCount) {
    if ((pressed & PAD_UP) != 0) {
        selection = selection > 0 ? selection - 1 : lastSelection;
        (*moveCount)++;
    }
    if ((pressed & PAD_DOWN) != 0) {
        selection = selection < lastSelection ? selection + 1 : 0;
        (*moveCount)++;
    }
    return selection;
}

CarSelectInputResult CarSelectHandleInput(s32 selection, s32 grandPrixMode,
                                          s32 shopCarIndex,
                                          s32 upgradesAvailable,
                                          s32 maxClassReached,
                                          s32 requiredClass,
                                          u16 pressed) {
    CarSelectInputResult result;
    s32 lastSelection = grandPrixMode != 0 ? 4 : 2;

    result.moveCount = 0;
    result.command = CAR_SELECT_COMMAND_NONE;
    result.selection = StepSelection(
        selection, lastSelection, pressed, &result.moveCount);
    if ((pressed & PAD_CONFIRM) != 0) {
        switch (result.selection) {
        case 0:
            result.command = CAR_SELECT_COMMAND_START_RACE;
            break;
        case 1:
            result.command = CAR_SELECT_COMMAND_CUSTOMIZE;
            break;
        case 2:
            result.command = grandPrixMode == 0
                ? CAR_SELECT_COMMAND_BACK
                : (shopCarIndex != -1
                    ? CAR_SELECT_COMMAND_CAR_SHOP
                    : CAR_SELECT_COMMAND_CAR_SHOP_UNAVAILABLE);
            break;
        case 3:
            result.command = upgradesAvailable != 0 &&
                             maxClassReached >= requiredClass
                ? CAR_SELECT_COMMAND_ENGINEER_SHOP
                : CAR_SELECT_COMMAND_ENGINEER_SHOP_UNAVAILABLE;
            break;
        case 4:
            result.command = CAR_SELECT_COMMAND_BACK;
            break;
        }
    } else if ((pressed & PAD_CANCEL) != 0) {
        result.command = CAR_SELECT_COMMAND_BACK;
    }
    return result;
}

CarSelectScreenResult CarSelectReduceInput(
    const CarSelectScreenState *state,
    const CarSelectScreenInput *input) {
    CarSelectScreenResult result;

    result.state = *state;
    result.command = CAR_SELECT_COMMAND_NONE;
    result.moveCount = 0;
    if (state->phase == CAR_SELECT_ACTIVE) {
        CarSelectInputResult menu = CarSelectHandleInput(
            state->selection, input->grandPrixMode, input->shopCarIndex,
            input->upgradesAvailable, input->maxClassReached,
            input->requiredClass, input->pressed);
        result.state.selection = menu.selection;
        result.command = menu.command;
        result.moveCount = menu.moveCount;
    } else if (state->phase < CAR_SELECT_ACTIVE &&
               (input->pressed & (PAD_CONFIRM | PAD_CANCEL)) != 0) {
        result.state.phase = CAR_SELECT_ACTIVE;
    }
    return result;
}

CustomizeInputResult CustomizeHandleInput(s32 selection, s32 grandPrixMode,
                                          s32 transmissionAvailable,
                                          u16 pressed) {
    CustomizeInputResult result;
    s32 lastSelection = grandPrixMode != 0 ? 3 : 2;

    result.moveCount = 0;
    result.command = CUSTOMIZE_COMMAND_NONE;
    result.selection = StepSelection(
        selection, lastSelection, pressed, &result.moveCount);
    if ((pressed & PAD_CONFIRM) != 0) {
        if (result.selection == 0) {
            result.command = CUSTOMIZE_COMMAND_TIRES;
        } else if (result.selection == 1) {
            result.command = transmissionAvailable != 0
                ? CUSTOMIZE_COMMAND_TRANSMISSION
                : CUSTOMIZE_COMMAND_TRANSMISSION_UNAVAILABLE;
        } else if (result.selection == lastSelection) {
            result.command = CUSTOMIZE_COMMAND_BACK;
        } else if (result.selection == 2) {
            result.command = CUSTOMIZE_COMMAND_DESIGN;
        }
    } else if ((pressed & PAD_CANCEL) != 0) {
        result.command = CUSTOMIZE_COMMAND_BACK;
    }
    return result;
}

CustomizeScreenResult CustomizeReduceInput(
    const CustomizeScreenState *state,
    const CustomizeScreenInput *input) {
    CustomizeScreenResult result;

    result.state = *state;
    result.command = CUSTOMIZE_COMMAND_NONE;
    result.moveCount = 0;
    result.effects = CUSTOMIZE_EFFECT_NONE;

    if (state->phase == CUSTOMIZE_ACTIVE) {
        CustomizeInputResult menu = CustomizeHandleInput(
            state->selection, input->grandPrixMode,
            input->transmissionAvailable, input->pressed);
        result.state.selection = menu.selection;
        result.command = menu.command;
        result.moveCount = menu.moveCount;
        return result;
    }

    if (state->phase == CUSTOMIZE_TIRE_PROMPT) {
        MenuDialogInputResult dialog = MenuDialogHandleRange(
            state->modalCursor, 0, 4, 1, 0,
            input->pressed, input->pressed);
        result.state.modalCursor = dialog.value;
        result.moveCount = dialog.moveCount;
        if (dialog.confirmed) {
            result.state.phase = CUSTOMIZE_CONFIRM_TIRES;
            result.state.confirmTimer = 0x23;
            result.effects |= CUSTOMIZE_EFFECT_ACCEPT;
        }
        if (dialog.cancelled) {
            result.state.phase = CUSTOMIZE_ACTIVE;
            result.effects |= CUSTOMIZE_EFFECT_CANCEL;
        }
        return result;
    }

    if (state->phase == CUSTOMIZE_TRANSMISSION_PROMPT) {
        MenuDialogInputResult dialog = MenuDialogHandleBinary(
            state->modalCursor, 0, 1, input->pressed);
        result.state.modalCursor = dialog.value;
        result.moveCount = dialog.moveCount;
        if (dialog.confirmed) {
            result.state.phase = CUSTOMIZE_CONFIRM_TRANSMISSION;
            result.state.confirmTimer = 0x23;
            result.effects |= CUSTOMIZE_EFFECT_ACCEPT |
                              CUSTOMIZE_EFFECT_APPLY_TRANSMISSION;
        }
        if (dialog.cancelled) {
            result.state.phase = CUSTOMIZE_ACTIVE;
            result.effects |= CUSTOMIZE_EFFECT_CANCEL;
        }
        return result;
    }

    if (state->phase == CUSTOMIZE_TRANSMISSION_UNAVAILABLE &&
        (input->pressed & (PAD_CONFIRM | PAD_CANCEL)) != 0) {
        result.state.phase = CUSTOMIZE_CLOSE_UNAVAILABLE;
    }
    return result;
}

CustomizeScreenState CustomizeTickConfirmTimer(
    const CustomizeScreenState *state) {
    CustomizeScreenState result = *state;
    if ((result.phase == CUSTOMIZE_CONFIRM_TIRES ||
         result.phase == CUSTOMIZE_CONFIRM_TRANSMISSION) &&
        result.confirmTimer > 0) {
        result.confirmTimer--;
    }
    return result;
}

CustomizeScreenState CustomizeFinishPopup(
    const CustomizeScreenState *state) {
    CustomizeScreenState result = *state;
    if (result.phase == CUSTOMIZE_CLOSE_UNAVAILABLE ||
        result.phase == CUSTOMIZE_CONFIRM_TIRES ||
        result.phase == CUSTOMIZE_CONFIRM_TRANSMISSION) {
        result.phase = CUSTOMIZE_ACTIVE;
    }
    return result;
}
