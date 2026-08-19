#ifndef GAME_MENU_LEGACY_GLOBALS_H
#define GAME_MENU_LEGACY_GLOBALS_H

#include "common.h"

extern s32 g_MenuScreen;
extern s32 g_MenuHandlerIndex;
extern s32 g_MenuHandlerIndex2;

extern u8 *g_CourseSelectModalScript;
extern u8 *g_CarSelectPopupScript;
extern u8 *g_CustomizePopupScript;
extern void *g_TeamLogoSubPanelScript;
extern void *g_LogoSampleSubPanelScript;
extern u8 *g_CarShopModalScript;
extern u8 *g_EngineerShopModalScript;
extern s32 g_MenuViewAngle;
extern s32 g_MenuViewAngleTarget;
extern s32 g_MenuViewOffset;
extern s32 g_MenuViewOffsetTarget;
extern s32 g_CourseCardSpin;
extern s32 g_CourseCardSpinTarget;
extern s32 g_CourseCardPendingGrade;
extern s32 g_MenuPendingCourseIndex;
extern s32 g_CarSwapFromIndex;
extern s32 g_CarSwapToIndex;
extern s32 g_MenuOverlayPattern;
extern s32 g_CarNamePlateStep;
extern s32 g_MenuPlateCarIndex;
extern s32 g_CarSpecGraphStep;
extern s32 g_MenuCourseModelIndex;
extern s32 g_UiScriptProgress;
extern s32 g_UiScriptProgress2;
extern s32 g_MenuHintBarProgress;
extern s32 g_MenuConfirmTimer;
extern s32 GameMenuBusy;
extern s32 g_MenuHintBarStep;
extern s32 g_ClassChangeApplied;
extern s32 g_CourseSwapDelay;
extern s32 g_MenuAltPanelStep;
extern s32 g_MenuAltPanelStep2;
extern s32 g_TimeAttackPlateStep;
extern s32 g_MenuHintButtonsVisible;
extern s32 g_MenuAltLayoutSetting;
extern s32 g_CarShopUnlockAll;
extern s32 g_CourseSelectOption;
extern s32 g_CarSelectCursor;
extern s32 g_RankingOption;
extern s32 g_DesignModeOption;
extern s32 g_CourseSelectScrollState;
extern s32 g_RankingScrollState;
extern s32 g_CarSelectFadeAccum;
extern s32 g_CustomizeFadeAccum;
extern s32 g_DesignModeScreenFade;
extern s32 g_TeamLogoScreenFade;
extern s32 g_LogoSampleScreenFade;
extern s32 g_TeamNameScreenProgress;
extern s32 g_PaintColorScreenProgress;
extern s32 g_CarShopScreenProgress;
extern u32 g_EngineSpecStep;
extern s32 g_CarSpecGraphProgress;
extern s32 g_MenuLightBurstLevel;
extern volatile s32 g_TimeAttackPlateProgress;

#endif
