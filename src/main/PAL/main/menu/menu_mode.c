#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/scene.h"

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
    g_Camera.view.x = 0;
    g_Camera.view.y = -64;
    g_Camera.view.z = -256;
    g_Camera.view.angleX = 0x100;
    g_Camera.view.angleY = 0;
    g_Camera.view.angleZ = 0;
    SetCameraRotMatrix(&g_RenderState, &g_Camera.view);
    ScaleMatrix(&g_RenderState.geometry.matrix, &g_MenuViewScale);
}

static void ResetMenuNavigation(void) {
    CourseSelectScreen *courseSelect;
    Customize *customize;
    LogoSample *logoSample;

    MenuRuntimeReset();
    courseSelect = MenuCourseSelect();
    customize = MenuCustomize();
    logoSample = MenuLogoSample();
    courseSelect->modalScript = g_UiEmptyScript;
    customize->popupScript = g_UiEmptyScript;
    logoSample->subPanelScript = g_UiEmptyScript;
    g_CarSelectPopupScript = g_UiEmptyScript;
    g_TeamLogoSubPanelScript = g_UiEmptyScript;
    ResetCarShopScreen();
    ResetEngineerShopScreen();
    ResetMenuCar();
    g_MenuViewAngle = MENU_COURSE_VIEW_REBASE_SPAN;
    g_MenuViewAngleTarget = MENU_COURSE_VIEW_REBASE_SPAN;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    g_MenuHintBarProgress = 0;
    g_MenuConfirmTimer = 0;
    GameMenuBusy = 0;
    g_MenuHintBarStep = 0;
    courseSelect->swapDelay = 0;
    g_MenuViewOffset = 0;
    g_MenuViewOffsetTarget = 0;
    courseSelect->cardSpin = 0;
    courseSelect->cardSpinTarget = 0;
    courseSelect->cardPendingGrade = 0;
    courseSelect->pendingCourse = -1;
    g_CarSwapFromIndex = 0;
    g_CarSwapToIndex = -1;
    g_MenuOverlayPattern = 0;
    g_CarNamePlateStep = 0;
    g_MenuPlateCarIndex = 0;
    g_CarSpecGraphStep = 0;
    courseSelect->displayedCourse = g_CourseIndex;
    g_MenuUpperAltPanelStep = 0;
    g_MenuLowerAltPanelStep = 0;
    g_TimeAttackPlateStep = 0;
    g_MenuHintButtonsVisible = 1;
    g_MenuAltLayoutSetting = 0;
    g_CarShopUnlockAll = 0;
    g_CarSelectCursor = 0;
    g_DesignModeOption = 0;
}

/* Shared widgets outside the screen transition table own these counters. */
static void ResetMenuWidgets(void) {
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
