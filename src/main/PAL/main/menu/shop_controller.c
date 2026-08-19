#include "game/shop_controller.h"
#include "game/menu_dialog_controller.h"
#include "game/pad.h"

ShopInputResult ShopHandleInput(s32 selection, s32 canOpenPurchase,
                                s32 showNoFundsWhenBlocked, u16 pressed) {
    ShopInputResult result;

    result.selection = selection;
    result.moveCount = 0;
    result.command = SHOP_COMMAND_NONE;
    if ((pressed & PAD_UP) != 0) {
        result.selection = result.selection > 0 ? result.selection - 1 : 1;
        result.moveCount++;
    }
    if ((pressed & PAD_DOWN) != 0) {
        result.selection = result.selection <= 0 ? result.selection + 1 : 0;
        result.moveCount++;
    }
    if ((pressed & PAD_CONFIRM) != 0) {
        if (result.selection == 1) {
            result.command = SHOP_COMMAND_BACK;
        } else if (canOpenPurchase != 0) {
            result.command = SHOP_COMMAND_OPEN_PURCHASE;
        } else if (showNoFundsWhenBlocked != 0) {
            result.command = SHOP_COMMAND_NO_FUNDS;
        }
    } else if ((pressed & PAD_CANCEL) != 0) {
        result.command = SHOP_COMMAND_BACK;
    }
    return result;
}

ShopScreenResult ShopReduceInput(const ShopScreenState *state,
                                 const ShopScreenInput *input) {
    ShopScreenResult result;

    result.state = *state;
    result.command = SHOP_COMMAND_NONE;
    result.moveCount = 0;
    result.effects = SHOP_EFFECT_NONE;
    if (state->phase == SHOP_PHASE_ACTIVE) {
        ShopInputResult menu = ShopHandleInput(
            state->selection, input->canOpenPurchase,
            input->showNoFundsWhenBlocked, input->pressed);
        result.state.selection = menu.selection;
        result.command = menu.command;
        result.moveCount = menu.moveCount;
        return result;
    }

    if (state->phase == SHOP_PHASE_PURCHASE_PROMPT) {
        MenuDialogInputResult dialog = MenuDialogHandleBinary(
            state->modalCursor, 1, 0, input->pressed);
        result.state.modalCursor = dialog.value;
        result.moveCount = dialog.moveCount;
        if (dialog.confirmed) {
            if (state->modalCursor == 0) {
                result.state.phase = SHOP_PHASE_ACTIVE;
                result.effects |= SHOP_EFFECT_CANCEL;
            } else if (input->hasFunds != 0) {
                result.state.phase = SHOP_PHASE_COMMITTING;
                result.state.confirmTimer = 0x23;
                result.effects |= SHOP_EFFECT_ACCEPT |
                                  SHOP_EFFECT_BEGIN_COMMIT;
            } else {
                result.state.phase = SHOP_PHASE_NO_FUNDS;
            }
        }
        if (dialog.cancelled) {
            result.state.phase = SHOP_PHASE_ACTIVE;
            result.effects |= SHOP_EFFECT_CANCEL;
        }
        return result;
    }

    if (state->phase == SHOP_PHASE_NO_FUNDS &&
        (input->pressed & (PAD_CONFIRM | PAD_CANCEL)) != 0) {
        result.state.phase = SHOP_PHASE_ACTIVE;
    }
    return result;
}

ShopScreenState ShopTickConfirmTimer(const ShopScreenState *state) {
    ShopScreenState result = *state;
    if (result.phase == SHOP_PHASE_COMMITTING && result.confirmTimer > 0) {
        result.confirmTimer--;
    }
    return result;
}

ShopScreenState ShopFinishCommit(const ShopScreenState *state) {
    ShopScreenState result = *state;
    if (result.phase == SHOP_PHASE_COMMITTING) {
        result.phase = SHOP_PHASE_COMPLETED;
    }
    return result;
}
