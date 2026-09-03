#include "game/fmv.h"
#include "game/menu.h"
#include "game/records_internal.h"
#include "game/replay_internal.h"
#include "game/round_screen_internal.h"
#include "game/screens.h"
#include "game/state.h"
#include "game/scene.h"

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
    [GAME_SCENE_BOOT_LOGO] = UpdateBootLogoScene,
    [GAME_SCENE_ENTER_FRONTEND] = EnterFrontend,
    [GAME_SCENE_ENTER_TITLE] = EnterTitleScreen,
    [GAME_SCENE_FRONTEND] = UpdateFrontend,
    [GAME_SCENE_FMV] = UpdateFmv,
    [GAME_SCENE_INIT_MENU] = InitMenuMode,
    [GAME_SCENE_RETURN_FROM_CLASS_FMV] = ReturnFromClassFmv,
    [GAME_SCENE_MENU] = UpdateMenuMode,
    [GAME_SCENE_ENTER_ROUND] = EnterRoundScreen,
    [GAME_SCENE_ROUND] = UpdateRoundScreen,
    [GAME_SCENE_ENTER_RACE] = EnterRaceScene,
    [GAME_SCENE_RACE] = UpdateRaceScene,
    [GAME_SCENE_ENTER_LOST_RACE] = EnterLostRaceScreen,
    [GAME_SCENE_LOST_RACE] = UpdateLostRaceScreen,
    [GAME_SCENE_ENTER_RACE_END] = EnterRaceEndScreen,
    [GAME_SCENE_RACE_END] = UpdateRaceEndScreen,
    [GAME_SCENE_REPLAY] = UpdateReplayScene,
    [GAME_SCENE_ENTER_PRIZE] = EnterPrizeScreen,
    [GAME_SCENE_PRIZE] = UpdatePrizeMoneyScreen,
    [GAME_SCENE_ENTER_RECORD_ENTRY] = EnterRecordEntry,
    [GAME_SCENE_RECORD_ENTRY] = UpdateRecordEntry,
    [GAME_SCENE_ENTER_ATTRACT] = EnterAttractScene,
    [GAME_SCENE_OPTION] = UpdateOptionScene,
    [GAME_SCENE_ENTER_MEMORY_CARD] = EnterMemoryCardMenu,
    [GAME_SCENE_ENTER_MEMORY_CARD_LOAD] = EnterMemoryCardMenuFromLoad,
    [GAME_SCENE_MEMORY_CARD] = UpdateMemoryCardMenu,
    [GAME_SCENE_ENTER_BGM_SELECT] = EnterBgmSelectScreen,
    [GAME_SCENE_BGM_SELECT] = UpdateBgmSelectScene,
    [GAME_SCENE_ENTER_ATTRACT_DEMO] = EnterAttractDemo,
    [GAME_SCENE_ATTRACT_DEMO] = UpdateAttractDemoScene,
    [GAME_SCENE_ENTER_PROLOGUE] = EnterPrologue,
    [GAME_SCENE_PROLOGUE] = TickPrologueStep,
    [GAME_SCENE_RETURN_FROM_ENDING_FMV] = ReturnFromEndingFmv,
    [GAME_SCENE_ENDING_STILL] = UpdateEndingStill,
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
