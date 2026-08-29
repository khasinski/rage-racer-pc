#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/save_internal.h"
#include "game/race.h"

void UpdateRankingScreen(void) {
    s32 state;

    g_MenuAltLayout = 0;
    DrawMenuCourseView();
    DrawMenuLightBurst(-9);
    state = GameMenuBusy;
    if (state == 0) {
        g_UiScriptProgress2 = 0;
        GameMenuBusy = -1;
        DrawFadingMenuSprites(0, 2, g_RankingCursor);
        RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, 1);
    } else if (state < 0) {
        switch (state) {
        case -1:
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, 1) != 0) {
                g_MenuOverlayPattern = -1;
                if (g_PadPressed & PAD_UP) {
                    PlaySoundCue(1);
                    g_RankingCursor = (g_RankingCursor > 0) ? g_RankingCursor - 1 : 2;
                }
                if (g_PadPressed & PAD_DOWN) {
                    PlaySoundCue(1);
                    g_RankingCursor = (g_RankingCursor < 2) ? g_RankingCursor + 1 : 0;
                }
                {
                    s32 flags = g_PadPressed;
                    if (flags & 0x860) {
                        s32 x = g_RankingCursor;
                        if (x == 0) {
                            PlaySoundCue(2);
                            GameMenuBusy = -2;
                            g_RankingPendingState = -3;
                        } else if (x == 1) {
                            PlaySoundCue(2);
                            GameMenuBusy = -2;
                            g_RankingPendingState = -5;
                        } else if (x == 2) {
                            PlaySoundCue(3);
                            GameMenuBusy = 1;
                            g_MenuOverlayPattern = x;
                        }
                    } else if (flags & 0x90) {
                        PlaySoundCue(3);
                        GameMenuBusy = 1;
                        g_MenuOverlayPattern = 2;
                    }
                }
            }
            break;
        case -2:
            RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, -1);
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = g_RankingPendingState;
            break;
        case -3:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 0) == 0) {
                break;
            }
            if (!(g_PadPressed & (PAD_START | PAD_SQUARE | PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = -4;
            break;
        case -4:
            DrawRankingTable(&g_UiScriptProgress2, -1, 0);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = -1;
            break;
        case -5:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 1) == 0) {
                break;
            }
            if (!(g_PadPressed & (PAD_START | PAD_SQUARE | PAD_CROSS | PAD_CIRCLE | PAD_TRIANGLE))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = -6;
            break;
        case -6:
            DrawRankingTable(&g_UiScriptProgress2, -1, 1);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = -1;
            break;
        }
        RunTimedDrawScript(&g_RankingPanelScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }
    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 2;
    RunTimedDrawScript(&g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    RunTimedDrawScript(&g_RankingPanelScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
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

s32 DrawCarSelectScreen(s32 step) {
    OT_TYPE *ot = SCRATCH_OT_BASE_AS(OT_TYPE);
    OrderingTableAddress otAddress;
    s32 p;
    u32 *buf = (u32 *)(ot + 1);
    s32 v;
    s32 col;
    s32 xpos;
    s32 mode;
    u8 tex;

    otAddress.pointer = ot;
    p = otAddress.value;

    if (step == 0) {
        g_CarSelectFadeAccum = 0;
        return p;
    }

    if (step > 0) {
        g_CarSelectFadeAccum += step;
        if (g_CarSelectFadeAccum >= 509) {
            g_CarSelectFadeAccum = 508;
        }
    } else {
        g_CarSelectFadeAccum += step;
        if (g_CarSelectFadeAccum < 0) {
            g_CarSelectFadeAccum = 0;
        }
    }

    v = g_CarSelectFadeAccum / 4U;
    col = v & 0xff;
    DrawRectOutline(buf, 0xa3, 0x180, 0x1a, 0x19, col, col, col, 0x20);

    tex = g_CarTable[g_PlayerCarIndex].transmission;
    if (tex != 0) {
        DrawSprite(buf, 0xad, 0x185, 0x10, 0x10, 0x6c, 0x7c, col, col, col,
                      0x244, 0, 1, 0x3b);
        xpos = 0xa5;
    } else {
        DrawSprite(buf, 0xae, 0x185, 0xc, 0x10, 0x60, 0x7c, col, col, col,
                      0x244, 0, 1, 0x3b);
        xpos = 0xa6;
    }

    mode = g_CarModelAsset->gearCount;
    switch (mode) {
    case 4:
        DrawSprite(buf, xpos, 0x185, 8, 0x10, 0x20, 0x18, v & 0xff, v & 0xff,
                      v & 0xff, 0x244, 0, 1, 0x3b);
        break;
    case 5:
        DrawSprite(buf, xpos, 0x185, 8, 0x10, 0x28, 0x18, v & 0xff, v & 0xff,
                      v & 0xff, 0x244, 0, 1, 0x3b);
        break;
    case 6:
        DrawSprite(buf, xpos, 0x185, 8, 0x10, 0x30, 0x18, v & 0xff, v & 0xff,
                      v & 0xff, 0x244, 0, 1, 0x3b);
        break;
    }

    return g_CarSelectFadeAccum;
}

void UpdateOwnedCarNeighbours(void) {
    s32 index;
    CarEntry *ptr;

    g_PrevOwnedCarIndex = -1;
    index = g_PlayerCarIndex - 1;
    if (index >= 0) {
        s32 one = 1;
        ptr = &g_CarTable[index];
        while (index >= 0) {
            if (ptr->enabled == one) {
                g_PrevOwnedCarIndex = index;
                break;
            }
            index--;
            ptr--;
        }
    }

    g_NextOwnedCarIndex = -1;
    index = g_PlayerCarIndex + 1;
    if (index < 13) {
        s32 one = 1;
        ptr = &g_CarTable[index];
        while (index < 13) {
            if (ptr->enabled == one) {
                g_NextOwnedCarIndex = index;
                break;
            }
            index++;
            ptr++;
        }
    }
}
void RefreshCarUnlockState(void) {
    s32 index;
    s32 value;
    CarEntry *ptr;
    CarEntry *enabledPtr;
    s32 byte;

    g_ShopCarIndex = -1;

    if (g_CarShopUnlockAll != 0) {
        index = 12;
        enabledPtr = &g_CarTable[12];
while (1) {
        byte = enabledPtr->enabled;
        enabledPtr--;
        if (byte == 0) {
            g_ShopCarIndex = index;
        }
        index--;
        if (index < 0) {
            return;
        }
        }
    }

    index = 12;
do {
    {
        value = GetCarUnlockLevel(index);
        ptr = &g_CarTable[index];
        if (ptr->enabled == 0) {
            if (g_RaceProgress->maxClassReached < 4) {
                if ((g_RaceProgress->maxClassReached + 1) < value) {
                    index--;
                    continue;
                }
            } else if (g_RaceProgress->maxClassReached < value) {
                index--;
                continue;
            }
            g_ShopCarIndex = index;
        }
        index--;
    }
    } while (index >= 0);

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


void UpdateCarSelectScreen(void) {
    s32 mode;
    u8 *cmdList;
    s32 lowMode;
    s32 sel;
    s32 t;
    s32 u;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    mode = 2;
    DrawMenuCarView();
    DrawMenuLightBurst(-9);
    if (g_GrandPrixMode != 0) {
        mode = 4;
    }
    cmdList = (u8 *)&g_CarSelectMenuScriptTimeAttack;
    if (g_GrandPrixMode != 0) {
        cmdList = (u8 *)&g_CarSelectMenuScriptGp;
    }

    if (GameMenuBusy == 0) {
        g_CarNamePlateStep = 0x14;
        g_CarSpecGraphStep = 3;
        g_MenuPlateCarIndex = g_PlayerCarIndex;
        RunTimedDrawScript(g_CarSelectPopupScript, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        DrawBrowseArrows(
            1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        {
            s32 initial;

            initial = -1;
            if (g_GrandPrixMode == 0) {
                DrawOwnedCarCounter(1, CountOwnedCars());
            }
            lowMode = mode & 0xFF;
            DrawFadingMenuSprites(g_UiScriptProgress, lowMode, g_CarSelectCursor);
            RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
            if ((RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) !=
                 0) &&
                (g_UiScriptProgress2 <= 0)) {
                g_MenuOverlayPattern = initial;
                if (g_PadPressed & PAD_UP) {
                    PlaySoundCue(1);
                    g_CarSelectCursor =
                        (g_CarSelectCursor > 0) ? g_CarSelectCursor - 1 : lowMode;
                }
                if (g_PadPressed & PAD_DOWN) {
                    PlaySoundCue(1);
                    g_CarSelectCursor =
                        (g_CarSelectCursor < mode) ? g_CarSelectCursor + 1 : 0;
                }
                UpdateOwnedCarNeighbours();
                RefreshCarUnlockState();
                sel = g_PlayerCarIndex;
                if ((g_PadHeld & PAD_LEFT) && (g_PrevOwnedCarIndex != -1)) {
                    t = g_MenuViewAngleTarget;
                    u = g_MenuViewAngle;
                    if (t < u ? (u - t <= 0x493DF) : (t - u <= 0x493DF)) {
                        if (g_CarSwapToIndex < 0) {
                            s32 prev;

                            PlaySoundCue(8);
                            g_PlayerCarIndex = g_PrevOwnedCarIndex;
                            RequestCarModel(g_PrevOwnedCarIndex);
                            prev = g_MenuViewAngleTarget;
                            g_CarSwapFromIndex = sel;
                            g_MenuViewAngleTarget = 0;
                            g_MenuAltPanelStep2 = -1;
                            g_CarSwapToIndex = g_PlayerCarIndex;
                            g_MenuViewAngle =
                                (g_MenuViewAngle - prev) + 0x927C0;
                        }
                    }
                }
                if ((g_PadHeld & PAD_RIGHT) && (g_NextOwnedCarIndex != -1)) {
                    t = g_MenuViewAngleTarget;
                    u = g_MenuViewAngle;
                    if (t < u ? (u - t <= 0x493DF) : (t - u <= 0x493DF)) {
                        if (g_CarSwapToIndex < 0) {
                            s32 base;
                            s32 prev;

                            PlaySoundCue(8);
                            g_PlayerCarIndex = g_NextOwnedCarIndex;
                            RequestCarModel(g_NextOwnedCarIndex);
                            base = 0x927C0;
                            prev = g_MenuViewAngleTarget;
                            g_MenuViewAngleTarget = 0x124F80;
                            g_CarSwapFromIndex = sel;
                            g_MenuAltPanelStep2 = -1;
                            g_CarSwapToIndex = g_PlayerCarIndex;
                            g_MenuViewAngle =
                                base - (prev - g_MenuViewAngle);
                        }
                    }
                }
                t = g_MenuViewAngleTarget;
                u = g_MenuViewAngle;
                if (t < u ? (u - t <= 0x493DF) : (t - u <= 0x493DF)) {
                    if (g_CarSwapToIndex < 0) {
                        if (g_PadPressed & PAD_CONFIRM) {
                            s32 choice;

                            choice = g_CarSelectCursor;
                            if (choice == 0) {
                                u16 series;

                                PlaySoundCue(2);
                                StartSequenceFadeOut();
                                if (g_GrandPrixMode != 0) {
                                    series = 0;
                                    if (g_GrandPrixClass < 5) {
                                        series = (u16)g_GrandPrixSeries;
                                    }
                                    g_GrandPrixSeries = series;
                                } else {
                                    g_GrandPrixSeries = g_CourseIndex >> 2;
                                }
                                RequestRoundAssets();
                                GameMenuBusy = 1;
                                g_MenuHintBarStep = -1;
                                g_CarNamePlateStep = -10;
                                g_MenuOverlayPattern = 0;
                                g_CarSpecGraphStep = -3;
                                g_MenuViewOffsetTarget = 0x3D090;
                                return;
                            }
                            if (choice == 1) {
                                PlaySoundCue(2);
                                GameMenuBusy = 2;
                                g_MenuOverlayPattern = 1;
                                g_CarNamePlateStep = -10;
                                return;
                            }
                            if (choice == mode) {
                                PlaySoundCue(3);
                                GameMenuBusy = 5;
                                g_MenuOverlayPattern = 2;
                                g_CarNamePlateStep = -10;
                                g_CarSpecGraphStep = -3;
                                g_MenuViewOffsetTarget = 0x3D090;
                                return;
                            }
                            if (choice == 2) {
                                s32 car;

                                car = g_ShopCarIndex;
                                if (car != -1) {
                                    s32 base;
                                    s32 prev;

                                    PlaySoundCue(2);
                                    g_CarListCursor = g_ShopCarIndex;
                                    RequestCarModel(g_CarListCursor);
                                    base = 0x927C0;
                                    prev = g_MenuViewAngleTarget;
                                    g_MenuViewAngleTarget = 0x124F80;
                                    GameMenuBusy = 3;
                                    g_MenuOverlayPattern = 1;
                                    g_CarSwapFromIndex = g_PlayerCarIndex;
                                    g_CarSwapToIndex = g_CarListCursor;
                                    g_MenuViewAngle =
                                        base - (prev - g_MenuViewAngle);
                                    return;
                                }
                                PlaySoundCue(5);
                                g_CarSelectPopupScript = (u8 *)&g_CarShopUnavailableScript;
                                GameMenuBusy = car;
                                g_UiScriptProgress2 = 0;
                                return;
                            }
                            if (choice == 3) {
                                s32 unlockLevel;

                                if (g_CarModelAsset->upgradesAvailable != 0) {
                                    unlockLevel =
                                        GetCarUnlockLevel(g_PlayerCarIndex);
                                    if (g_RaceProgress->maxClassReached >=
                                        unlockLevel) {
                                        GameMenuBusy = 4;
                                        g_MenuOverlayPattern = 1;
                                        PlaySoundCue(2);
                                        return;
                                    }
                                }
                                PlaySoundCue(5);
                                g_CarSelectPopupScript = (u8 *)&g_EngineerShopUnavailableScript;
                                GameMenuBusy = -2;
                                g_UiScriptProgress2 = 0;
                                return;
                            }
                        } else if ((g_PadPressed & PAD_CANCEL) &&
                                   ((u32)(g_MenuViewAngle - 0x2710) >
                                    0x120160U)) {
                            PlaySoundCue(3);
                            GameMenuBusy = 5;
                            g_MenuOverlayPattern = 2;
                            g_CarNamePlateStep = -10;
                            g_CarSpecGraphStep = -3;
                            g_MenuViewOffsetTarget = 0x3D090;
                        }
                    }
                }
            }
        }
        return;
    }

    if (GameMenuBusy < 0) {
        RunTimedDrawScript(g_CarSelectPopupScript, &g_UiScriptProgress2, 0);
        if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
            if (g_PadPressed & PAD_CONFIRM) {
                GameMenuBusy = 0;
            }
            if (g_PadPressed & PAD_CANCEL) {
                GameMenuBusy = 0;
            }
        }
        DrawBrowseArrows(
            1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        if (g_GrandPrixMode == 0) {
            DrawOwnedCarCounter(1, CountOwnedCars());
        }
        DrawFadingMenuSprites(g_UiScriptProgress, mode, g_CarSelectCursor);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 4;
    DrawBrowseArrows(
        -1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
    if (g_GrandPrixMode == 0) {
        DrawOwnedCarCounter(-1, CountOwnedCars());
    }
    RunTimedDrawScript(cmdList, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, mode, g_CarSelectCursor);
    if (g_UiScriptProgress <= 0) {
        switch (GameMenuBusy) {
        case 1:
            if ((g_MenuOutgoingScreenProgress > 0) &&
                (g_MenuViewOffset <= 0x3D08F)) {
                return;
            }
            g_SceneId = 9;
            g_CourseIndex &= 3;
            g_RaceProgress->course = g_CourseIndex;
            g_RaceProgress->carIndex = g_PlayerCarIndex;
            g_RaceProgress->classIndex = g_GrandPrixClass;
            if (g_GrandPrixMode != 0) {
                g_RaceProgress->money.value = g_PlayerMoney;
            } else {
                g_RaceProgress->money.value = g_GrandPrixSeries;
            }
            break;
        case 2:
            g_MenuScreen = 5;
            g_MenuHandlerIndex = 5;
            break;
        case 3:
            g_MenuScreen = 0xB;
            g_MenuHandlerIndex = 0xB;
            DrawCarShopPricePanel(0, 0, 0);
            DrawBrowseArrows(0, 0, 0, 0);
            DrawMenuAltPanel(0, 0);
            g_MenuAltPanelStep = 0;
            g_MenuAltPanelStep2 = 0;
            ClearTeamNameTexture();
            RestoreTeamLogoClut();
            break;
        case 4:
            g_MenuScreen = 0xC;
            g_MenuHandlerIndex = 0xC;
            DrawEngineerShopPricePanel(0, 0, 0);
            break;
        case 5:
        {
            s32 angle;
            s32 offset;
            s32 largeValue;
            s32 course;
            s32 minusOne;

            if (g_MenuViewOffset <= 0x3D08F) {
                return;
            }
            angle = 0x7A120;
            offset = 0x3D090;
            largeValue = 0x1F4000;
            course = g_CourseIndex;
            minusOne = -1;
            g_MenuViewAngle = angle;
            g_MenuViewAngleTarget = angle;
            g_MenuScreen = 1;
            g_MenuHandlerIndex = 1;
            g_CarSelectCursor = 0;
            g_MenuPendingCourseIndex = minusOne;
            g_MenuViewOffset = offset;
            g_MenuViewOffsetTarget = 0;
            g_CourseCardSpin = largeValue;
            g_MenuCourseModelIndex = course;
            g_CourseCardPendingGrade = g_CourseProgress->bestPlace[course & 3];
            DrawTimeAttackPlate(0);
            if (g_CourseIndex >= 4) {
                g_TimeAttackPlateStep = 1;
            } else {
                g_TimeAttackPlateStep = minusOne;
            }
            break;
        }
        }
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}

s32 DrawCustomizeScreen(s32 step) {
    s32 value;

    if (step == 0) {
        g_CustomizeFadeAccum = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_CustomizeFadeAccum;
        g_CustomizeFadeAccum = value;
        if (value >= 0x1FD) {
            g_CustomizeFadeAccum = 0x1FC;
        }
        value = 0;
    } else {
        s32 limit;
        u32 product;

        value = step + g_CustomizeFadeAccum;
        g_CustomizeFadeAccum = value;
        limit = 0x1FC;
        if (value < 0) {
            g_CustomizeFadeAccum = 0;
        }
        limit = limit - g_CustomizeFadeAccum;
        product = limit * limit;
        value = product / 2048;
    }

    DrawCarEngineSpec((s16)value, (g_CustomizeFadeAccum / 4U) & 0xFF, g_PlayerCarIndex);
    return g_CustomizeFadeAccum;
}


void UpdateCustomizeScreen(void) {
    void *ot;
    s32 mode;
    s32 lowMode;
    u8 *cmdList;
    u16 *pad;
    s32 sel;

    ot = SCRATCH_OT_BASE_AS(void);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    mode = 2;
    DrawMenuCarView();
    if (g_GrandPrixMode != 0) {
        mode = 3;
    }
    cmdList = (u8 *)&g_CustomizeMenuScriptTimeAttack;
    if (g_GrandPrixMode != 0) {
        cmdList = (u8 *)&g_CustomizeMenuScriptGp;
    }

    if (GameMenuBusy == 0) {
        g_CarSpecGraphStep = 3;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
        lowMode = mode & 0xFF;
        DrawFadingMenuSprites(g_UiScriptProgress, lowMode, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        if ((RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) != 0) && (g_UiScriptProgress2 <= 0)) {
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
                    g_CustomizePopupScript = (u8 *)&g_MenuDialogPanelUpperScript;
                    GameMenuBusy = -1;
                        g_UiScriptProgress2 = 0;
                        g_MenuSubCursor = carByte;
                        return;
                }
                if (sel == 1) {
                    if (g_CarModelAsset->transmissionAvailable != 0) {
                        PlaySoundCue(2);
                        carByte = g_CarTable[g_PlayerCarIndex].transmission;
                        g_CustomizePopupScript = (u8 *)&g_MenuDialogPanelLowerScript;
                        GameMenuBusy = -2;
                        g_UiScriptProgress2 = 0;
                        g_MenuSubCursor = carByte;
                        return;
                    }
                    PlaySoundCue(5);
                    g_CustomizePopupScript = (u8 *)&g_TransmissionUnavailableScript;
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
                GameDrawMenuButton(0xB8, 0x68, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                GameDrawMenuButton(0xDA, 0x68, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
            }
        } else if (GameMenuBusy == -3) {
            RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 0);
            if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                if (g_PadPressed & PAD_CONFIRM) {
                    GameMenuBusy = -4;
                }
                if (g_PadPressed & PAD_CANCEL) {
                    GameMenuBusy = -4;
                }
            }
        } else if (GameMenuBusy == -4) {
            RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
            RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
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
                GameDrawMenuButton(0xB8, 0x68, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                GameDrawMenuButton(0xDA, 0x68, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
            }
        }
        DrawFadingMenuSprites(g_UiScriptProgress, mode, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 5;
    RunTimedDrawScript(cmdList, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
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
