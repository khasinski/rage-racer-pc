#include "game/design_controller.h"
#include "game/menu_dialog_controller.h"
#include "game/option_controller.h"
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
    ScreenAdjustState adjust;
    ScreenAdjustResult adjustResult;
    ClassRecordBrowseState browse;
    ClassRecordBrowseResult browseResult;
    NegconCalibrationState calibration;
    NegconCalibrationInput calibrationInput;
    NegconCalibrationResult calibrationResult;
    static const struct {
        DesignModeState initial;
        DesignModeInput input;
        DesignModeState expected;
        DesignModeEffect effect;
    } designModeCases[] = {
        {{DESIGN_MODE_ACTIVE, 0}, {PAD_UP, 1},
         {DESIGN_MODE_ACTIVE, 3}, DESIGN_MODE_EFFECT_MOVE},
        {{DESIGN_MODE_ACTIVE, 3}, {PAD_DOWN, 1},
         {DESIGN_MODE_ACTIVE, 0}, DESIGN_MODE_EFFECT_MOVE},
        {{DESIGN_MODE_ACTIVE, 2}, {PAD_UP | PAD_DOWN, 1},
         {DESIGN_MODE_ACTIVE, 2}, DESIGN_MODE_EFFECT_NONE},
        {{DESIGN_MODE_ACTIVE, 0}, {PAD_CONFIRM, 1},
         {DESIGN_MODE_TO_TEAM_LOGO, 0},
         DESIGN_MODE_EFFECT_OPEN_TEAM_LOGO},
        {{DESIGN_MODE_ACTIVE, 1}, {PAD_CONFIRM, 1},
         {DESIGN_MODE_TO_TEAM_NAME, 1},
         DESIGN_MODE_EFFECT_OPEN_TEAM_NAME},
        {{DESIGN_MODE_ACTIVE, 2}, {PAD_CONFIRM, 1},
         {DESIGN_MODE_TO_PAINT, 2}, DESIGN_MODE_EFFECT_OPEN_PAINT},
        {{DESIGN_MODE_ACTIVE, 2}, {PAD_CONFIRM, 0},
         {DESIGN_MODE_PAINT_DENIED, 2}, DESIGN_MODE_EFFECT_PAINT_DENIED},
        {{DESIGN_MODE_PAINT_DENIED, 2}, {PAD_CANCEL, 0},
         {DESIGN_MODE_ACTIVE, 2}, DESIGN_MODE_EFFECT_DISMISS_DENIED},
        {{DESIGN_MODE_ACTIVE, 3}, {PAD_CONFIRM, 1},
         {DESIGN_MODE_BACK, 3}, DESIGN_MODE_EFFECT_BACK},
        {{DESIGN_MODE_ACTIVE, 1}, {PAD_CANCEL, 1},
         {DESIGN_MODE_BACK, 1}, DESIGN_MODE_EFFECT_BACK},
    };
    u32 caseIndex;

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

    for (caseIndex = 0;
         caseIndex < sizeof(designModeCases) / sizeof(designModeCases[0]);
         caseIndex++) {
        DesignModeState state = designModeCases[caseIndex].initial;
        DesignModeResult result =
            DesignModeReduce(&state, &designModeCases[caseIndex].input);

        EXPECT_EQ(designModeCases[caseIndex].expected.phase, state.phase);
        EXPECT_EQ(designModeCases[caseIndex].expected.selection,
                  state.selection);
        EXPECT_EQ(designModeCases[caseIndex].effect, result.effect);
    }

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

    adjust = (ScreenAdjustState){-10, -31};
    adjustResult = ScreenAdjustReduce(
        &adjust, 0, PAD_LEFT | PAD_UP);
    EXPECT_EQ(-11, adjust.x);
    EXPECT_EQ(-32, adjust.y);
    EXPECT_EQ(1, adjustResult.moved);
    adjustResult = ScreenAdjustReduce(&adjust, PAD_CONFIRM, PAD_LEFT | PAD_UP);
    EXPECT_EQ(-11, adjust.x);
    EXPECT_EQ(-32, adjust.y);
    EXPECT_EQ(OPTION_DECISION_ACCEPT, adjustResult.decision);

    browse = (ClassRecordBrowseState){0, 1};
    browseResult = ClassRecordBrowseReduce(&browse, PAD_LEFT);
    EXPECT_EQ(5, browse.column);
    EXPECT_EQ(0, browse.row);
    EXPECT_EQ(1, browseResult.moved);
    browseResult = ClassRecordBrowseReduce(&browse, PAD_CANCEL);
    EXPECT_EQ(1, browseResult.close);

    calibration = (NegconCalibrationState){1, 10, 0};
    calibrationInput = (NegconCalibrationInput){PAD_RIGHT, 1, 11};
    calibrationResult =
        NegconCalibrationReduce(&calibration, &calibrationInput);
    EXPECT_EQ(2, calibration.value);
    EXPECT_EQ(1, calibrationResult.moved);
    calibrationInput = (NegconCalibrationInput){PAD_CANCEL, 1, 11};
    calibrationResult =
        NegconCalibrationReduce(&calibration, &calibrationInput);
    EXPECT_EQ(1, calibration.nextMode);
    EXPECT_EQ(1, calibration.restoreSettings);
    EXPECT_EQ(OPTION_DECISION_CANCEL, calibrationResult.decision);
    calibration = (NegconCalibrationState){2, 10, 0};
    calibrationInput = (NegconCalibrationInput){0, 0, 11};
    NegconCalibrationReduce(&calibration, &calibrationInput);
    EXPECT_EQ(1, calibration.nextMode);
    EXPECT_EQ(1, calibration.restoreSettings);
    return 0;
}
