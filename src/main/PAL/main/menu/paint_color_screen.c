#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"

typedef enum PaintColorScreenState {
    PAINT_COLOR_IDLE = 0,
    PAINT_COLOR_EXIT = 1,
    PAINT_COLOR_CANCEL_EXIT = 3,
    PAINT_COLOR_EDIT_PRIMARY = -1,
    PAINT_COLOR_EDIT_SECONDARY = -2,
} PaintColorScreenState;

enum PaintColorOption {
    PAINT_COLOR_OPTION_PRIMARY,
    PAINT_COLOR_OPTION_SECONDARY,
    PAINT_COLOR_OPTION_EXIT,
    PAINT_COLOR_OPTION_COUNT,
};

static s32 PaintColorCarAvailable(void) {
    return g_CarTable != NULL &&
           (u32)g_PlayerCarIndex < CUSTOM_PAINT_CAR_COUNT;
}

static void LeavePaintColorScreen(PaintColorScreenState state) {
    PlaySoundCue(3);
    GameMenuBusy = state;
    g_MenuOverlayPattern = 2;
    g_MenuViewOffsetTarget = MENU_VIEW_OFFSET_MAX;
}

static void ChoosePaintColorRow(PaintColor *paint) {
    CarEntry *car;

    switch (paint->cursor) {
    case PAINT_COLOR_OPTION_PRIMARY:
        if (!PaintColorCarAvailable()) return;
        car = &g_CarTable[g_PlayerCarIndex];
        PlaySoundCue(2);
        GameMenuBusy = PAINT_COLOR_EDIT_PRIMARY;
        g_UiScriptProgress2 = 0;
        paint->selected = AddClampedMenuValue(
            car->paintColor1, 0, 0, MENU_PAINT_COLOR_COUNT - 1);
        break;
    case PAINT_COLOR_OPTION_SECONDARY:
        if (!PaintColorCarAvailable()) return;
        car = &g_CarTable[g_PlayerCarIndex];
        PlaySoundCue(2);
        GameMenuBusy = PAINT_COLOR_EDIT_SECONDARY;
        g_UiScriptProgress2 = 0;
        paint->selected = AddClampedMenuValue(
            car->paintColor2, 0, 0, MENU_PAINT_COLOR_COUNT - 1);
        break;
    case PAINT_COLOR_OPTION_EXIT:
        LeavePaintColorScreen(PAINT_COLOR_EXIT);
        break;
    }
}

static void UpdatePaintColorIdle(PaintColor *paint) {
    DrawPaintColorPalette(paint, &g_UiScriptProgress2, -1);
    DrawBrowseArrows(MenuBrowseArrows(), -1, 0, 1, 1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, paint->cursor);
    RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, 0);
    if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) == 0 ||
        g_UiScriptProgress2 > 0) {
        return;
    }

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        paint->cursor = WrapMenuIndex(paint->cursor, -1,
                                           PAINT_COLOR_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        paint->cursor = WrapMenuIndex(paint->cursor, 1,
                                           PAINT_COLOR_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChoosePaintColorRow(paint);
    } else if (g_PadPressed & PAD_CANCEL) {
        LeavePaintColorScreen(PAINT_COLOR_CANCEL_EXIT);
    }
}

static void UpdateSelectedPaintColor(PaintColor *paint,
                                     PaintColorScreenState state) {
    CarEntry *car = &g_CarTable[g_PlayerCarIndex];
    CarEntry *timeAttackCar = &g_TimeAttackCars[g_PlayerCarIndex];
    MenuDialogAction action;
    int editsPrimary = state == PAINT_COLOR_EDIT_PRIMARY;

    if (DrawPaintColorPalette(paint, &g_UiScriptProgress2, 1) != 0) {
        action = ChooseMenuDialogAction(g_PadPressed);
        if (action == MENU_DIALOG_CONFIRM) {
            PlaySoundCue(2);
            if (editsPrimary) {
                car->paintColor1 = (u8)paint->selected;
            } else {
                car->paintColor2 = (u8)paint->selected;
            }
            timeAttackCar->paintColor1 = car->paintColor1;
            timeAttackCar->paintColor2 = car->paintColor2;
            GameMenuBusy = 0;
        } else if (action == MENU_DIALOG_CANCEL) {
            PlaySoundCue(3);
            paint->selected =
                editsPrimary ? car->paintColor1 : car->paintColor2;
            GameMenuBusy = 0;
        } else if (g_PadPressedRepeat & PAD_LEFT) {
            PlaySoundCue(1);
            paint->selected = WrapMenuIndex(
                paint->selected, -1, MENU_PAINT_COLOR_COUNT);
        } else if (g_PadPressedRepeat & PAD_RIGHT) {
            PlaySoundCue(1);
            paint->selected = WrapMenuIndex(
                paint->selected, 1, MENU_PAINT_COLOR_COUNT);
        }
        if (editsPrimary) {
            SetPrimaryBodyColor(paint->selected);
        } else {
            SetSecondaryBodyColor(paint->selected);
        }
    }

    DrawBrowseArrows(MenuBrowseArrows(), 1, 0, 1, 1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, paint->cursor);
    RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void UpdatePaintColorOutgoing(PaintColor *paint) {
    MenuBeginExit(MENU_SCREEN_PAINT_COLOR);
    RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, paint->cursor);
    if (g_UiScriptProgress <= 0) {
        MenuActivateScreen(MENU_SCREEN_DESIGN_MODE);
        paint->cursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}

void UpdatePaintColorScreen(void) {
    PaintColor *paint = MenuPaintColor();
    PaintColorScreenState state = (PaintColorScreenState)GameMenuBusy;

    paint->cursor = AddClampedMenuValue(
        paint->cursor, 0, 0, PAINT_COLOR_OPTION_COUNT - 1);
    paint->selected = AddClampedMenuValue(
        paint->selected, 0, 0, MENU_PAINT_COLOR_COUNT - 1);
    DrawMenuCarView();
    if (state == PAINT_COLOR_IDLE) {
        UpdatePaintColorIdle(paint);
    } else if (state == PAINT_COLOR_EDIT_PRIMARY ||
               state == PAINT_COLOR_EDIT_SECONDARY) {
        if (!PaintColorCarAvailable()) {
            GameMenuBusy = PAINT_COLOR_IDLE;
            return;
        }
        UpdateSelectedPaintColor(paint, state);
    } else if (state > PAINT_COLOR_IDLE) {
        UpdatePaintColorOutgoing(paint);
    } else {
        GameMenuBusy = PAINT_COLOR_IDLE;
    }
}
