#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

s32 GameMenuBusy;
s32 g_CarNamePlateStep;
CarModelAsset *g_CarModelAsset;
s32 g_CarSpecGraphStep;
CarEntry *g_CarTable;
s32 g_CustomizeOption;
const TimedDrawCommand *g_CustomizePopupScript;
s16 g_GrandPrixMode;
s32 g_MenuAltLayout;
s32 g_MenuAltLayoutSetting;
s32 g_MenuConfirmTimer;
s32 g_MenuHandlerIndex;
s32 g_MenuOutgoingHandlerIndex;
s32 g_MenuOverlayPattern;
s32 g_MenuPlateCarIndex;
s32 g_MenuScreen;
u8 g_MenuSubCursor;
s32 g_MenuViewOffset;
s32 g_MenuViewOffsetTarget;
TimedDrawCommand g_NativeCustomizeMenuScriptGp[13];
TimedDrawCommand g_NativeCustomizeMenuScriptTimeAttack[11];
TimedDrawCommand g_NativeMenuDialogPanelLowerScript[8];
TimedDrawCommand g_NativeMenuDialogPanelUpperScript[4];
TimedDrawCommand g_NativeTransmissionUnavailableScript[4];
u16 g_PadPressed;
s32 g_PlayerCarIndex;
GameRenderState g_RenderState;
CarEntry g_TimeAttackCars[GAME_CAR_COUNT];
TimedDrawCommand g_UiChromeScript[1];
TimedDrawCommand g_UiChromeScript2[1];
s32 g_UiScriptProgress;
s32 g_UiScriptProgress2;

static CarEntry s_cars[GAME_CAR_COUNT];
static CarModelAsset s_model;
static s32 s_lastCue;

s32 RunTimedDrawScript(const TimedDrawCommand *commands, s32 *progress,
                       s32 step) {
    (void)commands;
    if (step < 0) *progress = 0;
    return 1;
}
void PlaySoundCue(s32 cue) { s_lastCue = cue; }
void DrawCarNamePlate(s32 step, s32 model, s32 grade) {
    (void)step;
    (void)model;
    (void)grade;
}
void DrawMenuCarView(void) {}
void DrawFadingMenuSprites(s32 progress, s32 count, s32 slot) {
    (void)progress;
    (void)count;
    (void)slot;
}
void DrawMenuCursorBox(s32 x, s32 y, s32 w, s32 h, s32 flash) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)flash;
}
void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 w, u16 h,
                u16 u, u16 v, u8 r, u8 g, u8 b, u16 clut, s32 shade,
                s32 semi, u32 flags) {
    (void)ot;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)u;
    (void)v;
    (void)r;
    (void)g;
    (void)b;
    (void)clut;
    (void)shade;
    (void)semi;
    (void)flags;
}
void GameDrawMenuButton(s32 x, s32 y, s32 w, s32 h, u8 r, u8 g, u8 b) {
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)r;
    (void)g;
    (void)b;
}
void DrawTireCompoundSlider(u8 compound, s32 flash) {
    (void)compound;
    (void)flash;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(s_cars, 0, sizeof(s_cars));
    memset(g_TimeAttackCars, 0, sizeof(g_TimeAttackCars));
    memset(&s_model, 0, sizeof(s_model));
    g_CarTable = s_cars;
    g_CarModelAsset = &s_model;
    g_PlayerCarIndex = 3;
    g_GrandPrixMode = 1;
    g_CustomizeOption = 0;
    g_CustomizePopupScript = NULL;
    GameMenuBusy = 0;
    g_PadPressed = 0;
    g_UiScriptProgress = 0;
    g_UiScriptProgress2 = 0;
    g_MenuSubCursor = 0;
    g_MenuConfirmTimer = 0;
    s_lastCue = -1;
}

int main(void) {
    Reset();
    g_PadPressed = PAD_UP;
    UpdateCustomizeScreen();
    CHECK(g_CustomizeOption == 3 && s_lastCue == 1);

    Reset();
    g_CustomizeOption = 3;
    g_PadPressed = PAD_DOWN;
    UpdateCustomizeScreen();
    CHECK(g_CustomizeOption == 0 && s_lastCue == 1);

    Reset();
    g_GrandPrixMode = 0;
    g_PadPressed = PAD_UP;
    UpdateCustomizeScreen();
    CHECK(g_CustomizeOption == 2);

    Reset();
    g_CarTable = NULL;
    g_PadPressed = PAD_CONFIRM;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == 0 && s_lastCue == -1);

    Reset();
    g_PlayerCarIndex = GAME_CAR_COUNT;
    g_CustomizeOption = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == -3 && s_lastCue == 5);

    Reset();
    s_cars[3].tireCompound = UINT8_MAX;
    g_PadPressed = PAD_CONFIRM;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == -1 && g_MenuSubCursor == 4);

    GameMenuBusy = -5;
    g_MenuSubCursor = UINT8_MAX;
    g_MenuConfirmTimer = 0;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == 0 && s_cars[3].tireCompound == 4 &&
          g_TimeAttackCars[3].tireCompound == 4);

    Reset();
    s_model.transmissionAvailable = 1;
    s_cars[3].transmission = 7;
    g_CustomizeOption = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == -2 && g_MenuSubCursor == 1);

    g_PadPressed = PAD_CONFIRM;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == -6 && s_cars[3].transmission == 1 &&
          g_TimeAttackCars[3].transmission == 1);

    Reset();
    s_model.transmissionAvailable = 1;
    GameMenuBusy = -2;
    g_MenuSubCursor = 1;
    g_CarModelAsset = NULL;
    g_PadPressed = PAD_CONFIRM;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == 0 && s_cars[3].transmission == 0 &&
          g_TimeAttackCars[3].transmission == 0);

    Reset();
    g_GrandPrixMode = 0;
    g_CustomizeOption = INT32_MAX;
    g_PadPressed = PAD_CONFIRM;
    UpdateCustomizeScreen();
    CHECK(g_CustomizeOption == 2 && GameMenuBusy == 2);

    Reset();
    GameMenuBusy = -99;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == 0);

    Reset();
    GameMenuBusy = 99;
    UpdateCustomizeScreen();
    CHECK(GameMenuBusy == 0);

    puts("customize screen tests passed");
    return 0;
}
