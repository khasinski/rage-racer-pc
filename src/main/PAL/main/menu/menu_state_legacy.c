#include "game/menu_legacy_globals.h"
#include "game/menu_state_legacy.h"

void MenuVisualStateApplyLegacy(const MenuVisualState *state) {
    g_CourseSelectScrollState = state->courseSelect;
    g_RankingScrollState = state->ranking;
    g_CarSelectFadeAccum = state->carSelect;
    g_CustomizeFadeAccum = state->customize;
    g_DesignModeScreenFade = state->designMode;
    g_TeamLogoScreenFade = state->teamLogo;
    g_LogoSampleScreenFade = state->logoSample;
    g_TeamNameScreenProgress = state->teamName;
    g_PaintColorScreenProgress = state->paintColor;
    g_CarShopScreenProgress = state->carShop;
    g_EngineSpecStep = state->engineerShop;
    g_CarSpecGraphProgress = state->carSpecGraph;
    g_MenuLightBurstLevel = state->lightBurst;
    g_TimeAttackPlateProgress = state->timeAttackPlate;
}

void MenuVisualStateCaptureLegacy(MenuVisualState *state) {
    state->courseSelect = g_CourseSelectScrollState;
    state->ranking = g_RankingScrollState;
    state->carSelect = g_CarSelectFadeAccum;
    state->customize = g_CustomizeFadeAccum;
    state->designMode = g_DesignModeScreenFade;
    state->teamLogo = g_TeamLogoScreenFade;
    state->logoSample = g_LogoSampleScreenFade;
    state->teamName = g_TeamNameScreenProgress;
    state->paintColor = g_PaintColorScreenProgress;
    state->carShop = g_CarShopScreenProgress;
    state->engineerShop = g_EngineSpecStep;
    state->carSpecGraph = g_CarSpecGraphProgress;
    state->lightBurst = g_MenuLightBurstLevel;
    state->timeAttackPlate = g_TimeAttackPlateProgress;
}

void MenuVisualStateResetLegacy(void) {
    MenuVisualState state;
    MenuVisualStateReset(&state);
    MenuVisualStateApplyLegacy(&state);
}

void MenuStateApplyLegacy(const MenuState *state) {
    g_CourseSelectModalScript = state->scripts.courseSelectModal;
    g_CarSelectPopupScript = state->scripts.carSelectPopup;
    g_CustomizePopupScript = state->scripts.customizePopup;
    g_TeamLogoSubPanelScript = state->scripts.teamLogoSubPanel;
    g_LogoSampleSubPanelScript = state->scripts.logoSampleSubPanel;
    g_CarShopModalScript = state->scripts.carShopModal;
    g_EngineerShopModalScript = state->scripts.engineerShopModal;
    g_MenuViewAngle = state->garage.viewAngle;
    g_MenuViewAngleTarget = state->garage.viewAngleTarget;
    g_MenuViewOffset = state->garage.viewOffset;
    g_MenuViewOffsetTarget = state->garage.viewOffsetTarget;
    g_CourseCardSpin = state->garage.courseCardSpin;
    g_CourseCardSpinTarget = state->garage.courseCardSpinTarget;
    g_CourseCardPendingGrade = state->garage.courseCardPendingGrade;
    g_MenuPendingCourseIndex = state->garage.pendingCourseIndex;
    g_CarSwapFromIndex = state->garage.carSwapFromIndex;
    g_CarSwapToIndex = state->garage.carSwapToIndex;
    g_MenuOverlayPattern = state->garage.overlayPattern;
    g_CarNamePlateStep = state->garage.carNamePlateStep;
    g_MenuPlateCarIndex = state->garage.plateCarIndex;
    g_CarSpecGraphStep = state->garage.carSpecGraphStep;
    g_MenuCourseModelIndex = state->garage.courseModelIndex;
    g_UiScriptProgress = state->transition.uiScriptProgress;
    g_UiScriptProgress2 = state->transition.uiScriptProgress2;
    g_MenuHintBarProgress = state->transition.hintBarProgress;
    g_MenuConfirmTimer = state->transition.confirmTimer;
    GameMenuBusy = state->transition.busy;
    g_MenuHintBarStep = state->transition.hintBarStep;
    g_ClassChangeApplied = state->transition.classChangeApplied;
    g_CourseSwapDelay = state->transition.courseSwapDelay;
    g_MenuAltPanelStep = state->transition.altPanelStep;
    g_MenuAltPanelStep2 = state->transition.altPanelStep2;
    g_TimeAttackPlateStep = state->transition.timeAttackPlateStep;
    g_MenuHintButtonsVisible = state->transition.hintButtonsVisible;
    g_MenuAltLayoutSetting = state->selection.altLayoutSetting;
    g_CarShopUnlockAll = state->selection.carShopUnlockAll;
    g_CourseSelectOption = state->selection.courseSelectOption;
    g_CarSelectCursor = state->selection.carSelectCursor;
    g_RankingOption = state->selection.rankingOption;
    g_DesignModeOption = state->selection.designModeOption;
}

void MenuStateCaptureLegacy(MenuState *state) {
    state->scripts.courseSelectModal = g_CourseSelectModalScript;
    state->scripts.carSelectPopup = g_CarSelectPopupScript;
    state->scripts.customizePopup = g_CustomizePopupScript;
    state->scripts.teamLogoSubPanel = g_TeamLogoSubPanelScript;
    state->scripts.logoSampleSubPanel = g_LogoSampleSubPanelScript;
    state->scripts.carShopModal = g_CarShopModalScript;
    state->scripts.engineerShopModal = g_EngineerShopModalScript;
    state->garage.viewAngle = g_MenuViewAngle;
    state->garage.viewAngleTarget = g_MenuViewAngleTarget;
    state->garage.viewOffset = g_MenuViewOffset;
    state->garage.viewOffsetTarget = g_MenuViewOffsetTarget;
    state->garage.courseCardSpin = g_CourseCardSpin;
    state->garage.courseCardSpinTarget = g_CourseCardSpinTarget;
    state->garage.courseCardPendingGrade = g_CourseCardPendingGrade;
    state->garage.pendingCourseIndex = g_MenuPendingCourseIndex;
    state->garage.carSwapFromIndex = g_CarSwapFromIndex;
    state->garage.carSwapToIndex = g_CarSwapToIndex;
    state->garage.overlayPattern = g_MenuOverlayPattern;
    state->garage.carNamePlateStep = g_CarNamePlateStep;
    state->garage.plateCarIndex = g_MenuPlateCarIndex;
    state->garage.carSpecGraphStep = g_CarSpecGraphStep;
    state->garage.courseModelIndex = g_MenuCourseModelIndex;
    state->transition.uiScriptProgress = g_UiScriptProgress;
    state->transition.uiScriptProgress2 = g_UiScriptProgress2;
    state->transition.hintBarProgress = g_MenuHintBarProgress;
    state->transition.confirmTimer = g_MenuConfirmTimer;
    state->transition.busy = GameMenuBusy;
    state->transition.hintBarStep = g_MenuHintBarStep;
    state->transition.classChangeApplied = g_ClassChangeApplied;
    state->transition.courseSwapDelay = g_CourseSwapDelay;
    state->transition.altPanelStep = g_MenuAltPanelStep;
    state->transition.altPanelStep2 = g_MenuAltPanelStep2;
    state->transition.timeAttackPlateStep = g_TimeAttackPlateStep;
    state->transition.hintButtonsVisible = g_MenuHintButtonsVisible;
    state->selection.altLayoutSetting = g_MenuAltLayoutSetting;
    state->selection.carShopUnlockAll = g_CarShopUnlockAll;
    state->selection.courseSelectOption = g_CourseSelectOption;
    state->selection.carSelectCursor = g_CarSelectCursor;
    state->selection.rankingOption = g_RankingOption;
    state->selection.designModeOption = g_DesignModeOption;
}
