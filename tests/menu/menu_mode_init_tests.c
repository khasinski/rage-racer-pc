#include "game/car.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
Matrix g_MenuColorMatrix;
Matrix g_MenuLightMatrix;
Matrix g_SceneColorMatrix;
Matrix g_SceneLightMatrix;
TimedDrawCommand g_UiEmptyScript[1];

static s32 s_displayMask;
static s32 s_displaySetups;
static s32 s_drawResetCalls;
static s32 s_initRenderMode;
static s32 s_cameraCalls;

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
void SetCameraRotMatrix(void) { s_cameraCalls++; }

#undef ScaleMatrix
MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale) {
    (void)scale;
    return matrix;
}

#define DRAW_RESET_STUB(name)                                                  \
    s32 name(s32 step) {                                                       \
        if (step == 0) s_drawResetCalls++;                                     \
        return 0;                                                              \
    }

DRAW_RESET_STUB(DrawCourseSelectScreen)
DRAW_RESET_STUB(DrawRankingScreen)
DRAW_RESET_STUB(DrawCarSelectScreen)
DRAW_RESET_STUB(DrawCustomizeScreen)
DRAW_RESET_STUB(DrawDesignModeScreen)
DRAW_RESET_STUB(DrawTeamLogoScreen)
DRAW_RESET_STUB(DrawLogoSampleScreen)
DRAW_RESET_STUB(DrawTeamNameScreen)
DRAW_RESET_STUB(DrawPaintColorScreen)
DRAW_RESET_STUB(DrawCarShopScreen)
DRAW_RESET_STUB(DrawEngineerShopScreen)

void DrawCarSpecGraph(s32 step, u32 tireGrade) {
    if (step == 0 && tireGrade == 0) s_drawResetCalls++;
}
void DrawMenuLightBurst(s32 step) {
    if (step == 0) s_drawResetCalls++;
}
void DrawTimeAttackPlate(s32 step) {
    if (step == 0) s_drawResetCalls++;
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
    g_CourseSelectModalScript = NULL;
    g_CarSelectPopupScript = NULL;
    g_CustomizePopupScript = NULL;
    g_TeamLogoSubPanelScript = NULL;
    g_LogoSampleSubPanelScript = NULL;
    g_CarShopModalScript = NULL;
    g_EngineerShopModalScript = NULL;
    g_MenuViewAngle = 1;
    g_MenuViewAngleTarget = 2;
    g_UiScriptProgress = 3;
    g_UiScriptProgress2 = 4;
    g_MenuHintBarProgress = 5;
    g_MenuConfirmTimer = 6;
    GameMenuBusy = 6;
    g_MenuHintBarStep = 7;
    g_ClassChangeApplied = 8;
    g_CourseSwapDelay = 9;
    g_MenuViewOffset = 10;
    g_MenuViewOffsetTarget = 11;
    g_CourseCardSpin = 12;
    g_CourseCardSpinTarget = 13;
    g_CourseCardPendingGrade = 14;
    g_MenuPendingCourseIndex = 7;
    g_CarSwapFromIndex = 15;
    g_CarSwapToIndex = 8;
    g_MenuOverlayPattern = 16;
    g_CarNamePlateStep = 17;
    g_MenuPlateCarIndex = 18;
    g_CarSpecGraphStep = 19;
    g_MenuUpperAltPanelStep = 20;
    g_MenuLowerAltPanelStep = 21;
    g_TimeAttackPlateStep = 22;
    g_MenuHintButtonsVisible = 0;
    g_MenuHandlerIndex = 9;
    g_MenuOutgoingHandlerIndex = 10;
    g_MenuAltLayoutSetting = 23;
    g_CarShopUnlockAll = 24;
    g_MenuScreen = MENU_SCREEN_UNUSED;
    g_CourseSelectOption = 25;
    g_CarSelectCursor = 26;
    g_CustomizeOption = 27;
    g_DesignModeOption = 28;
    s_displayMask = -1;
    s_displaySetups = 0;
    s_drawResetCalls = 0;
    s_initRenderMode = -1;
    s_cameraCalls = 0;
}

static int CheckCommonEntryState(const GameRaceProgress *progress) {
    CHECK(s_displayMask == 0 && s_displaySetups == 1);
    CHECK(s_initRenderMode == 1 && s_cameraCalls == 1);
    CHECK(g_MirrorMode == 0 && g_FrameSyncThreshold == 0x80);
    CHECK(g_SceneId == 8 && g_SceneTimer == 0);
    CHECK(g_PlayerCarIndex == progress->carIndex);
    CHECK(g_GrandPrixClass == progress->classIndex);
    CHECK(g_RenderState.viewX == 0 && g_RenderState.viewY == -64);
    CHECK(g_RenderState.viewZ == -256 && g_RenderState.viewAngleX == 0x100);
    CHECK(g_CourseSelectModalScript == g_UiEmptyScript);
    CHECK(g_CarSelectPopupScript == g_UiEmptyScript);
    CHECK(g_CustomizePopupScript == g_UiEmptyScript);
    CHECK(g_TeamLogoSubPanelScript == g_UiEmptyScript);
    CHECK(g_LogoSampleSubPanelScript == g_UiEmptyScript);
    CHECK(g_CarShopModalScript == g_UiEmptyScript);
    CHECK(g_EngineerShopModalScript == g_UiEmptyScript);
    CHECK(g_MenuViewAngle == 500000 && g_MenuViewAngleTarget == 500000);
    CHECK(g_UiScriptProgress == 0 && g_UiScriptProgress2 == 0);
    CHECK(g_MenuHintBarProgress == 0 && GameMenuBusy == 0);
    CHECK(g_MenuConfirmTimer == 0 && g_MenuHintBarStep == 0);
    CHECK(g_ClassChangeApplied == 0 && g_CourseSwapDelay == 0);
    CHECK(g_MenuViewOffset == 0 && g_MenuViewOffsetTarget == 0);
    CHECK(g_CourseCardSpin == 0 && g_CourseCardSpinTarget == 0);
    CHECK(g_CourseCardPendingGrade == 0 && g_CarSwapFromIndex == 0);
    CHECK(g_MenuPendingCourseIndex == -1 && g_CarSwapToIndex == -1);
    CHECK(g_MenuOverlayPattern == 0 && g_CarNamePlateStep == 0);
    CHECK(g_MenuPlateCarIndex == 0 && g_CarSpecGraphStep == 0);
    CHECK(g_MenuUpperAltPanelStep == 0 && g_MenuLowerAltPanelStep == 0);
    CHECK(g_TimeAttackPlateStep == 0 && g_MenuHintButtonsVisible == 1);
    CHECK(g_MenuHandlerIndex == -1 && g_MenuOutgoingHandlerIndex == -1);
    CHECK(g_MenuAltLayoutSetting == 0 && g_CarShopUnlockAll == 0);
    CHECK(g_MenuScreen == MENU_SCREEN_BOOTSTRAP);
    CHECK(g_CourseSelectOption == 0 && g_CarSelectCursor == 0);
    CHECK(g_CustomizeOption == 0 && g_DesignModeOption == 0);
    CHECK(s_drawResetCalls == 14);
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
    CHECK(g_MenuCourseModelIndex == 6 && g_PlayerMoney == 123456);
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
    CHECK(g_MenuCourseModelIndex == 7 && g_PlayerMoney == 0);
    return 0;
}

int main(void) {
    CHECK(TestGrandPrixEntry() == 0);
    CHECK(TestTimeAttackEntry() == 0);
    puts("menu mode initialization tests passed");
    return 0;
}
