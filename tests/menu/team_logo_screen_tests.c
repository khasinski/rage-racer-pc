#include "common.h"
#include "game/audio.h"
#include "game/menu.h"

#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
s32 g_LogoSampleScreenFade;
s32 g_MenuAltLayout;
s32 g_MenuConfirmTimer;
s32 g_MenuHandlerIndex;
s32 g_MenuHandlerIndex2;
s32 g_MenuHintButtonsVisible;
s32 g_MenuOverlayPattern;
s32 g_MenuScreen;
u8 g_MenuSubCursor;
u16 g_PadPressed;
s32 g_TeamLogoOption;
s32 g_TeamLogoPaintArmed;
u16 g_TeamLogoClut[16];
Rect g_TeamLogoClutRect;
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;

TimedDrawCommand g_NativeMenuDialogPanelUpperScript[4];
TimedDrawCommand g_NativeMenuRow1MarkerScript[16];
TimedDrawCommand g_NativeTeamLogoScreenScript[12];
TimedDrawCommand g_NativeTeamLogoScreenScript2[2];
TimedDrawCommand g_UiChromeScript[1];
TimedDrawCommand g_UiChromeScript2[1];
TimedDrawCommand g_EmptyScript[1];
const TimedDrawCommand *g_TeamLogoSubPanelScript = g_EmptyScript;

GameRenderState g_RenderState;

static s32 s_scriptFinished = 1;
static s32 s_canvasUpdates;
static s32 s_duckCalls;
static s32 s_restoreCalls;
static s32 s_samplePanelCalls;

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    (void)commands;
    if (step > 0 && s_scriptFinished) {
        *progress = 16;
    } else if (step < 0) {
        *progress = 0;
    }
    return s_scriptFinished;
}

void PlaySoundCue(s32 cue) { (void)cue; }
void ApplyDuckedSequenceAudio(void) { s_duckCalls++; }
void ApplyCurrentSequenceAudio(void) { s_restoreCalls++; }
void RampTeamLogoCanvas(s32 from, s32 to) {
    (void)from;
    (void)to;
}
void DrawTeamLogoCanvas(s32 panelStep, s32 editorStep) {
    (void)panelStep;
    (void)editorStep;
}
void UpdateTeamLogoCanvas(void) { s_canvasUpdates++; }
void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    (void)progress;
    (void)count;
    (void)slot;
}
void DrawMenuCursorBox(s32 x, s32 y, s32 width, s32 height, s32 flash) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)flash;
}
void DrawSprite(void *ot, s16 x, s16 y, s16 width, u16 height, u16 u, u16 v,
                u8 r, u8 g, u8 b, u16 clut, s32 shade, s32 semiTrans,
                u32 flags) {
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
    (void)shade;
    (void)semiTrans;
    (void)flags;
}
void GameDrawMenuButton(s32 x, s32 y, s32 width, s32 height, u8 r, u8 g,
                        u8 b) {
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)r;
    (void)g;
    (void)b;
}
void DrawLogoSamplePanel(s32 step, s32 sample) {
    (void)step;
    (void)sample;
    s_samplePanelCalls++;
}
static void Reset(void) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    GameMenuBusy = 0;
    g_MenuOverlayPattern = 0;
    g_MenuSubCursor = 0;
    g_PadPressed = 0;
    g_TeamLogoOption = 0;
    g_TeamLogoPaintArmed = 1;
    g_TeamLogoSubPanelScript = g_EmptyScript;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    s_canvasUpdates = 0;
    s_duckCalls = 0;
    s_restoreCalls = 0;
    s_samplePanelCalls = 0;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    Reset();
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == -1);
    CHECK(g_TeamLogoSubPanelScript == g_MenuDialogPanelUpperScript);

    Reset();
    g_TeamLogoOption = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == -3);
    CHECK(g_TeamLogoPaintArmed == 0);
    CHECK(s_duckCalls == 1);
    CHECK(g_TeamLogoSubPanelScript == g_MenuRow1MarkerScript);

    Reset();
    GameMenuBusy = -1;
    g_PadPressed = PAD_LEFT;
    UpdateTeamLogoScreen();
    CHECK(g_MenuSubCursor == 1);
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == -2);
    CHECK(g_MenuConfirmTimer == 0x23);

    Reset();
    GameMenuBusy = -3;
    g_PadPressed = PAD_START;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == -4);
    CHECK(s_restoreCalls == 1);
    CHECK(s_canvasUpdates == 1);

    Reset();
    GameMenuBusy = 1;
    UpdateTeamLogoScreen();
    CHECK(g_MenuScreen == MENU_SCREEN_LOGO_SAMPLE);
    CHECK(s_samplePanelCalls == 1);

    Reset();
    GameMenuBusy = 2;
    g_TeamLogoClut[0] = 0xFFFF;
    UpdateTeamLogoScreen();
    CHECK(g_MenuScreen == MENU_SCREEN_DESIGN_MODE);
    CHECK(g_TeamLogoClut[0] == 0);

    puts("team logo screen tests passed");
    return 0;
}
