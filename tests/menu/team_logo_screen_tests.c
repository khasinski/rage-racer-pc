#include "common.h"
#include "game/audio.h"
#include "game/menu.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

static MenuWidgets s_menuWidgets;
static TeamLogo s_teamLogo;

extern s32 g_MenuHandlerIndex;
extern s32 g_MenuOutgoingHandlerIndex;
extern s32 g_MenuScreen;

void MenuActivateScreen(s32 screen) {
    g_MenuScreen = screen;
    g_MenuHandlerIndex = screen;
}
void MenuBeginExit(s32 screen) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = screen;
}

s32 GameMenuBusy;
s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuOverlayPattern;
s32 g_MenuScreen;
u16 g_PadPressed;
u16 g_TeamLogoClut[16];
Rect g_TeamLogoClutRect;
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;

TimedDrawCommand g_NativeMenuDialogPanelUpperScript[4];
TimedDrawCommand g_NativeLogoSampleScreenScript[12];
TimedDrawCommand g_NativeMenuRow0MarkerScript[4];
TimedDrawCommand g_NativeMenuRow1MarkerScript[16];
TimedDrawCommand g_NativeTeamLogoScreenScript[12];
TimedDrawCommand g_NativeTeamLogoScreenScript2[2];
TimedDrawCommand g_UiChromeScript[1];
TimedDrawCommand g_UiChromeScript2[1];
TimedDrawCommand g_EmptyScript[1];

GameRenderState g_RenderState;
static LogoSample s_logo;

LogoSample *MenuLogoSample(void) { return &s_logo; }
TeamLogo *MenuTeamLogo(void) { return &s_teamLogo; }

static s32 s_scriptFinished = 1;
static s32 s_canvasUpdates;
static s32 s_duckCalls;
static s32 s_restoreCalls;
static s32 s_clutUploadCalls;
static s32 s_samplePanelCalls;
static s32 s_composedCharacter;
static s32 s_composedBackground;

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
void RampTeamLogoCanvas(TeamLogo *logo, s32 from, s32 to) {
    (void)logo;
    (void)from;
    (void)to;
}
void UploadTeamLogoClut(void) { s_clutUploadCalls++; }
void DrawTeamLogoCanvas(TeamLogo *logo, s32 panelStep, s32 editorStep) {
    (void)logo;
    (void)panelStep;
    (void)editorStep;
}
void ComposeSampleTeamLogo(TeamLogo *logo, s32 character, s32 background) {
    (void)logo;
    s_composedCharacter = character;
    s_composedBackground = background;
}
void UpdateTeamLogoCanvas(TeamLogo *logo) {
    (void)logo;
    s_canvasUpdates++;
}
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
void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width, u16 height, u16 u, u16 v,
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
void DrawLogoSamplePanel(LogoSample *samplePanel, const TeamLogo *logo,
                         s32 step, s32 sample) {
    (void)samplePanel;
    (void)logo;
    (void)step;
    (void)sample;
    s_samplePanelCalls++;
}
static void Reset(void) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    GameMenuBusy = 0;
    g_MenuScreen = 0;
    g_MenuHandlerIndex = 0;
    g_MenuOutgoingHandlerIndex = 0;
    g_MenuOverlayPattern = 0;
    s_teamLogo.confirmTimer = 0;
    s_teamLogo.modalCursor = 0;
    g_PadPressed = 0;
    s_teamLogo.option = 0;
    s_teamLogo.paintArmed = 1;
    s_teamLogo.subPanelScript = g_EmptyScript;
    s_logo.background = 0;
    s_logo.character = 0;
    s_logo.cursor = 0;
    s_logo.saved = 0;
    s_logo.subPanelScript = g_EmptyScript;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    s_canvasUpdates = 0;
    s_duckCalls = 0;
    s_restoreCalls = 0;
    s_clutUploadCalls = 0;
    s_samplePanelCalls = 0;
    s_composedCharacter = -1;
    s_composedBackground = -1;
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
    CHECK(s_teamLogo.subPanelScript == g_MenuDialogPanelUpperScript);

    Reset();
    s_teamLogo.option = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == -3);
    CHECK(s_teamLogo.paintArmed == 0);
    CHECK(s_teamLogo.subPanelScript == g_MenuRow1MarkerScript);

    Reset();
    GameMenuBusy = -1;
    g_PadPressed = PAD_LEFT;
    UpdateTeamLogoScreen();
    CHECK(s_teamLogo.modalCursor == 1);
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == -2);
    CHECK(s_teamLogo.confirmTimer == 0x23);

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
    CHECK(s_clutUploadCalls == 1);

    Reset();
    s_teamLogo.option = INT_MAX;
    s_teamLogo.modalCursor = UINT8_MAX;
    UpdateTeamLogoScreen();
    CHECK(s_teamLogo.option == 2 && s_teamLogo.modalCursor == 1);

    Reset();
    GameMenuBusy = -2;
    s_teamLogo.confirmTimer = INT_MAX;
    UpdateTeamLogoScreen();
    CHECK(s_teamLogo.confirmTimer == 34 && GameMenuBusy == -2);

    Reset();
    GameMenuBusy = INT_MIN;
    g_UiScriptProgress2 = 12;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == 0 && g_UiScriptProgress2 == 12);

    Reset();
    GameMenuBusy = INT_MAX;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == 0 && g_MenuScreen == 0);

    Reset();
    s_logo.character = 7;
    s_logo.background = 9;
    g_PadPressed = PAD_CONFIRM;
    UpdateLogoSampleScreen();
    CHECK(s_composedCharacter == 7);
    CHECK(s_composedBackground == 9);
    CHECK(GameMenuBusy == -1);
    CHECK(s_logo.saved == 7);
    CHECK(s_logo.subPanelScript == g_MenuRow0MarkerScript);

    Reset();
    s_logo.cursor = 1;
    s_logo.background = 11;
    g_PadPressed = PAD_CONFIRM;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == -2);
    CHECK(s_logo.saved == 11);
    CHECK(s_logo.subPanelScript == g_MenuRow1MarkerScript);

    Reset();
    s_logo.cursor = 2;
    g_PadPressed = PAD_CONFIRM;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 1 && g_MenuOverlayPattern == 2);

    Reset();
    g_PadPressed = PAD_CANCEL;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 1 && g_MenuOverlayPattern == 2);

    Reset();
    GameMenuBusy = -1;
    s_logo.character = 0;
    g_PadPressed = PAD_LEFT;
    UpdateLogoSampleScreen();
    CHECK(s_logo.character == 19);

    Reset();
    GameMenuBusy = -2;
    s_logo.background = 19;
    g_PadPressed = PAD_RIGHT;
    UpdateLogoSampleScreen();
    CHECK(s_logo.background == 0);

    Reset();
    GameMenuBusy = -1;
    s_logo.character = 8;
    g_PadPressed = PAD_CONFIRM | PAD_RIGHT;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 0);
    CHECK(s_logo.saved == 8);
    CHECK(s_logo.character == 8);

    Reset();
    GameMenuBusy = -2;
    s_logo.background = 12;
    s_logo.saved = 4;
    g_PadPressed = PAD_CANCEL;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 0);
    CHECK(s_logo.background == 4);

    Reset();
    s_logo.cursor = INT_MAX;
    s_logo.character = INT_MIN;
    s_logo.background = INT_MAX;
    s_logo.saved = INT_MAX;
    UpdateLogoSampleScreen();
    CHECK(s_logo.cursor == 2);
    CHECK(s_logo.character == 0 && s_logo.background == 19);
    CHECK(s_logo.saved == 19);
    CHECK(s_composedCharacter == 0 && s_composedBackground == 19);

    Reset();
    GameMenuBusy = INT_MIN;
    s_logo.character = 6;
    s_logo.background = 7;
    g_PadPressed = PAD_RIGHT;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 0);
    CHECK(s_logo.character == 6 && s_logo.background == 7);

    puts("logo screen state tests passed");
    return 0;
}

MenuWidgets *MenuWidgetState(void) { return &s_menuWidgets; }
