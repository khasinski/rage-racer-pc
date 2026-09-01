#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/save_internal.h"
#include "game/race.h"

enum RankingScreenState {
    RANKING_MENU = -1,
    RANKING_MENU_CLOSING = -2,
    RANKING_TOTAL_TABLE = -3,
    RANKING_TOTAL_TABLE_CLOSING = -4,
    RANKING_LAP_TABLE = -5,
    RANKING_LAP_TABLE_CLOSING = -6
};

void UpdateRankingScreen(void) {
    s32 state;

    g_MenuAltLayout = 0;
    DrawMenuCourseView();
    DrawMenuLightBurst(-9);
    state = GameMenuBusy;
    if (state == 0) {
        g_UiScriptProgress2 = 0;
        GameMenuBusy = RANKING_MENU;
        DrawFadingMenuSprites(0, 2, g_RankingCursor);
        RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, 1);
        /*
         * Having just arrived, draw the frame and wait for the next one.
         * Falling through from here reaches the code that leaves the screen,
         * which ran on the very frame the screen opened and sent the player
         * straight back to the course select: the ranking could not be
         * entered at all.
         */
        RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }
    if (state < 0) {
        switch (state) {
        case RANKING_MENU:
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, 1) != 0) {
                g_MenuOverlayPattern = -1;
                if (g_PadPressed & PAD_UP) {
                    PlaySoundCue(1);
                    g_RankingCursor = (g_RankingCursor > 0) ? g_RankingCursor - 1 : 2;
                }
                if (g_PadPressed & PAD_DOWN) {
                    PlaySoundCue(1);
                    g_RankingCursor = (g_RankingCursor < 2) ? g_RankingCursor + 1 : 0;
                }
                if (g_PadPressed & PAD_CONFIRM) {
                    if (g_RankingCursor == 0) {
                        PlaySoundCue(2);
                        GameMenuBusy = RANKING_MENU_CLOSING;
                        g_RankingPendingState = RANKING_TOTAL_TABLE;
                    } else if (g_RankingCursor == 1) {
                        PlaySoundCue(2);
                        GameMenuBusy = RANKING_MENU_CLOSING;
                        g_RankingPendingState = RANKING_LAP_TABLE;
                    } else if (g_RankingCursor == 2) {
                        PlaySoundCue(3);
                        GameMenuBusy = 1;
                        g_MenuOverlayPattern = 2;
                    }
                } else if (g_PadPressed & PAD_CANCEL) {
                    PlaySoundCue(3);
                    GameMenuBusy = 1;
                    g_MenuOverlayPattern = 2;
                }
            }
            break;
        case RANKING_MENU_CLOSING:
            RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = g_RankingPendingState;
            break;
        case RANKING_TOTAL_TABLE:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 0) == 0) {
                break;
            }
            if (!(g_PadPressed & (PAD_CONFIRM | PAD_CANCEL))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = RANKING_TOTAL_TABLE_CLOSING;
            break;
        case RANKING_TOTAL_TABLE_CLOSING:
            DrawRankingTable(&g_UiScriptProgress2, -1, 0);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = RANKING_MENU;
            break;
        case RANKING_LAP_TABLE:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 1) == 0) {
                break;
            }
            if (!(g_PadPressed & (PAD_CONFIRM | PAD_CANCEL))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = RANKING_LAP_TABLE_CLOSING;
            break;
        case RANKING_LAP_TABLE_CLOSING:
            DrawRankingTable(&g_UiScriptProgress2, -1, 1);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = RANKING_MENU;
            break;
        }
        RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }
    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 2;
    RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    if (g_UiScriptProgress > 0) {
        return;
    }
    g_MenuScreen = 1;
    g_MenuHandlerIndex = 1;
    g_RankingCursor = 0;
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
    DrawTimeAttackPlate(0);
    if (g_CourseIndex >= 4) {
        g_TimeAttackPlateStep = 1;
    } else {
        g_TimeAttackPlateStep = -1;
    }
}

void UpdateOwnedCarNeighbours(void) {
    s32 index;

    g_PrevOwnedCarIndex = -1;
    for (index = g_PlayerCarIndex - 1; index >= 0; index--) {
        if (g_CarTable[index].enabled == 1) {
            g_PrevOwnedCarIndex = index;
            break;
        }
    }

    g_NextOwnedCarIndex = -1;
    for (index = g_PlayerCarIndex + 1; index < GAME_CAR_COUNT; index++) {
        if (g_CarTable[index].enabled == 1) {
            g_NextOwnedCarIndex = index;
            break;
        }
    }
}

void EnterCarSelectScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    InstallCarModelSlot();
    g_MenuScreen = 4;
    g_UiScriptProgress = 0;
    UpdateOwnedCarNeighbours();
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    DrawMenuLightBurst(-9);
}


void UpdateCustomizeScreen(void) {
    void *ot;
    s32 mode;
    s32 lowMode;
    const TimedDrawCommand *cmdList;
    u16 *pad;
    s32 sel;

    ot = RENDER_OT_BASE_AS(void);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    mode = g_GrandPrixMode != 0 ? 3 : 2;
    cmdList = g_GrandPrixMode != 0 ? g_CustomizeMenuScriptGp
                                   : g_CustomizeMenuScriptTimeAttack;

    if (GameMenuBusy == 0) {
        g_CarSpecGraphStep = 3;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
        lowMode = mode & 0xFF;
        DrawFadingMenuSprites(g_UiScriptProgress, lowMode, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        if ((RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) && (g_UiScriptProgress2 <= 0)) {
            g_MenuOverlayPattern = -1;
            if (g_PadPressed & PAD_UP) {
                PlaySoundCue(1);
                g_RankingOption = (g_RankingOption > 0) ? g_RankingOption - 1 : lowMode;
            }
            if (g_PadPressed & PAD_DOWN) {
                PlaySoundCue(1);
                g_RankingOption = (g_RankingOption < mode) ? g_RankingOption + 1 : 0;
            }
            if (g_PadPressed & PAD_CONFIRM) {
                u8 carByte;

                sel = g_RankingOption;
                if (sel == 0) {
                    PlaySoundCue(2);
                    carByte = g_CarTable[g_PlayerCarIndex].tireCompound;
                    g_CustomizePopupScript = g_MenuDialogPanelUpperScript;
                    GameMenuBusy = -1;
                        g_UiScriptProgress2 = 0;
                        g_MenuSubCursor = carByte;
                        return;
                }
                if (sel == 1) {
                    if (g_CarModelAsset->transmissionAvailable != 0) {
                        PlaySoundCue(2);
                        carByte = g_CarTable[g_PlayerCarIndex].transmission;
                        g_CustomizePopupScript = g_MenuDialogPanelLowerScript;
                        GameMenuBusy = -2;
                        g_UiScriptProgress2 = 0;
                        g_MenuSubCursor = carByte;
                        return;
                    }
                    PlaySoundCue(5);
                    g_CustomizePopupScript = g_TransmissionUnavailableScript;
                    GameMenuBusy = -3;
                    g_UiScriptProgress2 = 0;
                    return;
                }
                if (sel == mode) {
                PlaySoundCue(3);
                GameMenuBusy = 2;
                g_MenuOverlayPattern = 2;
                                return;
                }
                if (sel == 2) {
                    PlaySoundCue(2);
                    GameMenuBusy = 1;
                    g_MenuOverlayPattern = 1;
                    g_CarSpecGraphStep = -3;
                    g_MenuViewOffsetTarget = 0x3D090;
                }
            } else if (g_PadPressed & PAD_CANCEL) {
                PlaySoundCue(3);
                GameMenuBusy = 2;
                g_MenuOverlayPattern = 2;
            }
        }
        return;
    }

    if (GameMenuBusy < 0) {
        if (GameMenuBusy == -1) {
            if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) != 0) {
                pad = &g_PadPressed;
                if (*pad & 0x860) {
                    PlaySoundCue(2);
                    GameMenuBusy = -5;
                    g_MenuConfirmTimer = 0x23;
                }
                if (*pad & 0x90) {
                    PlaySoundCue(3);
                    GameMenuBusy = 0;
                }
                if ((*pad & 0x8000) && (g_MenuSubCursor < 4)) {
                    PlaySoundCue(1);
                    g_MenuSubCursor++;
                }
                if (g_PadPressed & PAD_RIGHT) {
                    if (g_MenuSubCursor != 0) {
                        PlaySoundCue(1);
                        g_MenuSubCursor--;
                    }
                }
                DrawTireCompoundSlider(g_MenuSubCursor, 0);
            }
        } else if (GameMenuBusy == -2) {
            if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) != 0) {
                pad = &g_PadPressed;
                if (*pad & 0x860) {
                    PlaySoundCue(2);
                    GameMenuBusy = -6;
                    g_MenuConfirmTimer = 0x23;
                    g_CarTable[g_PlayerCarIndex].transmission = g_MenuSubCursor;
                    g_TimeAttackCarTransmissions[g_PlayerCarIndex * 8] = g_MenuSubCursor;
                }
                if (*pad & 0x90) {
                    PlaySoundCue(3);
                    GameMenuBusy = 0;
                }
                if ((*pad & 0x8000) && (g_MenuSubCursor != 0)) {
                    PlaySoundCue(1);
                    g_MenuSubCursor = 0;
                }
                if (g_PadPressed & PAD_RIGHT) {
                    if (g_MenuSubCursor == 0) {
                        PlaySoundCue(1);
                        g_MenuSubCursor = 1;
                    }
                }
                DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xDA : 0xB8, 0x68, 0x20, 0x20, 0);
                DrawSprite(ot, 0xC2, 0x70, 0xC, 0x10, 0x60, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                DrawSprite(ot, 0xE3, 0x70, 0x10, 0x10, 0x6C, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                GameDrawMenuButton(0xB8, 0x68, 0x20, 0x20, 0x95, 0x25, 0x1E);
                GameDrawMenuButton(0xDA, 0x68, 0x20, 0x20, 0x1E, 0x8E, 0x95);
            }
        } else if (GameMenuBusy == -3) {
            RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 0);
            if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                if (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) {
                    GameMenuBusy = -4;
                }
            }
        } else if (GameMenuBusy == -4) {
            RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
            RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
            if (g_UiScriptProgress2 <= 0) {
                GameMenuBusy = 0;
            }
        } else if (GameMenuBusy == -5) {
            if (g_MenuConfirmTimer <= 0) {
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
                if (g_UiScriptProgress2 <= 0) {
                    GameMenuBusy = 0;
                    g_CarTable[g_PlayerCarIndex].tireCompound = g_MenuSubCursor;
                    g_TimeAttackCarTires[g_PlayerCarIndex * 8] = g_MenuSubCursor;
                }
            } else {
                g_MenuConfirmTimer -= 1;
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1);
                DrawTireCompoundSlider(g_MenuSubCursor, 1);
            }
        } else if (GameMenuBusy == -6) {
            if (g_MenuConfirmTimer <= 0) {
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
                if (g_UiScriptProgress2 <= 0) {
                    GameMenuBusy = 0;
                }
            } else {
                g_MenuConfirmTimer -= 1;
                RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1);
                DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xDA : 0xB8, 0x68, 0x20, 0x20, 1);
                DrawSprite(ot, 0xC2, 0x70, 0xC, 0x10, 0x60, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                DrawSprite(ot, 0xE3, 0x70, 0x10, 0x10, 0x6C, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                GameDrawMenuButton(0xB8, 0x68, 0x20, 0x20, 0x95, 0x25, 0x1E);
                GameDrawMenuButton(0xDA, 0x68, 0x20, 0x20, 0x1E, 0x8E, 0x95);
            }
        }
        DrawFadingMenuSprites(g_UiScriptProgress, mode, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 5;
    RunTimedDrawScript(cmdList, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, mode, g_RankingOption);
    if (g_UiScriptProgress <= 0) {
        switch (GameMenuBusy) {
        case 1:
            if (g_MenuViewOffset <= 0x3D08F) {
                return;
            }
            g_MenuScreen = 6;
            g_MenuHandlerIndex = 6;
            break;
        case 2:
            g_MenuScreen = 4;
            g_MenuHandlerIndex = 4;
            g_RankingOption = 0;
            break;
        }
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}
