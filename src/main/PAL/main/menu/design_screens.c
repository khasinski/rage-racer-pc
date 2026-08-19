#include "common.h"
#include "game/game_input.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/design_controller.h"
#include "game/menu.h"
#include "game/menu_dialog_controller.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/team_name_controller.h"

typedef enum LogoSampleState {
    LOGO_SAMPLE_EDIT_BACKGROUND = -2,
    LOGO_SAMPLE_EDIT_CHARACTER = -1,
    LOGO_SAMPLE_ACTIVE = 0,
    LOGO_SAMPLE_BACK = 1
} LogoSampleState;

void UpdateLogoSampleScreen(void) {
    s32 v0;
    s32 t;

    g_MenuAltLayout = 0;
    ComposeSampleTeamLogo(g_LogoSampleCharIndex, g_LogoSampleBackIndex);
    DrawTeamLogoCanvas(1, 0);
    v0 = GameMenuBusy;
    if (v0 == LOGO_SAMPLE_ACTIVE) {
        RampTeamLogoCanvas(-10, 0);
        DrawLogoSamplePanel(-1, g_LogoSampleSavedIndex + 1);
        RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2, -1);
        DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
        RunTimedDrawScript(&g_LogoSampleScreenScript, &g_UiScriptProgress, 0);
        if (RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) == 0) return;
        if (g_UiScriptProgress2 > 0) return;
        {
            DesignMenuInputResult input = DesignMenuHandleInput(
                g_LogoSampleCursor, g_GameInput.pressed);
            s32 soundIndex;

            g_MenuOverlayPattern = -1;
            g_LogoSampleCursor = input.selection;
            for (soundIndex = 0; soundIndex < input.moveCount; soundIndex++) {
                PlaySoundCue(1);
            }
            if (input.command == DESIGN_MENU_PRIMARY) {
                PlaySoundCue(2);
                GameMenuBusy = LOGO_SAMPLE_EDIT_CHARACTER;
                g_UiScriptProgress2 = 0;
                g_LogoSampleSubPanelScript = &g_MenuRow0MarkerScript;
                g_LogoSampleSavedIndex = g_LogoSampleCharIndex;
            } else if (input.command == DESIGN_MENU_SECONDARY) {
                PlaySoundCue(2);
                GameMenuBusy = LOGO_SAMPLE_EDIT_BACKGROUND;
                g_UiScriptProgress2 = 0;
                g_LogoSampleSubPanelScript = &g_MenuRow1MarkerScript;
                g_LogoSampleSavedIndex = g_LogoSampleBackIndex;
            } else if (input.command == DESIGN_MENU_BACK ||
                       input.command == DESIGN_MENU_CANCEL) {
                PlaySoundCue(3);
                GameMenuBusy = LOGO_SAMPLE_BACK;
                g_MenuOverlayPattern = 2;
            }
        }
        return;
    }

    if (v0 < 0) {
        MenuDialogInputResult dialog;
        s32 soundIndex;

        RampTeamLogoCanvas(10, 0);
        if (GameMenuBusy == LOGO_SAMPLE_EDIT_CHARACTER) {
            if (RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2, 1) != 0) {
                dialog = MenuDialogHandleRange(
                    g_LogoSampleCharIndex, 0, 19, -1, 1,
                    0, g_GameInput.pressed);
                if (dialog.confirmed) {
                    PlaySoundCue(2);
                    GameMenuBusy = LOGO_SAMPLE_ACTIVE;
                    g_LogoSampleSavedIndex = g_LogoSampleCharIndex;
                }
                if (dialog.cancelled) {
                    PlaySoundCue(3);
                    GameMenuBusy = LOGO_SAMPLE_ACTIVE;
                    g_LogoSampleCharIndex = g_LogoSampleSavedIndex;
                }
                dialog = MenuDialogHandleRange(
                    g_LogoSampleCharIndex, 0, 19, -1, 1,
                    g_GameInput.pressed, 0);
                g_LogoSampleCharIndex = dialog.value;
                for (soundIndex = 0;
                     soundIndex < dialog.moveCount; soundIndex++) {
                    PlaySoundCue(1);
                }
            }
            t = g_LogoSampleCharIndex;
        } else {
            if (RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2, 1) != 0) {
                dialog = MenuDialogHandleRange(
                    g_LogoSampleBackIndex, 0, 19, -1, 1,
                    0, g_GameInput.pressed);
                if (dialog.confirmed) {
                    PlaySoundCue(2);
                    GameMenuBusy = LOGO_SAMPLE_ACTIVE;
                    g_LogoSampleSavedIndex = g_LogoSampleBackIndex;
                }
                if (dialog.cancelled) {
                    PlaySoundCue(3);
                    GameMenuBusy = LOGO_SAMPLE_ACTIVE;
                    g_LogoSampleBackIndex = g_LogoSampleSavedIndex;
                }
                dialog = MenuDialogHandleRange(
                    g_LogoSampleBackIndex, 0, 19, -1, 1,
                    g_GameInput.pressed, 0);
                g_LogoSampleBackIndex = dialog.value;
                for (soundIndex = 0;
                     soundIndex < dialog.moveCount; soundIndex++) {
                    PlaySoundCue(1);
                }
            }
            t = g_LogoSampleBackIndex;
        }
        DrawLogoSamplePanel(1, t + 1);
        DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
        RunTimedDrawScript(&g_LogoSampleScreenScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    MenuFlowFadeOut(MENU_SCREEN_LOGO_SAMPLE);
    DrawLogoSamplePanel(-1, 0);
    RunTimedDrawScript(&g_LogoSampleScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
    if (g_UiScriptProgress <= 0) {
        MenuFlowOpen(MENU_SCREEN_TEAM_LOGO);
        g_LogoSampleCursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = LOGO_SAMPLE_ACTIVE;
    }
}

s32 DrawTeamNameScreen(s32 step) {
    s32 value;

    if (step == 0) {
        g_TeamNameScreenProgress = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_TeamNameScreenProgress;
        g_TeamNameScreenProgress = value;
        if (value >= 0x1FD) {
            g_TeamNameScreenProgress = 0x1FC;
        }
    } else {
        value = step + g_TeamNameScreenProgress;
        g_TeamNameScreenProgress = value;
        if (value < 0) {
            g_TeamNameScreenProgress = 0;
        }
    }

    return g_TeamNameScreenProgress;
}


typedef enum TeamNameScreenState {
    TEAM_NAME_ACTIVE = 0,
    TEAM_NAME_BACK = 1
} TeamNameScreenState;

void UpdateTeamNameScreen(void) {
    TeamNameInputResult input;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawTeamNameCharModel();
    if (GameMenuBusy == TEAM_NAME_ACTIVE) {

    DrawTeamNameEntry(1, GameMenuCursor);
    if (RunTimedDrawScript(&g_TeamNameScreenScript, &g_UiScriptProgress, 1) == 0) return;
    g_MenuOverlayPattern = -1;

    input = TeamNameHandleInput(
        GameMenuCursor, g_TeamNameLength, GameMenuCursorAnim,
        g_GameInput.pressedRepeat, g_GameInput.pressed);
    GameMenuCursor = input.cursor;
    if (input.moved) {
        g_MenuViewAngleTarget = 0;
        g_MenuViewAngle = 0x3E8000;
        GameMenuCursorAnim = GameMenuCursor;
        PlaySoundCue(1);
    }
    if (input.command == TEAM_NAME_COMMAND_BACK) {
        PlaySoundCue(3);
        GameMenuBusy = TEAM_NAME_BACK;
        g_MenuOverlayPattern = 2;
        g_MenuViewOffsetTarget = 0x3D090;
        return;
    }
    if (input.command == TEAM_NAME_COMMAND_APPEND) {
        PlaySoundCue(2);
        g_TeamNameChars[g_TeamNameLength] = (u8)GameMenuCursor;
        if (g_TeamNameLength >= 5) GameMenuCursor = 0x2B;
        if (g_TeamNameLength < 7) g_TeamNameLength++;
    } else if (input.command == TEAM_NAME_COMMAND_DELETE) {
        if (g_TeamNameLength == 0) return;
        PlaySoundCue(4);
        g_TeamNameChars[g_TeamNameLength] = 0xA;
        g_TeamNameLength--;
    }
    return;
    }

    MenuFlowFadeOut(MENU_SCREEN_TEAM_NAME);
    DrawTeamNameEntry(-1, GameMenuCursor);
    RunTimedDrawScript(&g_TeamNameScreenScript, &g_UiScriptProgress, -1);
    if (g_UiScriptProgress > 0) return;
    if (0x3D08F < g_MenuViewOffset) {
        MenuFlowOpen(MENU_SCREEN_DESIGN_MODE);
        UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
        g_UiScriptProgress = 0;
        GameMenuBusy = TEAM_NAME_ACTIVE;
    }
}

s32 DrawPaintColorScreen(s32 step) {
    s32 value;

    if (step == 0) {
        g_PaintColorScreenProgress = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_PaintColorScreenProgress;
        g_PaintColorScreenProgress = value;
        if (value >= 0x1FD) {
            g_PaintColorScreenProgress = 0x1FC;
        }
    } else {
        value = step + g_PaintColorScreenProgress;
        g_PaintColorScreenProgress = value;
        if (value < 0) {
            g_PaintColorScreenProgress = 0;
        }
    }

    return g_PaintColorScreenProgress;
}


typedef enum PaintColorState {
    PAINT_COLOR_SECONDARY = -2,
    PAINT_COLOR_PRIMARY = -1,
    PAINT_COLOR_ACTIVE = 0,
    PAINT_COLOR_BACK = 1,
    PAINT_COLOR_CANCEL = 3
} PaintColorState;

void UpdatePaintColorScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuCarView();

    if (GameMenuBusy == PAINT_COLOR_ACTIVE) {
        DrawPaintColorPalette(&g_UiScriptProgress2, -1, g_PaintColorIndex);
        DrawBrowseArrows(-1, 0, 1, 1);
        DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
        RunTimedDrawScript(&g_PaintColorScreenScript, &g_UiScriptProgress, 0);
        if (RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) == 0) {
            return;
        }
        if (g_UiScriptProgress2 > 0) {
            return;
        }
        {
            DesignMenuInputResult input = DesignMenuHandleInput(
                g_PaintColorCursor, g_GameInput.pressed);
            s32 soundIndex;

            g_MenuOverlayPattern = -1;
            g_PaintColorCursor = input.selection;
            for (soundIndex = 0; soundIndex < input.moveCount; soundIndex++) {
                PlaySoundCue(1);
            }
            if (input.command == DESIGN_MENU_PRIMARY) {
                PlaySoundCue(2);
                GameMenuBusy = PAINT_COLOR_PRIMARY;
                g_UiScriptProgress2 = 0;
                g_PaintColorIndex =
                    g_CarTable[g_PlayerCarIndex].paintColor1;
            } else if (input.command == DESIGN_MENU_SECONDARY) {
                PlaySoundCue(2);
                GameMenuBusy = PAINT_COLOR_SECONDARY;
                g_UiScriptProgress2 = 0;
                g_PaintColorIndex =
                    g_CarTable[g_PlayerCarIndex].paintColor2;
            } else if (input.command == DESIGN_MENU_BACK ||
                       input.command == DESIGN_MENU_CANCEL) {
                PlaySoundCue(3);
                GameMenuBusy = input.command == DESIGN_MENU_BACK
                    ? PAINT_COLOR_BACK : PAINT_COLOR_CANCEL;
                g_MenuOverlayPattern = 2;
                g_MenuViewOffsetTarget = 0x3D090;
            }
        }
        return;
    }

    if (GameMenuBusy < 0) {
        if (DrawPaintColorPalette(&g_UiScriptProgress2, 1, g_PaintColorIndex) != 0) {
            MenuDialogInputResult dialog = MenuDialogHandleRange(
                g_PaintColorIndex, 0, 17, -1, 1,
                g_GameInput.pressedRepeat, g_GameInput.pressed);
            s32 soundIndex;

            g_PaintColorIndex = dialog.value;
            for (soundIndex = 0;
                 soundIndex < dialog.moveCount; soundIndex++) {
                PlaySoundCue(1);
            }
            if (GameMenuBusy == PAINT_COLOR_PRIMARY) {
                if (dialog.confirmed) {
                    PlaySoundCue(2);
                    g_CarTable[g_PlayerCarIndex].paintColor1 = g_PaintColorIndex;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor1 = g_PaintColorIndex;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor2 = g_CarTable[g_PlayerCarIndex].paintColor2;
                    GameMenuBusy = PAINT_COLOR_ACTIVE;
                }
                if (dialog.cancelled) {
                    PlaySoundCue(3);
                    g_PaintColorIndex = g_CarTable[g_PlayerCarIndex].paintColor1;
                    GameMenuBusy = PAINT_COLOR_ACTIVE;
                }
                SetBodyColor1(g_PaintColorIndex);
            } else {
                if (dialog.confirmed) {
                    PlaySoundCue(2);
                    g_CarTable[g_PlayerCarIndex].paintColor2 = g_PaintColorIndex;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor1 = g_CarTable[g_PlayerCarIndex].paintColor1;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor2 = g_PaintColorIndex;
                    GameMenuBusy = PAINT_COLOR_ACTIVE;
                }
                if (dialog.cancelled) {
                    PlaySoundCue(3);
                    g_PaintColorIndex = g_CarTable[g_PlayerCarIndex].paintColor2;
                    GameMenuBusy = PAINT_COLOR_ACTIVE;
                }
                SetBodyColor2(g_PaintColorIndex);
            }
        }

        DrawBrowseArrows(1, 0, 1, 1);
        DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
        RunTimedDrawScript(&g_PaintColorScreenScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    MenuFlowFadeOut(MENU_SCREEN_PAINT_COLOR);
    RunTimedDrawScript(&g_PaintColorScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
    if (g_UiScriptProgress <= 0) {
        MenuFlowOpen(MENU_SCREEN_DESIGN_MODE);
        g_PaintColorCursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = PAINT_COLOR_ACTIVE;
    }
}

s32 DrawCarShopScreen(s32 step) {
    s32 value;
    s32 limit;
    s32 amount;
    s32 phase;

    if (step == 0) {
        g_CarShopScreenProgress = 0;
        return 0;
    }

    if (step > 0) {
        value = g_CarShopScreenProgress + step;
        g_CarShopScreenProgress = value;
        if (value >= 0x1FD) {
            g_CarShopScreenProgress = 0x1FC;
        }
        value = 0;
    } else {
        u32 product;

        value = g_CarShopScreenProgress + step;
        g_CarShopScreenProgress = value;
        if (value < 0) {
            g_CarShopScreenProgress = 0;
        }

        value = g_CarShopScreenProgress;
        limit = 0x1FC;
        limit -= value;
        product = limit * limit;
        value = product >> 0xB;
    }

    amount = value << 16;
    amount >>= 16;
    phase = (u8)(g_CarShopScreenProgress / 4U);
    DrawCarEngineSpec(amount, phase);

    return g_CarShopScreenProgress;
}
void UpdateCarListCursor(void) {
    s32 index;
    CarEntry *entry;

    if (g_CarShopUnlockAll != 0) {
        g_PrevOwnedCarIndex = -1;
        index = g_CarListCursor - 1;
        if (index >= 0) {
            entry = &g_CarTable[index];
            while (index >= 0) {
                if (entry->enabled == 0) {
                    g_PrevOwnedCarIndex = index;
                    break;
                }
                index--;
                entry--;
            }
        }
    } else {
        g_PrevOwnedCarIndex = -1;
        index = g_CarListCursor - 1;
        if (index >= 0) {
        backward_loop:
            {
                s32 value = GetCarUnlockLevel(index);
                if (g_CarTable[index].enabled == 0) {
                    s32 progression = g_RaceProgress->maxClassReached;
                    if (progression < 4) {
                        if ((progression + 1) < value) {
                            index--;
                            goto backward_check;
                        }
                        g_PrevOwnedCarIndex = index;
                        goto previous_car_done;
                    }
                    if (progression >= value) {
                        g_PrevOwnedCarIndex = index;
                        goto previous_car_done;
                    }
                }
                index--;
            }
        backward_check:
            if (index >= 0) {
                goto backward_loop;
            }
        }
    }

previous_car_done:
    if (g_CarShopUnlockAll != 0) {
        g_NextOwnedCarIndex = -1;
        index = g_CarListCursor + 1;
        if (index < 13) {
            entry = &g_CarTable[index];
            while (index < 13) {
                if (entry->enabled == 0) {
                    g_NextOwnedCarIndex = index;
                    break;
                }
                index++;
                entry++;
            }
        }
    } else {
        g_NextOwnedCarIndex = -1;
        index = g_CarListCursor + 1;
        if (index < 13) {
        forward_loop:
            {
                s32 value = GetCarUnlockLevel(index);
                if (g_CarTable[index].enabled == 0) {
                    s32 progression = g_RaceProgress->maxClassReached;
                    if (progression < 4) {
                        if ((progression + 1) < value) {
                            index++;
                            goto forward_check;
                        }
                        g_NextOwnedCarIndex = index;
                        return;
                    }
                    if (progression >= value) {
                        g_NextOwnedCarIndex = index;
                        return;
                    }
                }
                index++;
            }
forward_check:
            if (index < 13) {
                goto forward_loop;
            }
        }
    }

    return;
}
