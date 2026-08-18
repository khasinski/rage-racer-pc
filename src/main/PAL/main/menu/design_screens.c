#include "common.h"
#include "game/game_input.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"


void UpdateLogoSampleScreen(void) {
    s32 v0;
    s32 t;
    s32 pl;

    g_MenuAltLayout = 0;
    ComposeSampleTeamLogo(g_LogoSampleCharIndex, g_LogoSampleBackIndex);
    DrawTeamLogoCanvas(1, 0);
    v0 = GameMenuBusy;
    if (v0 == 0) {
        RampTeamLogoCanvas(-10, 0);
        DrawLogoSamplePanel(-1, g_LogoSampleSavedIndex + 1);
        RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2, -1);
        DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
        RunTimedDrawScript(&g_LogoSampleScreenScript, &g_UiScriptProgress, 0);
        if (RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) == 0) return;
        if (g_UiScriptProgress2 > 0) return;
        g_MenuOverlayPattern = -1;
        if (g_GameInput.pressed & PAD_UP) {
            s32 n, c;
            PlaySoundCue(1);
            c = g_LogoSampleCursor;
            n = 2;
            if (c > 0) n = c - 1;
            g_LogoSampleCursor = n;
        }
        if (g_GameInput.pressed & PAD_DOWN) {
            s32 n, c;
            PlaySoundCue(1);
            c = g_LogoSampleCursor;
            n = 0;
            if (c < 2) n = c + 1;
            g_LogoSampleCursor = n;
        }
        if (g_GameInput.pressed & PAD_CONFIRM) {
            pl = g_LogoSampleCursor;
            if (pl == 0) {
                PlaySoundCue(2);
                GameMenuBusy = -1;
                g_UiScriptProgress2 = 0;
                g_LogoSampleSubPanelScript = &g_MenuRow0MarkerScript;
                g_LogoSampleSavedIndex = g_LogoSampleCharIndex;
            } else if (pl == 1) {
                PlaySoundCue(2);
                GameMenuBusy = -2;
                g_UiScriptProgress2 = 0;
                g_LogoSampleSubPanelScript = &g_MenuRow1MarkerScript;
                g_LogoSampleSavedIndex = g_LogoSampleBackIndex;
            } else if (pl == 2) {
                PlaySoundCue(3);
                GameMenuBusy = 1;
                g_MenuOverlayPattern = 2;
            }
        } else if (g_GameInput.pressed & PAD_CANCEL) {
            PlaySoundCue(3);
            GameMenuBusy = 1;
            g_MenuOverlayPattern = 2;
        }
        return;
    }

    if (v0 < 0) {
        RampTeamLogoCanvas(10, 0);
        if (GameMenuBusy == -1) {
            if (RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2, 1) != 0) {
                u16 *p = &g_GameInput.pressed;
                if (*p & 0x860) {
                    PlaySoundCue(2);
                    GameMenuBusy = 0;
                    g_LogoSampleSavedIndex = g_LogoSampleCharIndex;
                }
                if (*p & 0x90) {
                    PlaySoundCue(3);
                    GameMenuBusy = 0;
                    g_LogoSampleCharIndex = g_LogoSampleSavedIndex;
                }
                if (*p & 0x8000) {
                    s32 n, c;
                    PlaySoundCue(1);
                    c = g_LogoSampleCharIndex;
                    n = 0x13;
                    if (c > 0) n = c - 1;
                    g_LogoSampleCharIndex = n;
                }
                if (g_GameInput.pressed & PAD_RIGHT) {
                    s32 n, c;
                    PlaySoundCue(1);
                    c = g_LogoSampleCharIndex;
                    n = 0;
                    if (c < 19) n = c + 1;
                    g_LogoSampleCharIndex = n;
                }
            }
            t = g_LogoSampleCharIndex;
        } else {
            if (RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2, 1) != 0) {
                u16 *p = &g_GameInput.pressed;
                if (*p & 0x860) {
                    PlaySoundCue(2);
                    GameMenuBusy = 0;
                    g_LogoSampleSavedIndex = g_LogoSampleBackIndex;
                }
                if (*p & 0x90) {
                    PlaySoundCue(3);
                    GameMenuBusy = 0;
                    g_LogoSampleBackIndex = g_LogoSampleSavedIndex;
                }
                if (*p & 0x8000) {
                    s32 n, c;
                    PlaySoundCue(1);
                    c = g_LogoSampleBackIndex;
                    n = 0x13;
                    if (c > 0) n = c - 1;
                    g_LogoSampleBackIndex = n;
                }
                if (g_GameInput.pressed & PAD_RIGHT) {
                    s32 n, c;
                    PlaySoundCue(1);
                    c = g_LogoSampleBackIndex;
                    n = 0;
                    if (c < 19) n = c + 1;
                    g_LogoSampleBackIndex = n;
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

    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 8;
    DrawLogoSamplePanel(-1, 0);
    RunTimedDrawScript(&g_LogoSampleScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
    if (g_UiScriptProgress <= 0) {
        g_MenuScreen = 7;
        g_MenuHandlerIndex = 7;
        g_LogoSampleCursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
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


void UpdateTeamNameScreen(void) {
    u16 pad;
    register s32 newdepth asm("$2");

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawTeamNameCharModel();
    if (!(GameMenuBusy != 0)) {

    DrawTeamNameEntry(1, GameMenuCursor);
    if (RunTimedDrawScript(&g_TeamNameScreenScript, &g_UiScriptProgress, 1) == 0) return;
    g_MenuOverlayPattern = -1;

    if (g_TeamNameLength < 6) {
        if ((g_GameInput.pressedRepeat & PAD_DPAD) && GameMenuCursorAnim < 0) {
            if (g_GameInput.pressedRepeat & PAD_UP) { s32 u = GameMenuCursor; GameMenuCursor = (u < 0xB) ? u + 0x21 : u - 0xB; }
            if (g_GameInput.pressedRepeat & PAD_DOWN) { s32 d = GameMenuCursor; GameMenuCursor = (d < 0x21) ? d + 0xB : d - 0x21; }
            if (g_GameInput.pressedRepeat & PAD_LEFT) { s32 l = GameMenuCursor; GameMenuCursor = (l % 11 != 0) ? l - 1 : l + 0xA; }
            if (g_GameInput.pressedRepeat & PAD_RIGHT) {
                s32 r;
                s32 rn;
                r = GameMenuCursor;
                rn = r + 1;
                if (rn % 11 == 0) newdepth = r - 0xA; else newdepth = rn;
                GameMenuCursor = newdepth;
            }
            g_MenuViewAngleTarget = 0;
            g_MenuViewAngle = 0x3E8000;
            GameMenuCursorAnim = GameMenuCursor;
            PlaySoundCue(1);

        }
    } else {
        if ((g_GameInput.pressedRepeat & (PAD_LEFT | PAD_RIGHT)) && GameMenuCursorAnim < 0) {
            s32 nc = (GameMenuCursor == 0x2A) ? 0x2B : 0x2A;
            GameMenuCursor = nc;
            g_MenuViewAngleTarget = 0;
            g_MenuViewAngle = 0x3E8000;
            GameMenuCursorAnim = nc;
            PlaySoundCue(1);
        }
    }
    pad = g_GameInput.pressed;
    if (pad & 0x860) {
    {
        s32 c = GameMenuCursor;
        if (c == 0x2A) goto pop;
        if (!(c != 0x2B)) {
    PlaySoundCue(3);
    GameMenuBusy = 1;
    g_MenuOverlayPattern = 2;
    g_MenuViewOffsetTarget = 0x3D090;
    return;
    }
    }

    {
        u32 d;
        PlaySoundCue(2);
        g_TeamNameChars[g_TeamNameLength] = (u8)GameMenuCursor;
        d = g_TeamNameLength;
        if (d >= 5) GameMenuCursor = 0x2B;
        if (d >= 7) newdepth = d; else newdepth = d + 1;
    }
    } else {

    if (!(pad & 0x90)) return;
pop:
    if (g_TeamNameLength == 0) return;
    PlaySoundCue(4);
    {
        newdepth = 0xA;
        g_TeamNameChars[g_TeamNameLength] = newdepth;
    }
    newdepth = g_TeamNameLength - 1;
    }
    g_TeamNameLength = newdepth;
    return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 9;
    DrawTeamNameEntry(-1, GameMenuCursor);
    RunTimedDrawScript(&g_TeamNameScreenScript, &g_UiScriptProgress, -1);
    if (g_UiScriptProgress > 0) return;
    if (0x3D08F < g_MenuViewOffset) {
        g_MenuScreen = 6;
        g_MenuHandlerIndex = 6;
        UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
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


void UpdatePaintColorScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuCarView();

    if (GameMenuBusy == 0) {
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
        g_MenuOverlayPattern = -1;
        if (g_GameInput.pressed & PAD_UP) {
            PlaySoundCue(1);
            g_PaintColorCursor = g_PaintColorCursor > 0 ? g_PaintColorCursor - 1 : 2;
        }
        if (g_GameInput.pressed & PAD_DOWN) {
            PlaySoundCue(1);
            g_PaintColorCursor = g_PaintColorCursor < 2 ? g_PaintColorCursor + 1 : 0;
        }
        {
            u16 f = g_GameInput.pressed;
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
            if (g_GameInput.pressedRepeat & PAD_LEFT) {
                PlaySoundCue(1);
                g_PaintColorIndex = g_PaintColorIndex > 0 ? g_PaintColorIndex - 1 : 0x11;
            }
            if (g_GameInput.pressedRepeat & PAD_RIGHT) {
                PlaySoundCue(1);
                g_PaintColorIndex = g_PaintColorIndex < 17 ? g_PaintColorIndex + 1 : 0;
            }
            if (GameMenuBusy == -1) {
                u16 *btn = &g_GameInput.pressed;
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
                u16 *btn = &g_GameInput.pressed;
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
        RunTimedDrawScript(&g_PaintColorScreenScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 10;
    RunTimedDrawScript(&g_PaintColorScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
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
