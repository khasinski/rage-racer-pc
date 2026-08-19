#include "common.h"
#include "game/game_input.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_controller.h"
#include "game/menu_internal.h"
#include "game/render.h"
#include "game/state.h"

typedef enum DesignModeState {
    DESIGN_MODE_PAINT_DENIED = -1,
    DESIGN_MODE_ACTIVE = 0,
    DESIGN_MODE_TO_TEAM_LOGO = 1,
    DESIGN_MODE_TO_TEAM_NAME = 2,
    DESIGN_MODE_TO_PAINT = 3,
    DESIGN_MODE_BACK = 4
} DesignModeState;

s32 DrawDesignModeScreen(s32 step) {
    DesignModeCellMask mask;
    void *ot;
    u32 limit;
    u32 offset;
    s32 intensity;
    s32 y;
    s32 row;
    s32 column;
    u32 fadeValue;

    ot = RENDER_OT_BASE_AS(OT_TYPE) + 1;
    mask = g_DesignModeCellMask;

    if (step == 0) {
        g_DesignModeScreenFade = 0;
        return 0;
    }

    if (step > 0) {
        s32 updated;

        updated = g_DesignModeScreenFade + step;
        g_DesignModeScreenFade = updated;
        if (updated >= MENU_FADE_COMPLETE) {
            g_DesignModeScreenFade = MENU_FADE_MAX;
        }
        offset = 0;
    } else {
        s32 updated;

        updated = g_DesignModeScreenFade + step;
        g_DesignModeScreenFade = updated;
        if (updated < 0) {
            g_DesignModeScreenFade = 0;
        }
        limit = MENU_FADE_MAX - g_DesignModeScreenFade;
        offset = limit * limit / 2048;
    }

    y = 0xB0 - (s16)offset;
    fadeValue = g_DesignModeScreenFade;
    intensity = fadeValue / MENU_FADE_INTENSITY_DIVISOR;

    DrawSprite(ot, 0xB4, y, 0x18, 0xC, 0x94, 0xDC,
                  (u8)intensity, (u8)intensity, (u8)intensity,
                  0x244, 0, 1, 0x3B);
    DrawSprite(ot, 0xCE, y, 0x14, 0xC, 0xE0, 0xDC,
                  (u8)intensity, (u8)intensity, (u8)intensity,
                  0x244, 0, 1, 0x3B);

    for (row = 0; row < 6; row++) {
        for (column = 0; column < 6; column++) {
            s32 clutX;

            if (mask.cells[row][column] != 0) {
                clutX = 0x26F;
            } else {
                clutX = 0x244;
            }

            DrawSprite(ot, 0xB4 + column * 0x10,
                          0xC0 + row * 0x20 - (s16)offset,
                          0xC, 0x18, 0xF4, 0x60,
                          (u8)intensity, (u8)intensity,
                          (u8)intensity,
                          clutX, 0, 1, 0x39);
        }
    }

    return g_DesignModeScreenFade;
}


void UpdateDesignModeScreen(void) {
    s32 sel;
    s32 direction;
    u16 edge;
    MenuSession menu;
    MenuSessionCommands commands;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuCarView();
    if (GameMenuBusy == DESIGN_MODE_ACTIVE) {
        RunTimedDrawScript(&g_DesignModeDeniedScript, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        DrawFadingMenuSprites(g_UiScriptProgress, 3, g_DesignModeOption);
        RunTimedDrawScript(&g_DesignModeScript, &g_UiScriptProgress, 0);
        if (RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) != 0) {
            g_MenuOverlayPattern = -1;
            direction = ((g_GameInput.pressed & PAD_UP) != 0) -
                        ((g_GameInput.pressed & PAD_DOWN) != 0);
            menu = (MenuSession){g_DesignModeOption, 4, 0};
            commands = MenuSessionStep(
                &menu, -direction, g_GameInput.pressed);
            g_DesignModeOption = menu.selection;
            if (commands.moved) PlaySoundCue(1);
            if (commands.action == MENU_ACTION_CONFIRM) {
                sel = g_DesignModeOption;
                if (sel == 0) {
                    PlaySoundCue(2);
                    RampTeamLogoCanvas(-256, -256);
                    GameMenuBusy = DESIGN_MODE_TO_TEAM_LOGO;
                    g_MenuOverlayPattern = 1;
                } else if (sel == 1) {
                    PlaySoundCue(2);
                    GameMenuBusy = DESIGN_MODE_TO_TEAM_NAME;
                    g_MenuOverlayPattern = sel;
                } else if (sel == 2) {
                    if (g_PlayerCarIndex < 10) {
                        GameMenuBusy = DESIGN_MODE_TO_PAINT;
                        g_MenuOverlayPattern = 1;
                        PlaySoundCue(2);
                    } else {
                        GameMenuBusy = DESIGN_MODE_PAINT_DENIED;
                        g_UiScriptProgress2 = 0;
                        PlaySoundCue(5);
                    }
                } else if (sel == 3) {
                    PlaySoundCue(3);
                    GameMenuBusy = DESIGN_MODE_BACK;
                    g_MenuOverlayPattern = 2;
                }
            } else if (commands.action == MENU_ACTION_CANCEL) {
                PlaySoundCue(3);
                GameMenuBusy = DESIGN_MODE_BACK;
                g_MenuOverlayPattern = 2;
            }
        }
    } else if (GameMenuBusy < 0) {
        RunTimedDrawScript(&g_DesignModeDeniedScript, &g_UiScriptProgress2, 0);
        if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
            edge = g_GameInput.pressed;
            if (edge & PAD_CONFIRM) GameMenuBusy = DESIGN_MODE_ACTIVE;
            if (edge & PAD_CANCEL) GameMenuBusy = DESIGN_MODE_ACTIVE;
        }
        DrawFadingMenuSprites(g_UiScriptProgress, 3, g_DesignModeOption);
        RunTimedDrawScript(&g_DesignModeScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
    } else {
        MenuFlowFadeOut(MENU_SCREEN_DESIGN_MODE);
        RunTimedDrawScript(&g_DesignModeScript, &g_UiScriptProgress, -1);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
        DrawFadingMenuSprites(g_UiScriptProgress, 3, g_DesignModeOption);
        if (g_UiScriptProgress <= 0) {
            switch (GameMenuBusy) {
            case DESIGN_MODE_TO_TEAM_LOGO:
                MenuFlowOpen(MENU_SCREEN_TEAM_LOGO);
                DrawTeamLogoCanvas(0, 0);
                break;
            case DESIGN_MODE_TO_TEAM_NAME:
                MenuFlowOpen(MENU_SCREEN_TEAM_NAME);
                DrawTeamNameEntry(0, 0);
                g_MenuViewOffset = 0x3D090;
                g_MenuViewOffsetTarget = 0;
                g_MenuViewAngleTarget = 0;
                g_MenuViewAngle = 0;
                GameMenuCursor = (g_TeamNameLength >= MENU_TEAM_NAME_MAX_LENGTH) ? 0x2B : 0;
                g_TeamNameCharModel = GameMenuCursor;
                break;
            case DESIGN_MODE_TO_PAINT:
                MenuFlowOpen(MENU_SCREEN_PAINT_COLOR);
                g_UiScriptProgress2 = 0;
                g_MenuViewOffset = 0x3D090;
                g_MenuViewOffsetTarget = 0;
                break;
            case DESIGN_MODE_BACK:
                MenuFlowOpen(MENU_SCREEN_CUSTOMIZE);
                g_DesignModeOption = 0;
                g_MenuViewOffset = 0x3D090;
                g_MenuViewOffsetTarget = 0;
                break;
            }
            g_UiScriptProgress = 0;
            GameMenuBusy = DESIGN_MODE_ACTIVE;
        }
    }
}

s32 DrawTeamLogoScreen(s32 step) {
    s32 value;

    if (step == 0) {
        g_TeamLogoScreenFade = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_TeamLogoScreenFade;
        g_TeamLogoScreenFade = value;
        if (value >= MENU_FADE_COMPLETE) {
            g_TeamLogoScreenFade = MENU_FADE_MAX;
        }
    } else {
        value = step + g_TeamLogoScreenFade;
        g_TeamLogoScreenFade = value;
        if (value < 0) {
            g_TeamLogoScreenFade = 0;
        }
    }

    return g_TeamLogoScreenFade;
}
