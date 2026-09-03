#include "game/fmv.h"
#include "game/menu.h"
#include "game/records_internal.h"
#include "game/replay_internal.h"
#include "game/round_screen_internal.h"
#include "game/screens.h"
#include "game/state.h"

void EnterRaceScene(void);
void UpdateRaceScene(void);
void EnterPrizeScreen(void);
void UpdatePrizeMoneyScreen(void);
void EnterAttractScene(void);
void EnterMemoryCardMenu(void);
void EnterMemoryCardMenuFromLoad(void);
void UpdateMemoryCardMenu(void);
void EnterBgmSelectScreen(void);
void UpdateBgmSelectScene(void);
void EnterAttractDemo(void);
void UpdateAttractDemoScene(void);
void EnterPrologue(void);
void TickPrologueStep(void);
void UpdateEndingStill(void);

void (*g_NativeGameModeHandlers[OPTION_MODE_COUNT])(void) = {
    UpdateOptionMenuFade,
    UpdateOptionRootMenu,
    UpdateClassRecordMenu,
    UpdateClassRecordBrowse,
    UpdateSoundOptionMenu,
    UpdateSoundSettingAdjust,
    UpdateScreenAdjustScreen,
    UpdateControllerConfigScreen,
    BeginNegconCalibration,
    UpdateNegconNeutralScreen,
    UpdateNegconSteerPlayScreen,
    UpdateNegconMaxTwistScreen,
};

void (*g_SceneHandlers[40])(void) = {
    [1] = UpdateBootLogoScene,
    [2] = EnterFrontend,
    [3] = EnterTitleScreen,
    [4] = UpdateFrontend,
    [5] = UpdateFmv,
    [6] = InitMenuMode,
    [7] = ReturnFromClassFmv,
    [8] = UpdateMenuMode,
    [9] = EnterRoundScreen,
    [10] = UpdateRoundScreen,
    [11] = EnterRaceScene,
    [12] = UpdateRaceScene,
    [13] = EnterLostRaceScreen,
    [14] = UpdateLostRaceScreen,
    [15] = EnterRaceEndScreen,
    [16] = UpdateRaceEndScreen,
    [17] = UpdateReplayScene,
    [18] = EnterPrizeScreen,
    [19] = UpdatePrizeMoneyScreen,
    [20] = EnterRecordEntry,
    [21] = UpdateRecordEntry,
    [22] = EnterAttractScene,
    [23] = UpdateOptionScene,
    [24] = EnterMemoryCardMenu,
    [25] = EnterMemoryCardMenuFromLoad,
    [26] = UpdateMemoryCardMenu,
    [27] = EnterBgmSelectScreen,
    [28] = UpdateBgmSelectScene,
    [29] = EnterAttractDemo,
    [30] = UpdateAttractDemoScene,
    [31] = EnterPrologue,
    [32] = TickPrologueStep,
    [33] = ReturnFromEndingFmv,
    [34] = UpdateEndingStill,
};

void (*g_FrontendDrawHandlers[])(void) = {
    UpdateTitleScreen,
    UpdateMainMenuOpen,
    UpdateMainMenuInput,
    UpdateMainMenuExit,
};

static s32 DrawMenuScreenNoOp(s32 step) {
    (void)step;
    return 0;
}

static void UpdateMenuScreenNoOp(void) {}

/* Retail main.exe stores code addresses in these two tables. They must be
 * expressed as native function pointers instead of copied 32-bit words. */
void (*g_MenuScreenUpdate[MENU_SCREEN_COUNT])(void) = {
    [MENU_SCREEN_BOOTSTRAP] = EnterCourseSelectScreen,
    [MENU_SCREEN_COURSE_SELECT] = UpdateCourseSelectScreen,
    [MENU_SCREEN_RANKING] = UpdateRankingScreen,
    [MENU_SCREEN_ENTER_CAR_SELECT] = EnterCarSelectScreen,
    [MENU_SCREEN_CAR_SELECT] = UpdateCarSelectScreen,
    [MENU_SCREEN_CUSTOMIZE] = UpdateCustomizeScreen,
    [MENU_SCREEN_DESIGN_MODE] = UpdateDesignModeScreen,
    [MENU_SCREEN_TEAM_LOGO] = UpdateTeamLogoScreen,
    [MENU_SCREEN_LOGO_SAMPLE] = UpdateLogoSampleScreen,
    [MENU_SCREEN_TEAM_NAME] = UpdateTeamNameScreen,
    [MENU_SCREEN_PAINT_COLOR] = UpdatePaintColorScreen,
    [MENU_SCREEN_CAR_SHOP] = UpdateCarShopScreen,
    [MENU_SCREEN_ENGINEER_SHOP] = UpdateEngineerShopScreen,
    [MENU_SCREEN_UNUSED] = UpdateMenuScreenNoOp,
};

s32 (*g_MenuScreenDraw[MENU_SCREEN_COUNT])(s32) = {
    [MENU_SCREEN_BOOTSTRAP] = DrawMenuScreenNoOp,
    [MENU_SCREEN_COURSE_SELECT] = DrawCourseSelectScreen,
    [MENU_SCREEN_RANKING] = DrawRankingScreen,
    [MENU_SCREEN_ENTER_CAR_SELECT] = DrawMenuScreenNoOp,
    [MENU_SCREEN_CAR_SELECT] = DrawCarSelectScreen,
    [MENU_SCREEN_CUSTOMIZE] = DrawCustomizeScreen,
    [MENU_SCREEN_DESIGN_MODE] = DrawDesignModeScreen,
    [MENU_SCREEN_TEAM_LOGO] = DrawTeamLogoScreen,
    [MENU_SCREEN_LOGO_SAMPLE] = DrawLogoSampleScreen,
    [MENU_SCREEN_TEAM_NAME] = DrawTeamNameScreen,
    [MENU_SCREEN_PAINT_COLOR] = DrawPaintColorScreen,
    [MENU_SCREEN_CAR_SHOP] = DrawCarShopScreen,
    [MENU_SCREEN_ENGINEER_SHOP] = DrawEngineerShopScreen,
    [MENU_SCREEN_UNUSED] = DrawMenuScreenNoOp,
};
