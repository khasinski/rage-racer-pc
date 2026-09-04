#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"

enum DesignModeState {
    DESIGN_MODE_DENIED = -1,
    DESIGN_MODE_IDLE = 0,
    DESIGN_MODE_EXIT_TO_LOGO = 1,
    DESIGN_MODE_EXIT_TO_NAME = 2,
    DESIGN_MODE_EXIT_TO_PAINT = 3,
    DESIGN_MODE_EXIT_TO_CUSTOMIZE = 4,
};

enum DesignModeOption {
    DESIGN_MODE_OPTION_LOGO,
    DESIGN_MODE_OPTION_NAME,
    DESIGN_MODE_OPTION_PAINT,
    DESIGN_MODE_OPTION_EXIT,
    DESIGN_MODE_OPTION_COUNT,
};

enum {
    DESIGN_MODE_GRID_SIZE = 6,
};

s32 DrawDesignModeScreen(s32 step) {
    const DesignModeCellMask *mask = &g_DesignModeCellMask;
    GameOrderingTableEntry *ot;
    s32 remainingFade;
    s32 offset = 0;
    s32 fade;
    s32 intensity;
    s32 y;
    s32 row;
    s32 column;

    fade = AdvanceMenuFade(&g_DesignModeScreenFade, step);
    if (step == 0 || RENDER_OT_BASE == NULL) {
        return fade;
    }
    ot = RENDER_OT_BASE + 1;

    if (step < 0) {
        remainingFade = MENU_FADE_MAX - fade;
        offset = remainingFade * remainingFade / 2048;
    }

    y = 0xB0 - offset;
    intensity = fade / MENU_FADE_INTENSITY_DIVISOR;

    DrawSprite(ot, 0xB4, y, 0x18, 0xC, 0x94, 0xDC, (u8)intensity,
               (u8)intensity, (u8)intensity, 0x244, 0, 1, 0x3B);
    DrawSprite(ot, 0xCE, y, 0x14, 0xC, 0xE0, 0xDC, (u8)intensity,
               (u8)intensity, (u8)intensity, 0x244, 0, 1, 0x3B);

    for (row = 0; row < DESIGN_MODE_GRID_SIZE; row++) {
        for (column = 0; column < DESIGN_MODE_GRID_SIZE; column++) {
            s32 clutX;

            if (mask->cells[row][column] != 0) {
                clutX = 0x26F;
            } else {
                clutX = 0x244;
            }

            DrawSprite(ot, 0xB4 + column * 0x10,
                       0xC0 + row * 0x20 - offset, 0xC, 0x18, 0xF4,
                       0x60, (u8)intensity, (u8)intensity, (u8)intensity,
                       clutX, 0, 1, 0x39);
        }
    }

    return g_DesignModeScreenFade;
}

static void ExitDesignMode(void) {
    PlaySoundCue(3);
    GameMenuBusy = DESIGN_MODE_EXIT_TO_CUSTOMIZE;
    g_MenuOverlayPattern = 2;
}

static void HandleDesignModeInput(void) {
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_DesignModeOption = g_DesignModeOption > 0
                                 ? g_DesignModeOption - 1
                                 : DESIGN_MODE_OPTION_COUNT - 1;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_DesignModeOption = g_DesignModeOption < DESIGN_MODE_OPTION_COUNT - 1
                                 ? g_DesignModeOption + 1
                                 : DESIGN_MODE_OPTION_LOGO;
    }

    if (!(g_PadPressed & PAD_CONFIRM)) {
        if (g_PadPressed & PAD_CANCEL) ExitDesignMode();
        return;
    }

    switch (g_DesignModeOption) {
    case DESIGN_MODE_OPTION_LOGO:
        PlaySoundCue(2);
        RampTeamLogoCanvas(-256, -256);
        GameMenuBusy = DESIGN_MODE_EXIT_TO_LOGO;
        g_MenuOverlayPattern = 1;
        break;
    case DESIGN_MODE_OPTION_NAME:
        PlaySoundCue(2);
        GameMenuBusy = DESIGN_MODE_EXIT_TO_NAME;
        g_MenuOverlayPattern = DESIGN_MODE_OPTION_NAME;
        break;
    case DESIGN_MODE_OPTION_PAINT:
        if ((u32)g_PlayerCarIndex < CUSTOM_PAINT_CAR_COUNT) {
            GameMenuBusy = DESIGN_MODE_EXIT_TO_PAINT;
            g_MenuOverlayPattern = 1;
            PlaySoundCue(2);
        } else {
            GameMenuBusy = DESIGN_MODE_DENIED;
            g_UiScriptProgress2 = 0;
            PlaySoundCue(5);
        }
        break;
    case DESIGN_MODE_OPTION_EXIT:
        ExitDesignMode();
        break;
    }
}

static void UpdateDesignModeIdle(void) {
    RunTimedDrawScript(g_DesignModeDeniedScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, DESIGN_MODE_OPTION_COUNT - 1,
                          g_DesignModeOption);
    RunTimedDrawScript(g_DesignModeScript, &g_UiScriptProgress, 0);
    if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) {
        g_MenuOverlayPattern = -1;
        HandleDesignModeInput();
    }
}

static void UpdateDesignModeDenied(void) {
    RunTimedDrawScript(g_DesignModeDeniedScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0 &&
        (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL))) {
        GameMenuBusy = DESIGN_MODE_IDLE;
    }
    DrawFadingMenuSprites(g_UiScriptProgress, DESIGN_MODE_OPTION_COUNT - 1,
                          g_DesignModeOption);
    RunTimedDrawScript(g_DesignModeScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void FinishDesignModeExit(void) {
    switch (GameMenuBusy) {
    case DESIGN_MODE_EXIT_TO_LOGO:
        g_MenuScreen = MENU_SCREEN_TEAM_LOGO;
        g_MenuHandlerIndex = MENU_SCREEN_TEAM_LOGO;
        DrawTeamLogoCanvas(0, 0);
        break;
    case DESIGN_MODE_EXIT_TO_NAME:
        g_MenuScreen = MENU_SCREEN_TEAM_NAME;
        g_MenuHandlerIndex = MENU_SCREEN_TEAM_NAME;
        DrawTeamNameEntry(0, 0);
        g_MenuViewOffset = MENU_VIEW_OFFSET_MAX;
        g_MenuViewOffsetTarget = 0;
        g_MenuViewAngleTarget = 0;
        g_MenuViewAngle = 0;
        GameMenuCursor = g_TeamNameLength >= MENU_TEAM_NAME_MAX_LENGTH
                             ? TEAM_NAME_KEY_END
                             : 0;
        g_TeamNameCharModel = GameMenuCursor;
        break;
    case DESIGN_MODE_EXIT_TO_PAINT:
        g_MenuScreen = MENU_SCREEN_PAINT_COLOR;
        g_MenuHandlerIndex = MENU_SCREEN_PAINT_COLOR;
        g_UiScriptProgress2 = 0;
        g_MenuViewOffset = MENU_VIEW_OFFSET_MAX;
        g_MenuViewOffsetTarget = 0;
        break;
    case DESIGN_MODE_EXIT_TO_CUSTOMIZE:
        g_MenuScreen = MENU_SCREEN_CUSTOMIZE;
        g_MenuHandlerIndex = MENU_SCREEN_CUSTOMIZE;
        g_DesignModeOption = DESIGN_MODE_OPTION_LOGO;
        g_MenuViewOffset = MENU_VIEW_OFFSET_MAX;
        g_MenuViewOffsetTarget = 0;
        break;
    }
    g_UiScriptProgress = 0;
    GameMenuBusy = DESIGN_MODE_IDLE;
}

static void UpdateDesignModeExit(void) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_DESIGN_MODE;
    RunTimedDrawScript(g_DesignModeScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, DESIGN_MODE_OPTION_COUNT - 1,
                          g_DesignModeOption);
    if (g_UiScriptProgress <= 0) FinishDesignModeExit();
}

void UpdateDesignModeScreen(void) {
    g_DesignModeOption = AddClampedMenuValue(
        g_DesignModeOption, 0, DESIGN_MODE_OPTION_LOGO,
        DESIGN_MODE_OPTION_COUNT - 1);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuCarView();

    if (GameMenuBusy == DESIGN_MODE_IDLE) {
        UpdateDesignModeIdle();
    } else if (GameMenuBusy < DESIGN_MODE_IDLE) {
        UpdateDesignModeDenied();
    } else {
        UpdateDesignModeExit();
    }
}
