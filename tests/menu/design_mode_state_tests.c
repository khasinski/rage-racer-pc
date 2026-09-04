#include "common.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

DesignModeCellMask g_DesignModeCellMask;
s32 g_DesignModeOption;
s32 g_DesignModeScreenFade;
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuOverlayPattern;
s32 g_MenuScreen;
s32 g_MenuViewAngle;
s32 g_MenuViewAngleTarget;
s32 g_MenuViewOffset;
s32 g_MenuViewOffsetTarget;
TimedDrawCommand g_NativeDesignModeDeniedScript[2];
TimedDrawCommand g_NativeDesignModeScript[16];
u16 g_PadPressed;
s32 g_PlayerCarIndex;
GameRenderState g_RenderState;
s32 g_TeamNameCharModel;
u8 g_TeamNameLength;
TimedDrawCommand g_UiChromeScript[1];
TimedDrawCommand g_UiChromeScript2[1];
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;
s32 GameMenuBusy;
s32 GameMenuCursor;

static s32 s_lastCue;
static s32 s_logoCanvasCalls;
static s32 s_nameEntryCalls;
static s32 s_rampCalls;
static s32 s_scriptsReady;
static s32 s_spriteCalls;
static s32 s_selectedCellSprites;

void DrawFadingMenuSprites(s32 progress, s32 lastRow, s32 selectedRow) {
    (void)progress;
    (void)lastRow;
    (void)selectedRow;
}
void DrawMenuCarView(void) {}
void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width,
                u16 height, u16 u, u16 v, u8 r, u8 g, u8 b, u16 clut,
                s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
    s_spriteCalls++;
    if (clut == 0x26F) s_selectedCellSprites++;
}
void DrawTeamLogoCanvas(s32 panelStep, s32 editorStep) {
    (void)panelStep;
    (void)editorStep;
    s_logoCanvasCalls++;
}
void DrawTeamNameEntry(s32 step, s32 cursor) {
    (void)step;
    (void)cursor;
    s_nameEntryCalls++;
}
void PlaySoundCue(s32 cue) { s_lastCue = cue; }
void RampTeamLogoCanvas(s32 from, s32 to) {
    (void)from;
    (void)to;
    s_rampCalls++;
}
s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    (void)commands;
    if (step < 0) *progress = 0;
    return step > 0 ? s_scriptsReady : 0;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetState(void) {
    g_DesignModeOption = 0;
    g_MenuHandlerIndex = MENU_SCREEN_DESIGN_MODE;
    g_MenuOutgoingHandlerIndex = -1;
    g_MenuOverlayPattern = 0;
    g_MenuScreen = MENU_SCREEN_DESIGN_MODE;
    g_MenuViewOffset = 0;
    g_MenuViewOffsetTarget = -1;
    g_PadPressed = 0;
    g_PlayerCarIndex = 0;
    g_TeamNameLength = 0;
    g_UiScriptProgress = 1;
    g_UiScriptProgress2 = 0;
    GameMenuBusy = 0;
    s_lastCue = 0;
    s_logoCanvasCalls = 0;
    s_nameEntryCalls = 0;
    s_rampCalls = 0;
    s_spriteCalls = 0;
    s_selectedCellSprites = 0;
    s_scriptsReady = 1;
}

static int CheckChoice(s32 option, s32 expectedBusy, s32 expectedCue) {
    ResetState();
    g_DesignModeOption = option;
    g_PadPressed = PAD_CONFIRM;
    UpdateDesignModeScreen();
    CHECK(GameMenuBusy == expectedBusy);
    CHECK(s_lastCue == expectedCue);
    return 0;
}

static int CheckExit(s32 busy, s32 expectedScreen) {
    ResetState();
    GameMenuBusy = busy;
    UpdateDesignModeScreen();
    CHECK(GameMenuBusy == 0);
    CHECK(g_MenuScreen == expectedScreen);
    CHECK(g_MenuHandlerIndex == expectedScreen);
    CHECK(g_MenuOutgoingHandlerIndex == MENU_SCREEN_DESIGN_MODE);
    return 0;
}

int main(void) {
    GameOrderingTableEntry ot[4];

    memset(&g_DesignModeCellMask, 0, sizeof(g_DesignModeCellMask));
    memset(ot, 0, sizeof(ot));
    RENDER_OT_BASE = ot;
    g_DesignModeCellMask.cells[2][3] = 1;
    g_DesignModeScreenFade = 123;
    CHECK(DrawDesignModeScreen(0) == 0 && s_spriteCalls == 0);
    CHECK(DrawDesignModeScreen(MENU_FADE_MAX) == MENU_FADE_MAX);
    CHECK(s_spriteCalls == 38 && s_selectedCellSprites == 1);

    s_spriteCalls = 0;
    RENDER_OT_BASE = NULL;
    CHECK(DrawDesignModeScreen(-1) == MENU_FADE_MAX - 1);
    CHECK(s_spriteCalls == 0);
    RENDER_OT_BASE = ot;

    if (CheckChoice(0, 1, 2)) return 1;
    CHECK(s_rampCalls == 1);
    if (CheckChoice(1, 2, 2)) return 1;
    if (CheckChoice(2, 3, 2)) return 1;
    if (CheckChoice(3, 4, 3)) return 1;
    if (CheckChoice(INT_MIN, 1, 2)) return 1;
    CHECK(g_DesignModeOption == 0);
    if (CheckChoice(INT_MAX, 4, 3)) return 1;
    CHECK(g_DesignModeOption == 3);

    ResetState();
    g_DesignModeOption = 2;
    g_PlayerCarIndex = CUSTOM_PAINT_CAR_COUNT;
    g_PadPressed = PAD_CONFIRM;
    UpdateDesignModeScreen();
    CHECK(GameMenuBusy == -1 && s_lastCue == 5);
    g_PadPressed = PAD_CANCEL;
    UpdateDesignModeScreen();
    CHECK(GameMenuBusy == 0);

    if (CheckExit(1, MENU_SCREEN_TEAM_LOGO)) return 1;
    CHECK(s_logoCanvasCalls == 1);
    g_TeamNameLength = MENU_TEAM_NAME_MAX_LENGTH;
    if (CheckExit(2, MENU_SCREEN_TEAM_NAME)) return 1;
    CHECK(s_nameEntryCalls == 1);
    if (CheckExit(3, MENU_SCREEN_PAINT_COLOR)) return 1;
    if (CheckExit(4, MENU_SCREEN_CUSTOMIZE)) return 1;

    puts("design mode state tests passed");
    return 0;
}
