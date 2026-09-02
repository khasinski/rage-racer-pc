#include "common.h"
#include "game/menu.h"

#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
s32 GameMenuCursor;
s32 GameMenuCursorAnim;
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
u16 g_PadPressed;
u16 g_PadPressedRepeat;
u8 g_TeamNameChars[16];
u8 g_TeamNameLength;
s32 g_TeamNameScreenProgress;
s32 g_UiScriptProgress;
TimedDrawCommand g_NativeTeamNameScreenScript[61];

static s32 s_lastCue;
static s32 s_uploadCalls;
static s32 s_uploadedLength;

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    (void)commands;
    if (step < 0) {
        *progress = 0;
    }
    return 1;
}
void PlaySoundCue(s32 cue) { s_lastCue = cue; }
void DrawTeamNameCharModel(void) {}
void DrawTeamNameEntry(s32 step, s32 cursor) {
    (void)step;
    (void)cursor;
}
void UploadTeamNameTexture(const u8 *text, s32 length) {
    (void)text;
    s_uploadCalls++;
    s_uploadedLength = length;
}

static void Reset(void) {
    GameMenuBusy = 0;
    GameMenuCursor = 0;
    GameMenuCursorAnim = -1;
    g_MenuOverlayPattern = 0;
    g_MenuViewAngle = 0;
    g_MenuViewAngleTarget = 1;
    g_MenuViewOffset = 0;
    g_MenuViewOffsetTarget = 0;
    g_PadPressed = 0;
    g_PadPressedRepeat = 0;
    memset(g_TeamNameChars, 0, sizeof(g_TeamNameChars));
    g_TeamNameLength = 0;
    g_UiScriptProgress = 0;
    s_lastCue = -1;
    s_uploadCalls = 0;
    s_uploadedLength = -1;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int CheckCursorMove(s32 from, u16 pressed, s32 expected) {
    Reset();
    GameMenuCursor = from;
    g_PadPressedRepeat = pressed;
    UpdateTeamNameScreen();
    CHECK(GameMenuCursor == expected);
    CHECK(GameMenuCursorAnim == expected);
    CHECK(g_MenuViewAngle == 0x3E8000);
    CHECK(g_MenuViewAngleTarget == 0);
    CHECK(s_lastCue == 1);
    return 0;
}

int main(void) {
    g_TeamNameScreenProgress = 100;
    CHECK(DrawTeamNameScreen(0) == 0);
    CHECK(DrawTeamNameScreen(600) == MENU_FADE_MAX);
    CHECK(DrawTeamNameScreen(-600) == 0);

    if (CheckCursorMove(0, PAD_UP, 33) ||
        CheckCursorMove(33, PAD_DOWN, 0) ||
        CheckCursorMove(0, PAD_LEFT, 10) ||
        CheckCursorMove(10, PAD_RIGHT, 0)) {
        return 1;
    }

    Reset();
    g_TeamNameLength = 6;
    GameMenuCursor = 0x2A;
    g_PadPressedRepeat = PAD_RIGHT;
    UpdateTeamNameScreen();
    CHECK(GameMenuCursor == 0x2B);

    Reset();
    GameMenuCursor = 5;
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamNameScreen();
    CHECK(g_TeamNameLength == 1);
    CHECK(g_TeamNameChars[0] == 5);
    CHECK(s_lastCue == 2);

    Reset();
    g_TeamNameLength = 2;
    g_PadPressed = PAD_CANCEL;
    UpdateTeamNameScreen();
    CHECK(g_TeamNameLength == 1);
    CHECK(g_TeamNameChars[2] == 0xA);
    CHECK(s_lastCue == 4);

    Reset();
    GameMenuCursor = 0x2B;
    g_PadPressed = PAD_CONFIRM;
    UpdateTeamNameScreen();
    CHECK(GameMenuBusy == 1);
    CHECK(g_MenuOverlayPattern == 2);
    CHECK(g_MenuViewOffsetTarget == 0x3D090);

    Reset();
    GameMenuBusy = 1;
    g_TeamNameLength = 4;
    g_MenuViewOffset = 0x3D090;
    UpdateTeamNameScreen();
    CHECK(g_MenuScreen == 6);
    CHECK(GameMenuBusy == 0);
    CHECK(s_uploadCalls == 1);
    CHECK(s_uploadedLength == 4);

    puts("team name screen tests passed");
    return 0;
}
