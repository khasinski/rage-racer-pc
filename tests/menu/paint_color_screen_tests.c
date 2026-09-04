#include "common.h"
#include "game/car.h"
#include "game/menu.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
static CarEntry s_cars[13];
CarEntry *g_CarTable = s_cars;
CarEntry g_TimeAttackCars[13];
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuOverlayPattern;
s32 g_MenuScreen;
s32 g_MenuViewOffsetTarget;
u16 g_PadPressed;
u16 g_PadPressedRepeat;
s32 g_PaintColorCursor;
s32 g_PaintColorIndex;
s32 g_PaintColorScreenProgress;
s32 g_PlayerCarIndex;
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;
TimedDrawCommand g_NativePaintColorScreenScript[15];
TimedDrawCommand g_UiChromeScript[1];

static s32 s_lastCue;
static s32 s_bodyColor1;
static s32 s_bodyColor2;

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    (void)commands;
    if (step < 0) {
        *progress = 0;
    }
    return 1;
}
void PlaySoundCue(s32 cue) { s_lastCue = cue; }
void DrawMenuCarView(void) {}
s32 DrawPaintColorPalette(s32 *progress, s32 step, s32 index) {
    (void)progress;
    (void)step;
    (void)index;
    return 1;
}
void DrawBrowseArrows(s32 step, s32 wide, s32 left, s32 right) {
    (void)step;
    (void)wide;
    (void)left;
    (void)right;
}
void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    (void)progress;
    (void)count;
    (void)slot;
}
void SetPrimaryBodyColor(s32 color) { s_bodyColor1 = color; }
void SetSecondaryBodyColor(s32 color) { s_bodyColor2 = color; }

static void Reset(void) {
    memset(s_cars, 0, sizeof(s_cars));
    memset(g_TimeAttackCars, 0, sizeof(g_TimeAttackCars));
    GameMenuBusy = 0;
    g_MenuOverlayPattern = 0;
    g_MenuScreen = 0;
    g_MenuViewOffsetTarget = 0;
    g_PadPressed = 0;
    g_PadPressedRepeat = 0;
    g_PaintColorCursor = 0;
    g_PaintColorIndex = 0;
    g_PlayerCarIndex = 3;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    s_lastCue = -1;
    s_bodyColor1 = -1;
    s_bodyColor2 = -1;
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
    g_PaintColorScreenProgress = 100;
    CHECK(DrawPaintColorScreen(0) == 0);
    CHECK(DrawPaintColorScreen(600) == MENU_FADE_MAX);
    CHECK(DrawPaintColorScreen(-600) == 0);

    Reset();
    s_cars[3].paintColor1 = 6;
    g_PadPressed = PAD_CONFIRM;
    UpdatePaintColorScreen();
    CHECK(GameMenuBusy == -1);
    CHECK(g_PaintColorIndex == 6);
    CHECK(s_lastCue == 2);

    Reset();
    s_cars[3].paintColor1 = UINT8_MAX;
    g_PadPressed = PAD_CONFIRM;
    UpdatePaintColorScreen();
    CHECK(GameMenuBusy == -1);
    CHECK(g_PaintColorIndex == MENU_PAINT_COLOR_COUNT - 1);

    Reset();
    GameMenuBusy = -1;
    g_PaintColorIndex = 17;
    g_PadPressedRepeat = PAD_RIGHT;
    UpdatePaintColorScreen();
    CHECK(g_PaintColorIndex == 0);
    CHECK(s_bodyColor1 == 0);

    Reset();
    s_cars[3].paintColor1 = 2;
    s_cars[3].paintColor2 = 4;
    GameMenuBusy = -1;
    g_PaintColorIndex = 8;
    g_PadPressed = PAD_CONFIRM;
    UpdatePaintColorScreen();
    CHECK(s_cars[3].paintColor1 == 8);
    CHECK(g_TimeAttackCars[3].paintColor1 == 8);
    CHECK(g_TimeAttackCars[3].paintColor2 == 4);
    CHECK(GameMenuBusy == 0);

    Reset();
    s_cars[3].paintColor1 = 2;
    GameMenuBusy = -1;
    g_PaintColorIndex = 8;
    g_PadPressed = PAD_CONFIRM | PAD_CANCEL;
    g_PadPressedRepeat = PAD_RIGHT;
    UpdatePaintColorScreen();
    CHECK(s_cars[3].paintColor1 == 8);
    CHECK(g_PaintColorIndex == 8);
    CHECK(s_lastCue == 2);
    CHECK(GameMenuBusy == 0);

    Reset();
    s_cars[3].paintColor2 = 11;
    GameMenuBusy = -2;
    g_PaintColorIndex = 5;
    g_PadPressed = PAD_CANCEL;
    UpdatePaintColorScreen();
    CHECK(g_PaintColorIndex == 11);
    CHECK(s_bodyColor2 == 11);
    CHECK(GameMenuBusy == 0);

    Reset();
    g_PaintColorCursor = 2;
    g_PadPressed = PAD_CONFIRM;
    UpdatePaintColorScreen();
    CHECK(GameMenuBusy == 1);
    CHECK(g_MenuViewOffsetTarget == 0x3D090);

    Reset();
    GameMenuBusy = 3;
    UpdatePaintColorScreen();
    CHECK(g_MenuScreen == 6);
    CHECK(GameMenuBusy == 0);

    Reset();
    g_PaintColorCursor = INT_MAX;
    g_PaintColorIndex = INT_MIN;
    UpdatePaintColorScreen();
    CHECK(g_PaintColorCursor == 2 && g_PaintColorIndex == 0);

    Reset();
    g_CarTable = NULL;
    g_PadPressed = PAD_CONFIRM;
    UpdatePaintColorScreen();
    CHECK(GameMenuBusy == 0 && s_lastCue == -1);

    Reset();
    g_PlayerCarIndex = CUSTOM_PAINT_CAR_COUNT;
    GameMenuBusy = -1;
    g_PadPressed = PAD_CONFIRM;
    UpdatePaintColorScreen();
    CHECK(GameMenuBusy == 0);

    Reset();
    g_PlayerCarIndex = CUSTOM_PAINT_CAR_COUNT;
    g_PadPressed = PAD_CONFIRM;
    UpdatePaintColorScreen();
    CHECK(GameMenuBusy == 0 && s_lastCue == -1);

    Reset();
    GameMenuBusy = INT_MIN;
    UpdatePaintColorScreen();
    CHECK(GameMenuBusy == 0 && s_bodyColor1 == -1 && s_bodyColor2 == -1);

    puts("paint color screen tests passed");
    return 0;
}
