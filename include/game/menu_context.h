#ifndef GAME_MENU_CONTEXT_H
#define GAME_MENU_CONTEXT_H

#include "game/menu_flow.h"

MenuState *MenuFlowGetState(void);
MenuVisualState *MenuFlowGetVisualState(void);
void MenuFlowInitializeState(
    const MenuState *state, const MenuVisualState *visual);
void MenuFlowPublishLegacyState(void);

/* Transitional source aliases: gameplay owns the structured state while the
 * original absolute symbols remain an ABI snapshot for tooling and old code. */
#define g_CourseSelectModalScript (MenuFlowGetState()->scripts.courseSelectModal)
#define g_CarSelectPopupScript (MenuFlowGetState()->scripts.carSelectPopup)
#define g_CustomizePopupScript (MenuFlowGetState()->scripts.customizePopup)
#define g_TeamLogoSubPanelScript (MenuFlowGetState()->scripts.teamLogoSubPanel)
#define g_LogoSampleSubPanelScript (MenuFlowGetState()->scripts.logoSampleSubPanel)
#define g_CarShopModalScript (MenuFlowGetState()->scripts.carShopModal)
#define g_EngineerShopModalScript (MenuFlowGetState()->scripts.engineerShopModal)
#define g_MenuViewAngle (MenuFlowGetState()->garage.viewAngle)
#define g_MenuViewAngleTarget (MenuFlowGetState()->garage.viewAngleTarget)
#define g_MenuViewOffset (MenuFlowGetState()->garage.viewOffset)
#define g_MenuViewOffsetTarget (MenuFlowGetState()->garage.viewOffsetTarget)
#define g_CourseCardSpin (MenuFlowGetState()->garage.courseCardSpin)
#define g_CourseCardSpinTarget (MenuFlowGetState()->garage.courseCardSpinTarget)
#define g_CourseCardPendingGrade (MenuFlowGetState()->garage.courseCardPendingGrade)
#define g_MenuPendingCourseIndex (MenuFlowGetState()->garage.pendingCourseIndex)
#define g_CarSwapFromIndex (MenuFlowGetState()->garage.carSwapFromIndex)
#define g_CarSwapToIndex (MenuFlowGetState()->garage.carSwapToIndex)
#define g_MenuOverlayPattern (MenuFlowGetState()->garage.overlayPattern)
#define g_CarNamePlateStep (MenuFlowGetState()->garage.carNamePlateStep)
#define g_MenuPlateCarIndex (MenuFlowGetState()->garage.plateCarIndex)
#define g_CarSpecGraphStep (MenuFlowGetState()->garage.carSpecGraphStep)
#define g_MenuCourseModelIndex (MenuFlowGetState()->garage.courseModelIndex)
#define g_UiScriptProgress (MenuFlowGetState()->transition.uiScriptProgress)
#define g_UiScriptProgress2 (MenuFlowGetState()->transition.uiScriptProgress2)
#define g_MenuHintBarProgress (MenuFlowGetState()->transition.hintBarProgress)
#define g_MenuConfirmTimer (MenuFlowGetState()->transition.confirmTimer)
#define GameMenuBusy (MenuFlowGetState()->transition.busy)
#define g_MenuHintBarStep (MenuFlowGetState()->transition.hintBarStep)
#define g_ClassChangeApplied (MenuFlowGetState()->transition.classChangeApplied)
#define g_CourseSwapDelay (MenuFlowGetState()->transition.courseSwapDelay)
#define g_MenuAltPanelStep (MenuFlowGetState()->transition.altPanelStep)
#define g_MenuAltPanelStep2 (MenuFlowGetState()->transition.altPanelStep2)
#define g_TimeAttackPlateStep (MenuFlowGetState()->transition.timeAttackPlateStep)
#define g_MenuHintButtonsVisible (MenuFlowGetState()->transition.hintButtonsVisible)
#define g_MenuAltLayoutSetting (MenuFlowGetState()->selection.altLayoutSetting)
#define g_CarShopUnlockAll (MenuFlowGetState()->selection.carShopUnlockAll)
#define g_CourseSelectOption (MenuFlowGetState()->selection.courseSelectOption)
#define g_CarSelectCursor (MenuFlowGetState()->selection.carSelectCursor)
#define g_RankingOption (MenuFlowGetState()->selection.rankingOption)
#define g_DesignModeOption (MenuFlowGetState()->selection.designModeOption)
#define g_CourseSelectScrollValue (MenuFlowGetVisualState()->courseSelect)
#define g_RankingScrollState (MenuFlowGetVisualState()->ranking)
#define g_CarSelectFadeAccum (MenuFlowGetVisualState()->carSelect)
#define g_CustomizeFadeAccum (MenuFlowGetVisualState()->customize)
#define g_DesignModeScreenFade (MenuFlowGetVisualState()->designMode)
#define g_TeamLogoScreenFade (MenuFlowGetVisualState()->teamLogo)
#define g_LogoSampleScreenFade (MenuFlowGetVisualState()->logoSample)
#define g_TeamNameScreenProgress (MenuFlowGetVisualState()->teamName)
#define g_PaintColorScreenProgress (MenuFlowGetVisualState()->paintColor)
#define g_CarShopScreenProgress (MenuFlowGetVisualState()->carShop)
#define g_EngineSpecStep (MenuFlowGetVisualState()->engineerShop)
#define g_CarSpecGraphProgress (MenuFlowGetVisualState()->carSpecGraph)
#define g_MenuLightBurstLevel (MenuFlowGetVisualState()->lightBurst)
#define g_TimeAttackPlateProgress (MenuFlowGetVisualState()->timeAttackPlate)

#endif
