#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"

enum PaintColorScreenState {
    PAINT_COLOR_IDLE = 0,
    PAINT_COLOR_EXIT = 1,
    PAINT_COLOR_CANCEL_EXIT = 3,
    PAINT_COLOR_EDIT_PRIMARY = -1,
    PAINT_COLOR_EDIT_SECONDARY = -2,
};

enum PaintColorOption {
    PAINT_COLOR_OPTION_PRIMARY,
    PAINT_COLOR_OPTION_SECONDARY,
    PAINT_COLOR_OPTION_EXIT,
    PAINT_COLOR_OPTION_COUNT,
};

enum {
    PAINT_COLOR_VIEW_OFFSET = 250000,
};

s32 DrawPaintColorScreen(s32 step) {
    return AdvanceMenuFade(&g_PaintColorScreenProgress, step);
}

static s32 PaintColorCarAvailable(void) {
    return g_CarTable != NULL &&
           (u32)g_PlayerCarIndex < CUSTOM_PAINT_CAR_COUNT;
}

static void LeavePaintColorScreen(s32 busyState) {
    PlaySoundCue(3);
    GameMenuBusy = busyState;
    g_MenuOverlayPattern = 2;
    g_MenuViewOffsetTarget = PAINT_COLOR_VIEW_OFFSET;
}

static void ChoosePaintColorRow(void) {
    CarEntry *car;

    switch (g_PaintColorCursor) {
    case PAINT_COLOR_OPTION_PRIMARY:
        if (!PaintColorCarAvailable()) return;
        car = &g_CarTable[g_PlayerCarIndex];
        PlaySoundCue(2);
        GameMenuBusy = PAINT_COLOR_EDIT_PRIMARY;
        g_UiScriptProgress2 = 0;
        g_PaintColorIndex = AddClampedMenuValue(
            car->paintColor1, 0, 0, MENU_PAINT_COLOR_COUNT - 1);
        break;
    case PAINT_COLOR_OPTION_SECONDARY:
        if (!PaintColorCarAvailable()) return;
        car = &g_CarTable[g_PlayerCarIndex];
        PlaySoundCue(2);
        GameMenuBusy = PAINT_COLOR_EDIT_SECONDARY;
        g_UiScriptProgress2 = 0;
        g_PaintColorIndex = AddClampedMenuValue(
            car->paintColor2, 0, 0, MENU_PAINT_COLOR_COUNT - 1);
        break;
    case PAINT_COLOR_OPTION_EXIT:
        LeavePaintColorScreen(PAINT_COLOR_EXIT);
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
        g_PaintColorCursor = WrapMenuIndex(g_PaintColorCursor, -1,
                                           PAINT_COLOR_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_PaintColorCursor = WrapMenuIndex(g_PaintColorCursor, 1,
                                           PAINT_COLOR_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChoosePaintColorRow();
    } else if (g_PadPressed & PAD_CANCEL) {
        LeavePaintColorScreen(PAINT_COLOR_CANCEL_EXIT);
    }
}

static void UpdateSelectedPaintColor(s32 state) {
    CarEntry *car = &g_CarTable[g_PlayerCarIndex];
    CarEntry *timeAttackCar = &g_TimeAttackCars[g_PlayerCarIndex];
    MenuDialogAction action;
    int editsPrimary = state == PAINT_COLOR_EDIT_PRIMARY;

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
            g_PaintColorIndex = WrapMenuIndex(
                g_PaintColorIndex, -1, MENU_PAINT_COLOR_COUNT);
        } else if (g_PadPressedRepeat & PAD_RIGHT) {
            PlaySoundCue(1);
            g_PaintColorIndex = WrapMenuIndex(
                g_PaintColorIndex, 1, MENU_PAINT_COLOR_COUNT);
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
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_PAINT_COLOR;
    RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
    if (g_UiScriptProgress <= 0) {
        g_MenuScreen = MENU_SCREEN_DESIGN_MODE;
        g_MenuHandlerIndex = MENU_SCREEN_DESIGN_MODE;
        g_PaintColorCursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}

void UpdatePaintColorScreen(void) {
    s32 state = GameMenuBusy;

    g_PaintColorCursor = AddClampedMenuValue(
        g_PaintColorCursor, 0, 0, PAINT_COLOR_OPTION_COUNT - 1);
    g_PaintColorIndex = AddClampedMenuValue(
        g_PaintColorIndex, 0, 0, MENU_PAINT_COLOR_COUNT - 1);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuCarView();
    if (state == PAINT_COLOR_IDLE) {
        UpdatePaintColorIdle();
    } else if (state == PAINT_COLOR_EDIT_PRIMARY ||
               state == PAINT_COLOR_EDIT_SECONDARY) {
        if (!PaintColorCarAvailable()) {
            GameMenuBusy = PAINT_COLOR_IDLE;
            return;
        }
        UpdateSelectedPaintColor(state);
    } else if (state > PAINT_COLOR_IDLE) {
        UpdatePaintColorOutgoing();
    } else {
        GameMenuBusy = PAINT_COLOR_IDLE;
    }
}
