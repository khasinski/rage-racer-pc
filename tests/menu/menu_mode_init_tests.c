#include "game/car.h"
#include "game/menu.h"

static MenuWidgets s_menuWidgets;
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

static CarSelect s_carSelect;

GameRenderState g_RenderState;
Camera g_Camera;
Matrix g_SceneColorMatrix;
Matrix g_SceneLightMatrix;
TimedDrawCommand g_UiEmptyScript[1];

static s32 s_displayMask;
static s32 s_displaySetups;
static s32 s_drawResetCalls;
static s32 s_initRenderMode;
static s32 s_cameraCalls;
static s32 s_carShopResets;
static s32 s_engineerShopResets;
static EngineerShop s_engineerShop;
static Customize s_customize;
static LogoSample s_logo;
static TeamLogo s_teamLogo;
static s32 s_menuCarResets;
static CourseSelectScreen s_courseSelect;
static CarSpecGraph s_carSpecGraph;

s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuScreen;

void MenuRuntimeReset(void) {
    memset(&s_courseSelect, 0, sizeof(s_courseSelect));
    memset(&s_customize, 0, sizeof(s_customize));
    memset(&s_logo, 0, sizeof(s_logo));
    memset(&s_teamLogo, 0, sizeof(s_teamLogo));
    memset(&s_carSpecGraph, 0, sizeof(s_carSpecGraph));
    g_MenuScreen = MENU_SCREEN_BOOTSTRAP;
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = -1;
}
CourseSelectScreen *MenuCourseSelect(void) { return &s_courseSelect; }
Customize *MenuCustomize(void) { return &s_customize; }
LogoSample *MenuLogoSample(void) { return &s_logo; }
TeamLogo *MenuTeamLogo(void) { return &s_teamLogo; }
CarSpecGraph *MenuCarSpecGraph(void) { return &s_carSpecGraph; }
void ResetCarShopScreen(void) { s_carShopResets++; }
void ResetEngineerShopScreen(void) {
    s_engineerShop = (EngineerShop){.modalScript = g_UiEmptyScript};
    s_engineerShopResets++;
}
void ResetMenuCar(void) { s_menuCarResets++; }
void ResetMenuButtonAnimation(void) {}

void SetDispMask(s32 enabled) { s_displayMask = enabled; }
void InitRenderState(s32 mode) { s_initRenderMode = mode; }
void SetupDisplay480(s32 r, s32 g, s32 b) {
    (void)r;
    (void)g;
    (void)b;
    s_displaySetups++;
}
void SetColorMatrix(MATRIX *matrix) { (void)matrix; }
void SetLightMatrix(MATRIX *matrix) { (void)matrix; }
void SetBackColor(long r, long g, long b) {
    (void)r;
    (void)g;
    (void)b;
}
void SetFarColor(long r, long g, long b) {
    (void)r;
    (void)g;
    (void)b;
}
void SetFogNear(long nearValue, long projectionDistance) {
    (void)nearValue;
    (void)projectionDistance;
}
void SetCameraRotMatrix(GameRenderState *state, const GameCameraState *camera) { (void)state; (void)camera; s_cameraCalls++; }

#undef ScaleMatrix
MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale) {
    (void)scale;
    return matrix;
}

void DrawCarSpecGraph(CarSpecGraph *graph, u32 tireGrade) {
    if (graph->step == 0 && tireGrade == 0) s_drawResetCalls++;
}
void DrawMenuLightBurst(MenuWidgets *widgets, s32 step) {
    (void)widgets;
    if (step == 0) s_drawResetCalls++;
}
void DrawTimeAttackPlate(MenuWidgets *widgets) {
    if (widgets->timeAttackStep == 0) s_drawResetCalls++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void PoisonEntryState(void) {
    g_MirrorMode = 1;
    g_FrameSyncThreshold = 0;
    g_SceneId = -1;
    g_SceneTimer = 99;
    s_courseSelect.modalScript = NULL;
    s_carSelect.popupScript = NULL;
    s_customize.popupScript = NULL;
    s_teamLogo.subPanelScript = NULL;
    s_logo.subPanelScript = NULL;
    s_engineerShop.modalScript = NULL;
    g_MenuViewAngle = 1;
    g_MenuViewAngleTarget = 2;
    g_UiScriptProgress = 3;
    g_UiScriptProgress2 = 4;
    s_menuWidgets.hintProgress = 5;
    s_courseSelect.confirmTimer = 6;
    s_engineerShop.confirmTimer = 6;
    s_customize.confirmTimer = 6;
    s_teamLogo.confirmTimer = 6;
    GameMenuBusy = 6;
    s_menuWidgets.hintStep = 7;
    s_courseSelect.classChangeApplied = 8;
    s_courseSelect.swapDelay = 9;
    g_MenuViewOffset = 10;
    g_MenuViewOffsetTarget = 11;
    s_courseSelect.cardSpin = 12;
    s_courseSelect.cardSpinTarget = 13;
    s_courseSelect.cardPendingGrade = 14;
    s_courseSelect.pendingCourse = 7;
    g_CarSwapFromIndex = 15;
    g_CarSwapToIndex = 8;
    g_MenuOverlayPattern = 16;
    s_menuWidgets.carNameStep = 17;
    s_menuWidgets.carNameModel = 18;
    s_carSpecGraph.step = 19;
    s_menuWidgets.upperAltPanelStep = 20;
    s_menuWidgets.lowerAltPanelStep = 21;
    s_menuWidgets.timeAttackStep = 22;
    s_menuWidgets.hintButtonsVisible = 0;
    g_MenuHandlerIndex = 9;
    g_MenuOutgoingHandlerIndex = 10;
    g_CarShopUnlockAll = 24;
    g_MenuScreen = MENU_SCREEN_UNUSED;
    s_courseSelect.option = 25;
    s_carSelect.cursor = 26;
    s_customize.option = 27;
    g_DesignModeOption = 28;
    s_displayMask = -1;
    s_displaySetups = 0;
    s_drawResetCalls = 0;
    s_initRenderMode = -1;
    s_cameraCalls = 0;
    s_carShopResets = 0;
    s_engineerShopResets = 0;
    s_menuCarResets = 0;
}

static int CheckCommonEntryState(const GameRaceProgress *progress) {
    CHECK(s_displayMask == 0 && s_displaySetups == 1);
    CHECK(s_initRenderMode == 1 && s_cameraCalls == 1);
    CHECK(g_MirrorMode == 0 && g_FrameSyncThreshold == 0x80);
    CHECK(g_SceneId == 8 && g_SceneTimer == 0);
    CHECK(g_PlayerCarIndex == progress->carIndex);
    CHECK(g_GrandPrixClass == progress->classIndex);
    CHECK(g_Camera.view.x == 0 && g_Camera.view.y == -64);
    CHECK(g_Camera.view.z == -256 && g_Camera.view.angleX == 0x100);
    CHECK(s_courseSelect.modalScript == g_UiEmptyScript);
    CHECK(s_carSelect.popupScript == g_UiEmptyScript);
    CHECK(s_customize.popupScript == g_UiEmptyScript);
    CHECK(s_teamLogo.subPanelScript == g_UiEmptyScript);
    CHECK(s_logo.subPanelScript == g_UiEmptyScript);
    CHECK(s_engineerShop.modalScript == g_UiEmptyScript);
    CHECK(g_MenuViewAngle == MENU_COURSE_VIEW_REBASE_SPAN);
    CHECK(g_MenuViewAngleTarget == MENU_COURSE_VIEW_REBASE_SPAN);
    CHECK(g_UiScriptProgress == 0 && g_UiScriptProgress2 == 0);
    CHECK(s_menuWidgets.hintProgress == 0 && GameMenuBusy == 0);
    CHECK(s_courseSelect.confirmTimer == 0 &&
          s_engineerShop.confirmTimer == 0 &&
          s_customize.confirmTimer == 0 && s_teamLogo.confirmTimer == 0);
    CHECK(s_menuWidgets.hintStep == 0);
    CHECK(s_courseSelect.classChangeApplied == 0 && s_courseSelect.swapDelay == 0);
    CHECK(g_MenuViewOffset == 0 && g_MenuViewOffsetTarget == 0);
    CHECK(s_courseSelect.cardSpin == 0 && s_courseSelect.cardSpinTarget == 0);
    CHECK(s_courseSelect.cardPendingGrade == 0 && g_CarSwapFromIndex == 0);
    CHECK(s_courseSelect.pendingCourse == -1 && g_CarSwapToIndex == -1);
    CHECK(g_MenuOverlayPattern == 0 && s_menuWidgets.carNameStep == 0);
    CHECK(s_menuWidgets.carNameModel == 0 && s_carSpecGraph.step == 0);
    CHECK(s_menuWidgets.upperAltPanelStep == 0 && s_menuWidgets.lowerAltPanelStep == 0);
    CHECK(s_menuWidgets.timeAttackStep == 0 && s_menuWidgets.hintButtonsVisible == 1);
    CHECK(g_MenuHandlerIndex == -1 && g_MenuOutgoingHandlerIndex == -1);
    CHECK(g_CarShopUnlockAll == 0);
    CHECK(g_MenuScreen == MENU_SCREEN_BOOTSTRAP);
    CHECK(s_courseSelect.option == 0 && s_carSelect.cursor == 0);
    CHECK(s_customize.option == 0 && g_DesignModeOption == 0);
    CHECK(s_drawResetCalls == 3);
    CHECK(s_carShopResets == 1);
    CHECK(s_engineerShopResets == 1);
    CHECK(s_menuCarResets == 1);
    return 0;
}

static int TestGrandPrixEntry(void) {
    GameRaceProgress progress = {
        .course = 2,
        .carIndex = 3,
        .classIndex = 4,
        .maxClassReached = 5,
        .money = 123456,
    };

    PoisonEntryState();
    g_RaceProgress = &progress;
    g_GrandPrixMode = 1;
    g_SeriesSelection = 1;
    InitMenuMode();
    CHECK(CheckCommonEntryState(&progress) == 0);
    CHECK(g_GrandPrixSeries == 1 && g_CourseIndex == 6);
    CHECK(s_courseSelect.displayedCourse == 6 && g_PlayerMoney == 123456);
    return 0;
}

static int TestTimeAttackEntry(void) {
    GameRaceProgress progress = {
        .course = 3,
        .carIndex = 2,
        .classIndex = 1,
        .maxClassReached = 1,
        .timeAttackSeries = 1,
    };

    PoisonEntryState();
    g_RaceProgress = &progress;
    g_GrandPrixMode = 0;
    g_SeriesSelection = 0;
    InitMenuMode();
    CHECK(CheckCommonEntryState(&progress) == 0);
    CHECK(g_GrandPrixSeries == 1 && g_CourseIndex == 7);
    CHECK(s_courseSelect.displayedCourse == 7 && g_PlayerMoney == 0);
    return 0;
}

int main(void) {
    CHECK(TestGrandPrixEntry() == 0);
    CHECK(TestTimeAttackEntry() == 0);
    puts("menu mode initialization tests passed");
    return 0;
}

CarSelect *MenuCarSelect(void) { return &s_carSelect; }
MenuWidgets *MenuWidgetState(void) { return &s_menuWidgets; }
