#include "game/boot_defaults_legacy.h"
#include "game/frontend_state_legacy.h"
#include "game/menu_flow.h"
#include "game/menu_state_legacy.h"
#include "game/menu_context.h"

extern s32 g_MenuScreen;
extern s32 g_MenuHandlerIndex;
extern s32 g_MenuHandlerIndex2;

#define EXPECT_EQ(expected, actual) \
    do { if ((expected) != (actual)) return __LINE__; } while (0)

void PlaySoundCue(s32 cue) { (void)cue; }
void StartMenuExitFade(void) {}

static int MenuStateMatches(const MenuState *a, const MenuState *b) {
#define SAME(group, field) (a->group.field == b->group.field)
    return
        SAME(scripts, courseSelectModal) && SAME(scripts, carSelectPopup) &&
        SAME(scripts, customizePopup) && SAME(scripts, teamLogoSubPanel) &&
        SAME(scripts, logoSampleSubPanel) && SAME(scripts, carShopModal) &&
        SAME(scripts, engineerShopModal) &&
        SAME(garage, viewAngle) && SAME(garage, viewAngleTarget) &&
        SAME(garage, viewOffset) && SAME(garage, viewOffsetTarget) &&
        SAME(garage, courseCardSpin) && SAME(garage, courseCardSpinTarget) &&
        SAME(garage, courseCardPendingGrade) &&
        SAME(garage, pendingCourseIndex) && SAME(garage, carSwapFromIndex) &&
        SAME(garage, carSwapToIndex) && SAME(garage, overlayPattern) &&
        SAME(garage, carNamePlateStep) && SAME(garage, plateCarIndex) &&
        SAME(garage, carSpecGraphStep) && SAME(garage, courseModelIndex) &&
        SAME(transition, uiScriptProgress) &&
        SAME(transition, uiScriptProgress2) &&
        SAME(transition, hintBarProgress) && SAME(transition, confirmTimer) &&
        SAME(transition, busy) && SAME(transition, hintBarStep) &&
        SAME(transition, classChangeApplied) &&
        SAME(transition, courseSwapDelay) && SAME(transition, altPanelStep) &&
        SAME(transition, altPanelStep2) &&
        SAME(transition, timeAttackPlateStep) &&
        SAME(transition, hintButtonsVisible) &&
        SAME(selection, altLayoutSetting) && SAME(selection, carShopUnlockAll) &&
        SAME(selection, courseSelectOption) && SAME(selection, carSelectCursor) &&
        SAME(selection, rankingOption) && SAME(selection, designModeOption);
#undef SAME
}

static int TestMenuBridge(void) {
    static u8 script;
    MenuState source = MenuStateDefaults(37, &script);
    MenuState captured;
    MenuVisualState visual = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    MenuVisualState capturedVisual;

    source.garage.viewOffset = 101;
    source.garage.courseCardSpin = 102;
    source.transition.uiScriptProgress = 201;
    source.transition.confirmTimer = 202;
    source.transition.busy = 203;
    source.selection.altLayoutSetting = 301;
    source.selection.designModeOption = 302;
    MenuStateApplyLegacy(&source);
    MenuStateCaptureLegacy(&captured);
    EXPECT_EQ(1, MenuStateMatches(&source, &captured));

    MenuVisualStateApplyLegacy(&visual);
    MenuVisualStateCaptureLegacy(&capturedVisual);
    EXPECT_EQ(visual.courseSelect, capturedVisual.courseSelect);
    EXPECT_EQ(visual.ranking, capturedVisual.ranking);
    EXPECT_EQ(visual.carSelect, capturedVisual.carSelect);
    EXPECT_EQ(visual.customize, capturedVisual.customize);
    EXPECT_EQ(visual.designMode, capturedVisual.designMode);
    EXPECT_EQ(visual.teamLogo, capturedVisual.teamLogo);
    EXPECT_EQ(visual.logoSample, capturedVisual.logoSample);
    EXPECT_EQ(visual.teamName, capturedVisual.teamName);
    EXPECT_EQ(visual.paintColor, capturedVisual.paintColor);
    EXPECT_EQ(visual.carShop, capturedVisual.carShop);
    EXPECT_EQ(visual.engineerShop, capturedVisual.engineerShop);
    EXPECT_EQ(visual.carSpecGraph, capturedVisual.carSpecGraph);
    EXPECT_EQ(visual.lightBurst, capturedVisual.lightBurst);
    EXPECT_EQ(visual.timeAttackPlate, capturedVisual.timeAttackPlate);
    return 0;
}

static int TestBootBridge(void) {
    GameBootDefaults source = GameBootDefaultsCreate();
    GameBootDefaults captured;

    source.display.screenOffsetX = 11;
    source.display.screenOffsetY = 12;
    source.display.mirrorMode = 13;
    source.input.negconSteerPlay = 21;
    source.input.padMappingIndex = 22;
    source.input.negconMappingIndex = 23;
    source.input.negconSteerNeutral = 24;
    source.input.negconNeutralI = 25;
    source.input.negconNeutralII = 26;
    source.input.negconNeutralL = 27;
    source.input.negconMaxTwist = 28;
    source.padRuntime.errorState = PAD_ERROR_STATE_INVALID_INPUT;
    source.padRuntime.validateCountdown = 32;
    source.padRuntime.errorHoldBits = 33;
    source.progress.extraGrandPrixUnlocked = 41;
    GameBootDefaultsApplyLegacy(&source);
    GameBootDefaultsCaptureLegacy(&captured);

    EXPECT_EQ(source.display.screenOffsetX, captured.display.screenOffsetX);
    EXPECT_EQ(source.display.screenOffsetY, captured.display.screenOffsetY);
    EXPECT_EQ(source.display.mirrorMode, captured.display.mirrorMode);
    EXPECT_EQ(source.input.negconSteerPlay, captured.input.negconSteerPlay);
    EXPECT_EQ(source.input.padMappingIndex, captured.input.padMappingIndex);
    EXPECT_EQ(source.input.negconMappingIndex, captured.input.negconMappingIndex);
    EXPECT_EQ(source.input.negconSteerNeutral, captured.input.negconSteerNeutral);
    EXPECT_EQ(source.input.negconNeutralI, captured.input.negconNeutralI);
    EXPECT_EQ(source.input.negconNeutralII, captured.input.negconNeutralII);
    EXPECT_EQ(source.input.negconNeutralL, captured.input.negconNeutralL);
    EXPECT_EQ(source.input.negconMaxTwist, captured.input.negconMaxTwist);
    EXPECT_EQ(source.padRuntime.errorState, captured.padRuntime.errorState);
    EXPECT_EQ(source.padRuntime.validateCountdown, captured.padRuntime.validateCountdown);
    EXPECT_EQ(source.padRuntime.errorHoldBits, captured.padRuntime.errorHoldBits);
    EXPECT_EQ(source.progress.extraGrandPrixUnlocked,
              captured.progress.extraGrandPrixUnlocked);
    return 0;
}

static int TestFrontendBridge(void) {
    FrontendRuntimeState source = FrontendStateForTitle(1);
    FrontendRuntimeState captured;

    source.sceneTimer = 11;
    source.idleTimer = 12;
    source.mainMenuSlide = 13;
    source.titlePulse = 14;
    FrontendStateApplyLegacy(&source);
    FrontendStateCaptureLegacy(&captured);
    EXPECT_EQ(source.frameSyncThreshold, captured.frameSyncThreshold);
    EXPECT_EQ(source.sceneTimer, captured.sceneTimer);
    EXPECT_EQ(source.idleTimer, captured.idleTimer);
    EXPECT_EQ(source.titleFadeLevel, captured.titleFadeLevel);
    EXPECT_EQ(source.mainMenuSlide, captured.mainMenuSlide);
    EXPECT_EQ(source.titlePulse, captured.titlePulse);
    EXPECT_EQ(source.frontendState, captured.frontendState);
    EXPECT_EQ(source.titleExitTimer, captured.titleExitTimer);
    EXPECT_EQ(source.titleAttractTimer, captured.titleAttractTimer);
    return 0;
}

static int TestOwnedMenuRuntime(void) {
    MenuRuntime external = {
        .activeScreen = MENU_SCREEN_RANKING,
        .incomingScreen = MENU_SCREEN_RANKING,
        .outgoingScreen = MENU_SCREEN_NONE,
        .phase = MENU_RUNTIME_ACTIVE};
    const MenuRuntime *runtime;
    MenuState initialState = MenuStateDefaults(5, 0);
    MenuState publishedState;
    MenuVisualState initialVisual = MenuVisualStateDefaults();

    MenuFlowInitializeState(&initialState, &initialVisual);
    MenuFlowSetNavigation(&external);
    MenuFlowOpen(MENU_SCREEN_CAR_SELECT);
    runtime = MenuFlowGetRuntime();
    EXPECT_EQ(MENU_SCREEN_CAR_SELECT, runtime->activeScreen);
    EXPECT_EQ(runtime->activeScreen, g_MenuScreen);
    EXPECT_EQ(runtime->incomingScreen, g_MenuHandlerIndex);
    EXPECT_EQ(runtime->outgoingScreen, g_MenuHandlerIndex2);
    MenuFlowGetState()->garage.viewAngle = 765432;
    MenuFlowGetVisualState()->ranking = 17;
    MenuFlowPublishLegacyState();
    MenuStateCaptureLegacy(&publishedState);
    EXPECT_EQ(765432, publishedState.garage.viewAngle);
    return 0;
}

int main(void) {
    int result;
    if ((result = TestMenuBridge()) != 0) return result;
    if ((result = TestBootBridge()) != 0) return result;
    if ((result = TestFrontendBridge()) != 0) return result;
    if ((result = TestOwnedMenuRuntime()) != 0) return result;
    return 0;
}
