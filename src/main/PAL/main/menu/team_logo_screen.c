#include "game/audio.h"
#include "game/menu.h"
#include "game/team_logo.h"

static void DrawTeamLogoSaveButtons(void *ot, s32 flash) {
    DrawMenuCursorBox(g_MenuSubCursor != 0 ? 0xB8 : 0xDA, 0x44, 0x20, 0x20,
                      flash);
    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E);
    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x3A, 0x1E, 0x95);
}

static void ChooseTeamLogoOption(void) {
    switch (g_TeamLogoOption) {
    case 0:
        PlaySoundCue(2);
        GameMenuBusy = -1;
        g_MenuSubCursor = 0;
        g_UiScriptProgress2 = 0;
        g_TeamLogoSubPanelScript = g_MenuDialogPanelUpperScript;
        break;
    case 1:
        PlaySoundCue(2);
        ApplyDuckedSequenceAudio();
        GameMenuBusy = -3;
        g_TeamLogoPaintArmed = 0;
        g_UiScriptProgress2 = 0;
        g_TeamLogoSubPanelScript = g_MenuRow1MarkerScript;
        break;
    case 2:
        PlaySoundCue(3);
        GameMenuBusy = 2;
        g_MenuOverlayPattern = 2;
        break;
    }
}

static void UpdateTeamLogoIdle(void) {
    RampTeamLogoCanvas(-13, -21);
    RunTimedDrawScript(g_TeamLogoScreenScript2, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 0);
    DrawTeamLogoCanvas(1, -1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    RunTimedDrawScript(g_TeamLogoScreenScript, &g_UiScriptProgress, 0);
    if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) == 0 ||
        g_UiScriptProgress2 > 0) {
        return;
    }

    g_MenuHintButtonsVisible = 1;
    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_TeamLogoOption = g_TeamLogoOption > 0 ? g_TeamLogoOption - 1 : 2;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_TeamLogoOption = g_TeamLogoOption < 2 ? g_TeamLogoOption + 1 : 0;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseTeamLogoOption();
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 2;
        g_MenuOverlayPattern = 2;
    }
}

static void UpdateTeamLogoSavePrompt(void *ot) {
    RunTimedDrawScript(g_TeamLogoScreenScript2, &g_UiScriptProgress2, 0);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 1) !=
        0) {
        if (g_PadPressed & PAD_CONFIRM) {
            if (g_MenuSubCursor != 0) {
                PlaySoundCue(2);
                GameMenuBusy = -2;
                g_MenuConfirmTimer = 0x23;
            } else {
                PlaySoundCue(3);
                GameMenuBusy = 0;
            }
        }
        if (g_PadPressed & PAD_CANCEL) {
            PlaySoundCue(3);
            GameMenuBusy = 0;
        }
        if ((g_PadPressed & PAD_LEFT) && g_MenuSubCursor == 0) {
            PlaySoundCue(1);
            g_MenuSubCursor = 1;
        }
        if ((g_PadPressed & PAD_RIGHT) && g_MenuSubCursor != 0) {
            PlaySoundCue(1);
            g_MenuSubCursor = 0;
        }
        DrawTeamLogoSaveButtons(ot, 0);
    }
    DrawTeamLogoCanvas(1, 0);
}

static void UpdateTeamLogoSaveCountdown(void *ot) {
    if (g_MenuConfirmTimer <= 0) {
        RunTimedDrawScript(g_TeamLogoScreenScript2, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 0);
        if (g_UiScriptProgress2 <= 0) {
            GameMenuBusy = 1;
            g_MenuOverlayPattern = 1;
        }
    } else {
        g_MenuConfirmTimer--;
        RunTimedDrawScript(g_TeamLogoScreenScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 1);
        DrawTeamLogoSaveButtons(ot, 1);
    }
    DrawTeamLogoCanvas(1, 0);
}

static void UpdateTeamLogoPainting(void) {
    RampTeamLogoCanvas(9, 0x15);
    if (RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 1) !=
        0) {
        if (g_PadPressed & PAD_START) {
            PlaySoundCue(3);
            ApplyCurrentSequenceAudio();
            GameMenuBusy = -4;
        }
        UpdateTeamLogoCanvas();
    }
    if (g_UiScriptProgress2 >= 8) {
        g_MenuHintButtonsVisible = 0;
    }
    DrawTeamLogoCanvas(1, 1);
}

static void UpdateTeamLogoPaintClosing(void) {
    RampTeamLogoCanvas(-13, -21);
    RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, -1);
    DrawTeamLogoCanvas(1, -1);
    if (g_UiScriptProgress2 < 7) {
        g_MenuHintButtonsVisible = 1;
    }
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = 0;
    }
}

static void UpdateActiveTeamLogoModal(void *ot, s32 state) {
    switch (state) {
    case -1:
        UpdateTeamLogoSavePrompt(ot);
        break;
    case -2:
        UpdateTeamLogoSaveCountdown(ot);
        break;
    case -3:
        UpdateTeamLogoPainting();
        break;
    default:
        UpdateTeamLogoPaintClosing();
        break;
    }
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    RunTimedDrawScript(g_TeamLogoScreenScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void UpdateTeamLogoOutgoing(s32 state) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = 7;
    DrawTeamLogoCanvas(state == 2 ? -1 : 1, 0);
    RunTimedDrawScript(g_TeamLogoScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    if (g_UiScriptProgress > 0) {
        return;
    }

    switch (state) {
    case 1:
        g_MenuScreen = MENU_SCREEN_LOGO_SAMPLE;
        g_MenuHandlerIndex = MENU_SCREEN_LOGO_SAMPLE;
        DrawLogoSamplePanel(0, 0);
        break;

    case 2:
        g_MenuScreen = MENU_SCREEN_DESIGN_MODE;
        g_MenuHandlerIndex = MENU_SCREEN_DESIGN_MODE;
        g_TeamLogoOption = 0;
        g_TeamLogoClut[0] = 0;
        LoadImage(&g_TeamLogoClutRect, g_TeamLogoClut);
        break;

    default:
        break;
    }
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
}

void UpdateTeamLogoScreen(void) {
    s32 state = GameMenuBusy;

    g_MenuAltLayout = 0;
    if (state == 0) {
        UpdateTeamLogoIdle();
    } else if (state < 0) {
        UpdateActiveTeamLogoModal(RENDER_OT_BASE, state);
    } else {
        UpdateTeamLogoOutgoing(state);
    }
}
