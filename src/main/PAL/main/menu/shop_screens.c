#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"

void UpdateCarShopScreen(void) {
    void *ot;
    s32 value;
    s32 res;
    s32 sel;
    s32 t;
    s32 u;
    u32 modalState;

    ot = SCRATCH_OT_BASE_AS(void);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuAltPanel(g_MenuAltPanelStep, g_MenuAltPanelStep2);
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    value = g_CarPriceTable[GetOwnedCarAssetIndex(g_CarListCursor)];
    if (GameMenuBusy == 0) {
        g_MenuPlateCarIndex = g_CarListCursor;
        RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        DrawBrowseArrows(1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        DrawCarShopPricePanel(1, g_PlayerMoney, value);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
        RunTimedDrawScript(&g_CarShopScreenScript, &g_UiScriptProgress, 0);
        {
        s32 initial;

        initial = -1;
        res = RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        if ((res != 0) && (g_UiScriptProgress2 <= 0)) {
            g_MenuOverlayPattern = initial;
            if (g_PadPressed & PAD_UP) {
                PlaySoundCue(1);
                g_CarShopOption = (g_CarShopOption > 0) ? g_CarShopOption - 1 : 1;
            }
            if (g_PadPressed & PAD_DOWN) {
                PlaySoundCue(1);
                g_CarShopOption = (g_CarShopOption <= 0) ? g_CarShopOption + 1 : 0;
            }
            UpdateCarListCursor();
            sel = g_CarListCursor;
            if ((g_PadHeld & PAD_LEFT) && (g_PrevOwnedCarIndex != -1)) {
                t = g_MenuViewAngleTarget;
                u = g_MenuViewAngle;
                if (t < u ? (u - t <= 0x493DF) : (t - u <= 0x493DF)) {
                    if (g_CarSwapToIndex < 0) {
                        s32 lprev;

                        PlaySoundCue(8);
                        g_CarListCursor = g_PrevOwnedCarIndex;
                        RequestCarModel(g_PrevOwnedCarIndex);
                        lprev = g_MenuViewAngleTarget;
                        g_CarSwapFromIndex = sel;
                        g_MenuViewAngleTarget = 0;
                        g_MenuAltPanelStep2 = -1;
                        g_CarSwapToIndex = g_CarListCursor;
                        g_MenuViewAngle = (g_MenuViewAngle - lprev) + 0x927C0;
                    }
                }
            }
            if ((g_PadHeld & PAD_RIGHT) && (g_NextOwnedCarIndex != -1)) {
                t = g_MenuViewAngleTarget;
                u = g_MenuViewAngle;
                if (t < u ? (u - t <= 0x493DF) : (t - u <= 0x493DF)) {
                    if (g_CarSwapToIndex < 0) {
                        s32 base;
                        s32 lprev;

                        PlaySoundCue(8);
                        g_CarListCursor = g_NextOwnedCarIndex;
                        RequestCarModel(g_NextOwnedCarIndex);
                        base = 0x927C0;
                        lprev = g_MenuViewAngleTarget;
                        g_MenuViewAngleTarget = 0x124F80;
                        g_CarSwapFromIndex = sel;
                        g_MenuAltPanelStep2 = -1;
                        g_CarSwapToIndex = g_CarListCursor;
                        g_MenuViewAngle = base - (lprev - g_MenuViewAngle);
                    }
                }
            }
            if (g_CarModelAsset->transmissionAvailable == 0) {
                g_MenuAltPanelStep = 1;
            } else {
                g_MenuAltPanelStep = -1;
            }
            t = g_MenuViewAngleTarget;
            u = g_MenuViewAngle;
            if (t < u ? (u - t <= 0x493DF) : (t - u <= 0x493DF)) {
                if (g_CarSwapToIndex < 0) {
                    if (g_PadPressed & PAD_CONFIRM) {
                        sel = g_CarShopOption;
                        if (sel == 1) {
                            if (g_PlayerCarIndex != g_CarListCursor) {
                                s32 base;
                                s32 current;
                                s32 selected;
                                s32 lu;
                                s32 lprev;

                                RequestCarModel(g_PlayerCarIndex);
                                base = 0x927C0;
                                current = g_CarListCursor;
                                selected = g_PlayerCarIndex;
                                lu = g_MenuViewAngle;
                                lprev = g_MenuViewAngleTarget;
                                g_MenuViewAngleTarget = 0;
                                g_CarSwapFromIndex = current;
                                g_CarSwapToIndex = selected;
                                g_MenuViewAngle = (lu - lprev) + base;
                            }
                            PlaySoundCue(3);
                            g_MenuOverlayPattern = 2;
                            GameMenuBusy = sel;
                        g_MenuAltPanelStep = -1;
                        g_MenuAltPanelStep2 = -1;
                                                return;
                        }
                        if (sel != 0) {
                            return;
                        }
                        if (g_CarTable[g_CarListCursor].enabled == 0) {
                            PlaySoundCue(2);
                            GameMenuBusy = -1;
                            g_UiScriptProgress2 = 0;
                            g_MenuSubCursor = 0;
                            switch (g_CarListCursor) {
                            case 0:
                            case 1:
                            case 2:
                            case 10:
                                g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript1;
                                return;
                            case 3:
                                g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript2;
                                return;
                            case 4:
                            case 5:
                            case 6:
                            case 11:
                                g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript3;
                                return;
                            case 7:
                            case 8:
                            case 9:
                            case 12:
                                g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript4;
                                return;
                            }
                        }
                        return;
                    } else if (g_PadPressed & PAD_CANCEL) {
                        if (g_PlayerCarIndex != g_CarListCursor) {
                            s32 base;
                            s32 current;
                            s32 selected;
                            s32 lu;
                            s32 lprev;

                            RequestCarModel(g_PlayerCarIndex);
                            base = 0x927C0;
                            current = g_CarListCursor;
                            selected = g_PlayerCarIndex;
                            lu = g_MenuViewAngle;
                            lprev = g_MenuViewAngleTarget;
                            g_MenuViewAngleTarget = 0;
                            g_CarSwapFromIndex = current;
                            g_CarSwapToIndex = selected;
                            g_MenuViewAngle = (lu - lprev) + base;
                        }
                        PlaySoundCue(3);
                        GameMenuBusy = 1;
                        g_MenuOverlayPattern = 2;
                        g_MenuAltPanelStep = -1;
                        g_MenuAltPanelStep2 = -1;
                    }
                }
            }
        }
        }
    } else {
        if (GameMenuBusy < 0) {
            modalState = GameMenuBusy + 2;
            if (modalState < 2U) {
                u16 *pad;

                RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, 0);
                if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                    if (GameMenuBusy == -1) {
                        if (g_PadPressed & PAD_CONFIRM) {
                            if (g_MenuSubCursor != 0) {
                                if (g_PlayerMoney >= value) {
                                    PlaySoundCue(2);
                                    GameMenuBusy = -3;
                                    g_MenuConfirmTimer = 0x23;
                                } else {
                                    PlaySoundCue(5);
                                    g_CarShopModalScript = (u8 *)&g_CarShopNoFundsScript;
                                    GameMenuBusy = -2;
                                }
                            } else {
                                PlaySoundCue(3);
                                GameMenuBusy = 0;
                            }
                        }
                        pad = &g_PadPressed;
                        if (*pad & 0x90) {
                            PlaySoundCue(3);
                            GameMenuBusy = 0;
                        }
                        if ((*pad & 0x8000) && (g_MenuSubCursor == 0)) {
                            PlaySoundCue(1);
                            g_MenuSubCursor = 1;
                        }
                        if (g_PadPressed & PAD_RIGHT) {
                            if (g_MenuSubCursor != 0) {
                                PlaySoundCue(1);
                                g_MenuSubCursor = 0;
                            }
                        }
                    } else {
                        if (g_PadPressed & PAD_CONFIRM) {
                            GameMenuBusy = 0;
                        }
                        if (g_PadPressed & PAD_CANCEL) {
                            GameMenuBusy = 0;
                        }
                    }
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 0);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            } else if (GameMenuBusy == -3) {
                if (g_MenuConfirmTimer <= 0) {
                    RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, -1);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
                    if (g_UiScriptProgress2 <= 0) {
                        g_CarTable[g_CarListCursor].enabled = 1;
                        g_TimeAttackCarEnabled[g_CarListCursor * 8] = 1;
                        GameMenuBusy = 2;
                        g_MenuAltPanelStep = -1;
                        g_PlayerCarIndex = g_CarListCursor;
                    }
                } else {
                    g_MenuConfirmTimer -= 1;
                    RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, 0);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1);
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 1);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            }
            DrawBrowseArrows(1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
            DrawCarShopPricePanel(1, g_PlayerMoney, value);
            DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
            RunTimedDrawScript(&g_CarShopScreenScript, &g_UiScriptProgress, 0);
            RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
            return;
        }
        g_MenuHandlerIndex = -1;
        g_MenuHandlerIndex2 = 0xB;
        DrawBrowseArrows(-1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        DrawCarShopPricePanel(-1, g_PlayerMoney, value);
        RunTimedDrawScript(&g_CarShopScreenScript, &g_UiScriptProgress, -1);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
        if (g_UiScriptProgress <= 0) {
            if (GameMenuBusy == 2) {
                g_PlayerMoney -= value;
            }
            g_MenuScreen = 4;
            g_MenuHandlerIndex = 4;
            g_UiScriptProgress = 0;
            GameMenuBusy = 0;
            g_CarShopOption = 0;
            UploadTeamNameTexture(&g_TeamNameChars, g_TeamNameLength);
            UploadTeamLogoClut();
        }
    }
}

u32 DrawEngineerShopScreen(s32 step) {
    s32 value;
    s32 amount;

    if (step == 0) {
        g_EngineSpecStep = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_EngineSpecStep;
        g_EngineSpecStep = value;
        if (value >= 0x1FD) {
            g_EngineSpecStep = 0x1FC;
        }
        amount = 0;
    } else {
        s32 diff = 0x1FC;
        u32 product;

        value = step + g_EngineSpecStep;
        g_EngineSpecStep = value;
        if (value < 0) {
            g_EngineSpecStep = 0;
        }
        diff -= g_EngineSpecStep;
        product = diff * diff;
        amount = product / 2048;
    }

    DrawCarEngineSpec((s16)amount, (u8)(g_EngineSpecStep >> 2), g_PlayerCarIndex);
    return g_EngineSpecStep;
}


void UpdateEngineerShopScreen(void) {
    void *ot;
    s32 value;
    s32 res;
    s32 sel;

    ot = SCRATCH_OT_BASE_AS(void);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    g_MenuPlateCarIndex = g_PlayerCarIndex;
    value = g_CarTuneUpPriceTable[GetOwnedCarAssetIndex(g_PlayerCarIndex)];
    if (GameMenuBusy == 0) {
        RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        DrawEngineerShopPricePanel(1, g_PlayerMoney, value);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
        RunTimedDrawScript(&g_EngineerShopScreenScript, &g_UiScriptProgress, 0);
        res = RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        if ((res != 0) && (g_UiScriptProgress2 <= 0)) {
            g_MenuOverlayPattern = -1;
            if (g_PadPressed & PAD_UP) {
                PlaySoundCue(1);
                g_EngineerShopOption = (g_EngineerShopOption > 0) ? g_EngineerShopOption - 1 : 1;
            }
            if (g_PadPressed & PAD_DOWN) {
                PlaySoundCue(1);
                g_EngineerShopOption = (g_EngineerShopOption <= 0) ? g_EngineerShopOption + 1 : 0;
            }
            if (g_PadPressed & PAD_CONFIRM) {
                sel = g_EngineerShopOption;
                if (sel == 0) {
                    if (g_PlayerMoney >= value) {
                        PlaySoundCue(2);
                        g_EngineerShopModalScript = (u8 *)&g_EngineerShopTuneUpPromptScript;
                        GameMenuBusy = -1;
                        g_UiScriptProgress2 = 0;
                        g_MenuSubCursor = 0;
                    } else {
                        PlaySoundCue(5);
                        g_EngineerShopModalScript = (u8 *)&g_EngineerShopNoFundsScript;
                        GameMenuBusy = -3;
                        g_UiScriptProgress2 = 0;
                    }
                } else if (sel == 1) {
                    PlaySoundCue(3);
                    GameMenuBusy = sel;
                    g_MenuOverlayPattern = 2;
                }
            } else if (g_PadPressed & PAD_CANCEL) {
                PlaySoundCue(3);
                GameMenuBusy = 1;
                g_MenuOverlayPattern = 2;
            }
        }
    } else {
        if (GameMenuBusy < 0) {
            if (GameMenuBusy == -1) {
                u16 *pad;

                RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
                if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                    if (g_PadPressed & PAD_CONFIRM) {
                        if (g_MenuSubCursor != 0) {
                            PlaySoundCue(2);
                            GameMenuBusy = -2;
                            g_MenuConfirmTimer = 0x23;
                            RequestUpgradedCarModel(g_PlayerCarIndex);
                        } else {
                            PlaySoundCue(3);
                            GameMenuBusy = 0;
                        }
                    }
                    pad = &g_PadPressed;
                    if (*pad & 0x90) {
                        PlaySoundCue(3);
                        GameMenuBusy = 0;
                    }
                    if ((*pad & 0x8000) && (g_MenuSubCursor == 0)) {
                        PlaySoundCue(1);
                        g_MenuSubCursor = 1;
                    }
                    if (g_PadPressed & PAD_RIGHT) {
                        if (g_MenuSubCursor != 0) {
                            PlaySoundCue(1);
                            g_MenuSubCursor = 0;
                        }
                    }
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 0);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            } else if (GameMenuBusy == -2) {
                if (g_MenuConfirmTimer <= 0) {
                    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
                    if (g_UiScriptProgress2 <= 0) {
                        g_MenuViewAngle = 0x927C0;
                        g_MenuViewAngleTarget = 0;
                        GameMenuBusy = 2;
                        g_MenuOverlayPattern = 2;
                        g_CarSwapFromIndex = g_PlayerCarIndex;
                        g_CarSwapToIndex = g_PlayerCarIndex;
                    }
                } else {
                    g_MenuConfirmTimer -= 1;
                    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1);
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 1);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            } else {
                RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
                if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                    if (g_PadPressed & PAD_CONFIRM) {
                        GameMenuBusy = 0;
                    }
                    if (g_PadPressed & PAD_CANCEL) {
                        GameMenuBusy = 0;
                    }
                }
            }
            DrawEngineerShopPricePanel(1, g_PlayerMoney, value);
            DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
            RunTimedDrawScript(&g_EngineerShopScreenScript, &g_UiScriptProgress, 0);
            RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
            return;
        }
        g_MenuHandlerIndex = -1;
        g_MenuHandlerIndex2 = 0xC;
        DrawEngineerShopPricePanel(-1, g_PlayerMoney, value);
        RunTimedDrawScript(&g_EngineerShopScreenScript, &g_UiScriptProgress, -1);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
        if (g_UiScriptProgress <= 0) {
            if (GameMenuBusy == 2) {
                g_CarTable[g_PlayerCarIndex].modelVariant++;
                if (g_CarTable[g_PlayerCarIndex].modelVariant > g_TimeAttackCars[g_PlayerCarIndex].modelVariant) {
                    g_TimeAttackCars[g_PlayerCarIndex].modelVariant = g_CarTable[g_PlayerCarIndex].modelVariant;
                }
                g_PlayerMoney -= value;
            }
            g_MenuScreen = 4;
            g_MenuHandlerIndex = 4;
            g_UiScriptProgress = 0;
            GameMenuBusy = 0;
            g_EngineerShopOption = 0;
        }
    }
}

void ShopScreenNoOp(void) {
}
