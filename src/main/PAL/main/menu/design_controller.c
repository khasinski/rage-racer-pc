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
