#include "game/car.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/scene.h"

enum {
    MENU_INITIAL_VIEW_ANGLE = 500000,
};

/* The menu-mode twin of InitTrackLighting. */
static void InitMenuLighting(void) {
    g_SceneColorMatrix = g_MenuColorMatrix;
    g_SceneLightMatrix = g_MenuLightMatrix;
    SetColorMatrix(&g_SceneColorMatrix);
    SetLightMatrix(&g_SceneLightMatrix);
    SetBackColor(0x20, 0x20, 0x20);
    SetFarColor(0, 0, 0);
    SetFogNear(0x4E20, 0x140);
}

static void LoadMenuSelection(void) {
    g_CourseIndex = g_RaceProgress->course;
    g_PlayerCarIndex = g_RaceProgress->carIndex;
    g_GrandPrixClass = g_RaceProgress->classIndex;
}

static void LoadMenuSeriesProgress(void) {
    if (g_GrandPrixMode != 0) {
        g_GrandPrixSeries = g_SeriesSelection;
        g_PlayerMoney = g_RaceProgress->money;
    } else {
        g_GrandPrixSeries = (u16)g_RaceProgress->timeAttackSeries;
        g_PlayerMoney = 0;
    }
    g_CourseIndex = (g_GrandPrixSeries << 2) | g_CourseIndex;
}

static void InitMenuCamera(void) {
    g_RenderState.viewX = 0;
    g_RenderState.viewY = -64;
    g_RenderState.viewZ = -256;
    g_RenderState.viewAngleX = 0x100;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleZ = 0;
    SetCameraRotMatrix();
    ScaleMatrix(&g_RenderState.matrix, &g_MenuViewScale);
}

static void ResetMenuNavigation(void) {
    g_CourseSelectModalScript = g_UiEmptyScript;
    g_CarSelectPopupScript = g_UiEmptyScript;
    g_CustomizePopupScript = g_UiEmptyScript;
    g_TeamLogoSubPanelScript = g_UiEmptyScript;
    g_LogoSampleSubPanelScript = g_UiEmptyScript;
    g_CarShopModalScript = g_UiEmptyScript;
    g_EngineerShopModalScript = g_UiEmptyScript;
    g_MenuViewAngle = MENU_INITIAL_VIEW_ANGLE;
    g_MenuViewAngleTarget = MENU_INITIAL_VIEW_ANGLE;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    g_MenuHintBarProgress = 0;
    g_MenuConfirmTimer = 0;
    GameMenuBusy = 0;
    g_MenuHintBarStep = 0;
    g_ClassChangeApplied = 0;
    g_CourseSwapDelay = 0;
    g_MenuViewOffset = 0;
    g_MenuViewOffsetTarget = 0;
    g_CourseCardSpin = 0;
    g_CourseCardSpinTarget = 0;
    g_CourseCardPendingGrade = 0;
    g_MenuPendingCourseIndex = -1;
    g_CarSwapFromIndex = 0;
    g_CarSwapToIndex = -1;
    g_MenuOverlayPattern = 0;
    g_CarNamePlateStep = 0;
    g_MenuPlateCarIndex = 0;
    g_CarSpecGraphStep = 0;
    g_MenuCourseModelIndex = g_CourseIndex;
    g_MenuUpperAltPanelStep = 0;
    g_MenuLowerAltPanelStep = 0;
    g_TimeAttackPlateStep = 0;
    g_MenuHintButtonsVisible = 1;
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = -1;
    g_MenuAltLayoutSetting = 0;
    g_CarShopUnlockAll = 0;
    g_MenuScreen = MENU_SCREEN_BOOTSTRAP;
    g_CourseSelectOption = 0;
    g_CarSelectCursor = 0;
    g_CustomizeOption = 0;
    g_DesignModeOption = 0;
}

/* Screen draw functions own additional animation counters. A zero step is
 * their shared reset operation and returns before any renderer access. */
static void ResetMenuWidgets(void) {
    DrawCourseSelectScreen(0);
    DrawRankingScreen(0);
    DrawCarSelectScreen(0);
    DrawCustomizeScreen(0);
    DrawDesignModeScreen(0);
    DrawTeamLogoScreen(0);
    DrawLogoSampleScreen(0);
    DrawTeamNameScreen(0);
    DrawPaintColorScreen(0);
    DrawCarShopScreen(0);
    DrawEngineerShopScreen(0);
    DrawCarSpecGraph(0, 0); /* step 0 resets and returns before the grade */
    DrawMenuLightBurst(0);
    DrawTimeAttackPlate(0);
}

void InitMenuMode(void) {
    SetDispMask(0);
    g_MirrorMode = 0;
    g_FrameSyncThreshold = 0x80;
    LoadMenuSelection();
    InitRenderState(1);

    SetupDisplay480(0, 0, 0);
    g_SceneId = GAME_SCENE_MENU;
    g_SceneTimer = 0;
    LoadMenuSeriesProgress();
    InitMenuLighting();
    InitMenuCamera();
    ResetMenuNavigation();
    ResetMenuWidgets();
}
