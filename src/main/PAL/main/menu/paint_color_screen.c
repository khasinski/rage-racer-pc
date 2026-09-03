#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"

s32 DrawPaintColorScreen(s32 step) {
    return AdvanceMenuFade(&g_PaintColorScreenProgress, step);
}

static void LeavePaintColorScreen(s32 busyState) {
    PlaySoundCue(3);
    GameMenuBusy = busyState;
    g_MenuOverlayPattern = 2;
    g_MenuViewOffsetTarget = 0x3D090;
}

static void ChoosePaintColorRow(void) {
    CarEntry *car = &g_CarTable[g_PlayerCarIndex];

    switch (g_PaintColorCursor) {
    case 0:
        PlaySoundCue(2);
        GameMenuBusy = -1;
        g_UiScriptProgress2 = 0;
        g_PaintColorIndex = car->paintColor1;
        break;
    case 1:
        PlaySoundCue(2);
        GameMenuBusy = -2;
        g_UiScriptProgress2 = 0;
        g_PaintColorIndex = car->paintColor2;
        break;
    case 2:
        LeavePaintColorScreen(1);
        break;
    }
}

static void UpdatePaintColorIdle(void) {
    DrawPaintColorPalette(&g_UiScriptProgress2, -1, g_PaintColorIndex);
    DrawBrowseArrows(-1, 0, 1, 1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
    RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, 0);
    if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) == 0 ||
        g_UiScriptProgress2 > 0) {
        return;
    }

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_PaintColorCursor = g_PaintColorCursor > 0 ? g_PaintColorCursor - 1 : 2;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_PaintColorCursor = g_PaintColorCursor < 2 ? g_PaintColorCursor + 1 : 0;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChoosePaintColorRow();
    } else if (g_PadPressed & PAD_CANCEL) {
        LeavePaintColorScreen(3);
    }
}

static void UpdateSelectedPaintColor(void) {
    CarEntry *car = &g_CarTable[g_PlayerCarIndex];
    CarEntry *timeAttackCar = &g_TimeAttackCars[g_PlayerCarIndex];
    MenuDialogAction action;
    int editsPrimary = GameMenuBusy == -1;

    if (DrawPaintColorPalette(&g_UiScriptProgress2, 1, g_PaintColorIndex) != 0) {
        action = ChooseMenuDialogAction(g_PadPressed);
        if (action == MENU_DIALOG_CONFIRM) {
            PlaySoundCue(2);
            if (editsPrimary) {
                car->paintColor1 = (u8)g_PaintColorIndex;
            } else {
                car->paintColor2 = (u8)g_PaintColorIndex;
            }
            timeAttackCar->paintColor1 = car->paintColor1;
            timeAttackCar->paintColor2 = car->paintColor2;
            GameMenuBusy = 0;
        } else if (action == MENU_DIALOG_CANCEL) {
            PlaySoundCue(3);
            g_PaintColorIndex =
                editsPrimary ? car->paintColor1 : car->paintColor2;
            GameMenuBusy = 0;
        } else if (g_PadPressedRepeat & PAD_LEFT) {
            PlaySoundCue(1);
            g_PaintColorIndex = g_PaintColorIndex > 0 ? g_PaintColorIndex - 1 : 17;
        } else if (g_PadPressedRepeat & PAD_RIGHT) {
            PlaySoundCue(1);
            g_PaintColorIndex = g_PaintColorIndex < 17 ? g_PaintColorIndex + 1 : 0;
        }
        if (editsPrimary) {
            SetPrimaryBodyColor(g_PaintColorIndex);
        } else {
            SetSecondaryBodyColor(g_PaintColorIndex);
        }
    }

    DrawBrowseArrows(1, 0, 1, 1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
    RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void UpdatePaintColorOutgoing(void) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = 10;
    RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
    if (g_UiScriptProgress <= 0) {
        g_MenuScreen = 6;
        g_MenuHandlerIndex = 6;
        g_PaintColorCursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}

void UpdatePaintColorScreen(void) {
    s32 state = GameMenuBusy;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuCarView();
    if (state == 0) {
        UpdatePaintColorIdle();
    } else if (state < 0) {
        UpdateSelectedPaintColor();
    } else {
        UpdatePaintColorOutgoing();
    }
}
