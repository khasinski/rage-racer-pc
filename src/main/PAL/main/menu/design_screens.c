#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"


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


void UpdatePaintColorScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuCarView();

    if (GameMenuBusy == 0) {
        DrawPaintColorPalette(&g_UiScriptProgress2, -1, g_PaintColorIndex);
        DrawBrowseArrows(-1, 0, 1, 1);
        DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
        RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, 0);
        if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) == 0) {
            return;
        }
        if (g_UiScriptProgress2 > 0) {
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
        {
            u16 f = g_PadPressed;
            if (f & 0x860) {
                s32 sel = g_PaintColorCursor;
                s32 val;
                if (sel == 0) {
                    PlaySoundCue(2);
                    val = g_CarTable[g_PlayerCarIndex].paintColor1;
                    GameMenuBusy = -1;
                    g_UiScriptProgress2 = 0;
                    g_PaintColorIndex = val;
                } else if (sel == 1) {
                    PlaySoundCue(2);
                    val = g_CarTable[g_PlayerCarIndex].paintColor2;
                    GameMenuBusy = -2;
                    g_UiScriptProgress2 = 0;
                    g_PaintColorIndex = val;
                } else if (sel == 2) {
                    PlaySoundCue(3);
                    GameMenuBusy = 1;
                    g_MenuOverlayPattern = 2;
                    g_MenuViewOffsetTarget = 0x3D090;
                }
            } else if (f & 0x90) {
                PlaySoundCue(3);
                GameMenuBusy = 3;
                g_MenuOverlayPattern = 2;
                g_MenuViewOffsetTarget = 0x3D090;
            }
        }
        return;
    }

    if (GameMenuBusy < 0) {
        if (DrawPaintColorPalette(&g_UiScriptProgress2, 1, g_PaintColorIndex) != 0) {
            if (g_PadPressedRepeat & PAD_LEFT) {
                PlaySoundCue(1);
                g_PaintColorIndex = g_PaintColorIndex > 0 ? g_PaintColorIndex - 1 : 0x11;
            }
            if (g_PadPressedRepeat & PAD_RIGHT) {
                PlaySoundCue(1);
                g_PaintColorIndex = g_PaintColorIndex < 17 ? g_PaintColorIndex + 1 : 0;
            }
            if (GameMenuBusy == -1) {
                u16 *btn = &g_PadPressed;
                if (*btn & 0x860) {
                    PlaySoundCue(2);
                    g_CarTable[g_PlayerCarIndex].paintColor1 = g_PaintColorIndex;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor1 = g_PaintColorIndex;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor2 = g_CarTable[g_PlayerCarIndex].paintColor2;
                    GameMenuBusy = 0;
                }
                if (*btn & 0x90) {
                    PlaySoundCue(3);
                    g_PaintColorIndex = g_CarTable[g_PlayerCarIndex].paintColor1;
                    GameMenuBusy = 0;
                }
                SetBodyColor1(g_PaintColorIndex);
            } else {
                u16 *btn = &g_PadPressed;
                if (*btn & 0x860) {
                    PlaySoundCue(2);
                    g_CarTable[g_PlayerCarIndex].paintColor2 = g_PaintColorIndex;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor1 = g_CarTable[g_PlayerCarIndex].paintColor1;
                    g_TimeAttackCars[g_PlayerCarIndex].paintColor2 = g_PaintColorIndex;
                    GameMenuBusy = 0;
                }
                if (*btn & 0x90) {
                    PlaySoundCue(3);
                    g_PaintColorIndex = g_CarTable[g_PlayerCarIndex].paintColor2;
                    GameMenuBusy = 0;
                }
                SetBodyColor2(g_PaintColorIndex);
            }
        }

        DrawBrowseArrows(1, 0, 1, 1);
        DrawFadingMenuSprites(g_UiScriptProgress, 2, g_PaintColorCursor);
        RunTimedDrawScript(g_PaintColorScreenScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 10;
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
/*
 * The nearest car the player does not already own, walking `step` at a time.
 * -1 when there is none.
 */
static s32 FindCarNotOwned(s32 from, s32 step) {
    s32 index;

    for (index = from; index >= 0 && index < 13; index += step) {
        if (g_CarTable[index].enabled == 0) {
            return index;
        }
    }
    return -1;
}

/*
 * The same walk, but only over cars the player's progress has reached. Below
 * the last class the class above counts as reached, so the shop shows what is
 * coming next; in the last class it does not.
 */
static s32 FindCarOnOffer(s32 from, s32 step) {
    s32 index;

    for (index = from; index >= 0 && index < 13; index += step) {
        s32 unlockLevel = GetCarUnlockLevel(index);
        s32 progress;

        if (g_CarTable[index].enabled != 0) {
            continue;
        }
        progress = g_RaceProgress->maxClassReached;
        if (progress < 4 ? progress + 1 >= unlockLevel
                         : progress >= unlockLevel) {
            return index;
        }
    }
    return -1;
}

void UpdateCarListCursor(void) {
    if (g_CarShopUnlockAll != 0) {
        g_PrevOwnedCarIndex = FindCarNotOwned(g_CarListCursor - 1, -1);
        g_NextOwnedCarIndex = FindCarNotOwned(g_CarListCursor + 1, 1);
    } else {
        g_PrevOwnedCarIndex = FindCarOnOffer(g_CarListCursor - 1, -1);
        g_NextOwnedCarIndex = FindCarOnOffer(g_CarListCursor + 1, 1);
    }
}
