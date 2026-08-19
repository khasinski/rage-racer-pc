#include "game/design_controller.h"
#include "game/menu_dialog_controller.h"
#include "game/shop_controller.h"
#include "game/team_name_controller.h"
#include "game/team_logo_editor_controller.h"
#include "game/pad.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    ShopInputResult shop;
    ShopScreenState shopScreen;
    ShopScreenInput shopInput;
    ShopScreenResult shopResult;
    DesignMenuInputResult design;
    TeamNameInputResult name;
    MenuDialogInputResult dialog;

    shop = ShopHandleInput(0, 1, 0, PAD_CONFIRM);
    EXPECT_EQ(SHOP_COMMAND_OPEN_PURCHASE, shop.command);
    shop = ShopHandleInput(0, 0, 1, PAD_CONFIRM);
    EXPECT_EQ(SHOP_COMMAND_NO_FUNDS, shop.command);
    shop = ShopHandleInput(0, 1, 0, PAD_UP | PAD_DOWN | PAD_CANCEL);
    EXPECT_EQ(0, shop.selection);
    EXPECT_EQ(2, shop.moveCount);
    EXPECT_EQ(SHOP_COMMAND_BACK, shop.command);

    shopScreen = (ShopScreenState){SHOP_PHASE_PURCHASE_PROMPT, 0, 1, 0};
    shopInput = (ShopScreenInput){PAD_CONFIRM, 1, 0, 1};
    shopResult = ShopReduceInput(&shopScreen, &shopInput);
    EXPECT_EQ(SHOP_PHASE_COMMITTING, shopResult.state.phase);
    EXPECT_EQ(0x23, shopResult.state.confirmTimer);
    EXPECT_EQ(SHOP_EFFECT_ACCEPT | SHOP_EFFECT_BEGIN_COMMIT,
              shopResult.effects);
    shopScreen = (ShopScreenState){SHOP_PHASE_PURCHASE_PROMPT, 0, 1, 0};
    shopInput = (ShopScreenInput){PAD_CONFIRM, 1, 0, 0};
    shopResult = ShopReduceInput(&shopScreen, &shopInput);
    EXPECT_EQ(SHOP_PHASE_NO_FUNDS, shopResult.state.phase);
    shopInput.pressed = PAD_CANCEL;
    shopResult = ShopReduceInput(&shopResult.state, &shopInput);
    EXPECT_EQ(SHOP_PHASE_ACTIVE, shopResult.state.phase);
    shopScreen = (ShopScreenState){SHOP_PHASE_PURCHASE_PROMPT, 0, 1, 0};
    shopInput = (ShopScreenInput){PAD_CONFIRM | PAD_CANCEL, 1, 0, 1};
    shopResult = ShopReduceInput(&shopScreen, &shopInput);
    EXPECT_EQ(SHOP_PHASE_ACTIVE, shopResult.state.phase);
    EXPECT_EQ(SHOP_EFFECT_ACCEPT | SHOP_EFFECT_CANCEL |
                  SHOP_EFFECT_BEGIN_COMMIT,
              shopResult.effects);

    design = DesignMenuHandleInput(0, PAD_UP);
    EXPECT_EQ(2, design.selection);
    design = DesignMenuHandleInput(0, PAD_CONFIRM);
    EXPECT_EQ(DESIGN_MENU_PRIMARY, design.command);
    design = DesignMenuHandleInput(1, PAD_CANCEL);
    EXPECT_EQ(DESIGN_MENU_CANCEL, design.command);

    name = TeamNameHandleInput(0, 0, -1, PAD_UP, 0);
    EXPECT_EQ(33, name.cursor);
    name = TeamNameHandleInput(0, 0, -1, PAD_LEFT, 0);
    EXPECT_EQ(10, name.cursor);
    name = TeamNameHandleInput(42, 6, -1, PAD_RIGHT, PAD_CONFIRM);
    EXPECT_EQ(43, name.cursor);
    EXPECT_EQ(TEAM_NAME_COMMAND_BACK, name.command);
    name = TeamNameHandleInput(5, 0, 0, PAD_RIGHT, 0);
    EXPECT_EQ(0, name.moved);

    dialog = MenuDialogHandleBinary(
        1, 0, 1, PAD_LEFT | PAD_RIGHT | PAD_CONFIRM | PAD_CANCEL);
    EXPECT_EQ(1, dialog.value);
    EXPECT_EQ(2, dialog.moveCount);
    EXPECT_EQ(1, dialog.confirmed);
    EXPECT_EQ(1, dialog.cancelled);
    dialog = MenuDialogHandleRange(0, 0, 17, -1, 1, PAD_LEFT, 0);
    EXPECT_EQ(17, dialog.value);
    EXPECT_EQ(1, TeamLogoCycleGuideMode(0));
    EXPECT_EQ(0, TeamLogoCycleGuideMode(2));
    EXPECT_EQ(15, TeamLogoMovePaletteSlot(1, -1));
    EXPECT_EQ(1, TeamLogoMovePaletteSlot(15, 1));
    EXPECT_EQ(2, TeamLogoMoveColorChannel(0, -1));
    EXPECT_EQ(0, TeamLogoMoveColorChannel(2, 1));
    return 0;
}
