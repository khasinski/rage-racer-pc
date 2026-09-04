#include "common.h"
#include "game/menu.h"
#include "game/race.h"

#include <limits.h>
#include <stdio.h>

s32 GameMenuBusy;
s32 g_CourseIndex;
s32 g_MenuAltLayout;
s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuOverlayPattern;
s32 g_MenuScreen;
s32 g_RankingCursor;
s32 g_RankingPendingState;
s32 g_RankingScrollState;
s32 g_TimeAttackPlateStep;
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;
u16 g_PadPressed;
TimedDrawCommand g_RankingMenuScript[9];
TimedDrawCommand g_RankingPanelScript[5];
TimedDrawCommand g_UiChromeScript[1];

static s32 s_scriptResult;
static s32 s_tableResult;
static s32 s_lastCue;
static s32 s_timeAttackPlateStep;

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    (void)commands;
    (void)progress;
    (void)step;
    return s_scriptResult;
}

s32 DrawRankingTable(s32 *progress, s32 step, RankingTableKind table) {
    (void)progress;
    (void)step;
    (void)table;
    return s_tableResult;
}

void DrawFadingMenuSprites(s32 progress, s32 count, s32 selected) {
    (void)progress;
    (void)count;
    (void)selected;
}
void DrawMenuCourseView(void) {}
void DrawMenuLightBurst(s32 step) { (void)step; }
void DrawTimeAttackPlate(s32 step) { s_timeAttackPlateStep = step; }
void PlaySoundCue(s32 cue) { s_lastCue = cue; }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    GameMenuBusy = 0;
    g_MenuOverlayPattern = 0;
    g_PadPressed = 0;
    g_RankingCursor = 0;
    g_RankingPendingState = 0;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    s_scriptResult = 0;
    s_tableResult = 0;
    s_lastCue = 0;
    s_timeAttackPlateStep = -99;
    g_MenuScreen = 0;
    g_MenuHandlerIndex = 0;
    g_MenuOutgoingHandlerIndex = 0;
}

int main(void) {
    g_RankingScrollState = 100;
    CHECK(DrawRankingScreen(20) == 120);
    CHECK(DrawRankingScreen(0) == 0 && g_RankingScrollState == 0);

    Reset();
    g_UiScriptProgress2 = 9;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -1 && g_UiScriptProgress2 == 0);

    s_scriptResult = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -2 && g_RankingPendingState == -3);
    CHECK(s_lastCue == 2);

    g_UiScriptProgress2 = 0;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -3);

    s_tableResult = 1;
    g_PadPressed = PAD_CANCEL;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -4 && s_lastCue == 3);
    g_UiScriptProgress2 = 0;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -1);

    g_RankingCursor = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -2 && g_RankingPendingState == -5);
    g_UiScriptProgress2 = 0;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -5);
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -6);
    g_UiScriptProgress2 = 0;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -1);

    g_RankingCursor = 2;
    g_PadPressed = PAD_CONFIRM;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == 1 && g_MenuOverlayPattern == 2 && s_lastCue == 3);

    g_CourseIndex = 4;
    g_UiScriptProgress = 0;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == 0 && g_MenuScreen == 1);
    CHECK(g_MenuHandlerIndex == 1 && g_MenuOutgoingHandlerIndex == 2);
    CHECK(g_RankingCursor == 0 && g_TimeAttackPlateStep == 1);
    CHECK(s_timeAttackPlateStep == 0);

    GameMenuBusy = 1;
    g_CourseIndex = 3;
    UpdateRankingScreen();
    CHECK(g_TimeAttackPlateStep == -1);

    Reset();
    GameMenuBusy = -1;
    s_scriptResult = 1;
    g_PadPressed = PAD_CANCEL;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == 1 && g_MenuOverlayPattern == 2);

    Reset();
    GameMenuBusy = -2;
    g_RankingPendingState = INT_MAX;
    g_UiScriptProgress2 = 0;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -1);

    Reset();
    GameMenuBusy = INT_MIN;
    g_RankingCursor = INT_MAX;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -1 && g_RankingCursor == 2);

    Reset();
    GameMenuBusy = INT_MAX;
    UpdateRankingScreen();
    CHECK(GameMenuBusy == -1 && g_MenuScreen == 0);

    puts("ranking screen tests passed");
    return 0;
}
