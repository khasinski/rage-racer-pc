#include "common.h"
#include "game/audio.h"
#include "game/menu.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 g_TeamLogoScreenFade;

s32 GameMenuBusy;
s32 g_LogoSampleBackIndex;
s32 g_LogoSampleCharIndex;
s32 g_LogoSampleCursor;
s32 g_LogoSampleSavedIndex;
s32 g_LogoSampleScreenFade;
s32 g_MenuAltLayout;
s32 g_MenuConfirmTimer;
s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
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
TimedDrawCommand g_NativeLogoSampleScreenScript[12];
TimedDrawCommand g_NativeMenuRow0MarkerScript[4];
TimedDrawCommand g_NativeMenuRow1MarkerScript[16];
TimedDrawCommand g_NativeTeamLogoScreenScript[12];
TimedDrawCommand g_NativeTeamLogoScreenScript2[2];
TimedDrawCommand g_UiChromeScript[1];
TimedDrawCommand g_UiChromeScript2[1];
TimedDrawCommand g_EmptyScript[1];
const TimedDrawCommand *g_TeamLogoSubPanelScript = g_EmptyScript;
const TimedDrawCommand *g_LogoSampleSubPanelScript = g_EmptyScript;

GameRenderState g_RenderState;

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
void RampTeamLogoCanvas(s32 from, s32 to) {
    (void)from;
    (void)to;
}
void UploadTeamLogoClut(void) { s_clutUploadCalls++; }
void DrawTeamLogoCanvas(s32 panelStep, s32 editorStep) {
    (void)panelStep;
    (void)editorStep;
}
void ComposeSampleTeamLogo(s32 character, s32 background) {
    s_composedCharacter = character;
    s_composedBackground = background;
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
void DrawLogoSamplePanel(s32 step, s32 sample) {
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
    g_MenuConfirmTimer = 0;
    g_MenuSubCursor = 0;
    g_PadPressed = 0;
    g_TeamLogoOption = 0;
    g_TeamLogoPaintArmed = 1;
    g_TeamLogoSubPanelScript = g_EmptyScript;
    g_LogoSampleBackIndex = 0;
    g_LogoSampleCharIndex = 0;
    g_LogoSampleCursor = 0;
    g_LogoSampleSavedIndex = 0;
    g_LogoSampleScreenFade = 0;
    g_LogoSampleSubPanelScript = g_EmptyScript;
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
    CHECK(g_TeamLogoSubPanelScript == g_MenuDialogPanelUpperScript);

    Reset();
    g_TeamLogoOption = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamLogoScreen();
    CHECK(GameMenuBusy == -3);
    CHECK(g_TeamLogoPaintArmed == 0);
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
    CHECK(s_clutUploadCalls == 1);

    Reset();
    g_TeamLogoOption = INT_MAX;
    g_MenuSubCursor = UINT8_MAX;
    UpdateTeamLogoScreen();
    CHECK(g_TeamLogoOption == 2 && g_MenuSubCursor == 1);

    Reset();
    GameMenuBusy = -2;
    g_MenuConfirmTimer = INT_MAX;
    UpdateTeamLogoScreen();
    CHECK(g_MenuConfirmTimer == 34 && GameMenuBusy == -2);

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
    g_LogoSampleCharIndex = 7;
    g_LogoSampleBackIndex = 9;
    g_PadPressed = PAD_CONFIRM;
    UpdateLogoSampleScreen();
    CHECK(s_composedCharacter == 7);
    CHECK(s_composedBackground == 9);
    CHECK(GameMenuBusy == -1);
    CHECK(g_LogoSampleSavedIndex == 7);
    CHECK(g_LogoSampleSubPanelScript == g_MenuRow0MarkerScript);

    Reset();
    g_LogoSampleCursor = 1;
    g_LogoSampleBackIndex = 11;
    g_PadPressed = PAD_CONFIRM;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == -2);
    CHECK(g_LogoSampleSavedIndex == 11);
    CHECK(g_LogoSampleSubPanelScript == g_MenuRow1MarkerScript);

    Reset();
    g_LogoSampleCursor = 2;
    g_PadPressed = PAD_CONFIRM;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 1 && g_MenuOverlayPattern == 2);

    Reset();
    g_PadPressed = PAD_CANCEL;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 1 && g_MenuOverlayPattern == 2);

    Reset();
    GameMenuBusy = -1;
    g_LogoSampleCharIndex = 0;
    g_PadPressed = PAD_LEFT;
    UpdateLogoSampleScreen();
    CHECK(g_LogoSampleCharIndex == 19);

    Reset();
    GameMenuBusy = -2;
    g_LogoSampleBackIndex = 19;
    g_PadPressed = PAD_RIGHT;
    UpdateLogoSampleScreen();
    CHECK(g_LogoSampleBackIndex == 0);

    Reset();
    GameMenuBusy = -1;
    g_LogoSampleCharIndex = 8;
    g_PadPressed = PAD_CONFIRM | PAD_RIGHT;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 0);
    CHECK(g_LogoSampleSavedIndex == 8);
    CHECK(g_LogoSampleCharIndex == 8);

    Reset();
    GameMenuBusy = -2;
    g_LogoSampleBackIndex = 12;
    g_LogoSampleSavedIndex = 4;
    g_PadPressed = PAD_CANCEL;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 0);
    CHECK(g_LogoSampleBackIndex == 4);

    Reset();
    g_LogoSampleCursor = INT_MAX;
    g_LogoSampleCharIndex = INT_MIN;
    g_LogoSampleBackIndex = INT_MAX;
    g_LogoSampleSavedIndex = INT_MAX;
    UpdateLogoSampleScreen();
    CHECK(g_LogoSampleCursor == 2);
    CHECK(g_LogoSampleCharIndex == 0 && g_LogoSampleBackIndex == 19);
    CHECK(g_LogoSampleSavedIndex == 19);
    CHECK(s_composedCharacter == 0 && s_composedBackground == 19);

    Reset();
    GameMenuBusy = INT_MIN;
    g_LogoSampleCharIndex = 6;
    g_LogoSampleBackIndex = 7;
    g_PadPressed = PAD_RIGHT;
    UpdateLogoSampleScreen();
    CHECK(GameMenuBusy == 0);
    CHECK(g_LogoSampleCharIndex == 6 && g_LogoSampleBackIndex == 7);

    g_LogoSampleScreenFade = 100;
    CHECK(DrawLogoSampleScreen(25) == 125);
    CHECK(DrawLogoSampleScreen(-200) == 0);
    CHECK(DrawLogoSampleScreen(600) == MENU_FADE_MAX);
    CHECK(DrawLogoSampleScreen(0) == 0);

    puts("logo screen state tests passed");
    return 0;
}
