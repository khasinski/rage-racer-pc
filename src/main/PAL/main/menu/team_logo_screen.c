#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/team_logo.h"

typedef enum TeamLogoScreenState {
    TEAM_LOGO_IDLE = 0,
    TEAM_LOGO_EXIT_TO_SAMPLES = 1,
    TEAM_LOGO_EXIT_TO_DESIGN = 2,
    TEAM_LOGO_SAVE_PROMPT = -1,
    TEAM_LOGO_SAVE_COUNTDOWN = -2,
    TEAM_LOGO_PAINTING = -3,
    TEAM_LOGO_PAINT_CLOSING = -4,
} TeamLogoScreenState;

enum TeamLogoOption {
    TEAM_LOGO_OPTION_SAMPLES,
    TEAM_LOGO_OPTION_PAINT,
    TEAM_LOGO_OPTION_EXIT,
    TEAM_LOGO_OPTION_COUNT,
};

enum { TEAM_LOGO_SAVE_CONFIRM_FRAMES = 35 };

s32 DrawTeamLogoScreen(s32 step) {
    return AdvanceMenuFade(&g_TeamLogoScreenFade, step);
}

static void DrawTeamLogoSaveButtons(GameOrderingTableEntry *ot, s32 flash) {
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
    case TEAM_LOGO_OPTION_SAMPLES:
        PlaySoundCue(2);
        GameMenuBusy = TEAM_LOGO_SAVE_PROMPT;
        g_MenuSubCursor = 0;
        g_UiScriptProgress2 = 0;
        g_TeamLogoSubPanelScript = g_MenuDialogPanelUpperScript;
        break;
    case TEAM_LOGO_OPTION_PAINT:
        PlaySoundCue(2);
        ApplyDuckedSequenceAudio();
        GameMenuBusy = TEAM_LOGO_PAINTING;
        g_TeamLogoPaintArmed = 0;
        g_UiScriptProgress2 = 0;
        g_TeamLogoSubPanelScript = g_MenuRow1MarkerScript;
        break;
    case TEAM_LOGO_OPTION_EXIT:
        PlaySoundCue(3);
        GameMenuBusy = TEAM_LOGO_EXIT_TO_DESIGN;
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
        g_TeamLogoOption = WrapMenuIndex(
            g_TeamLogoOption, -1, TEAM_LOGO_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_TeamLogoOption = WrapMenuIndex(
            g_TeamLogoOption, 1, TEAM_LOGO_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseTeamLogoOption();
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = TEAM_LOGO_EXIT_TO_DESIGN;
        g_MenuOverlayPattern = 2;
    }
}

static void UpdateTeamLogoSavePrompt(GameOrderingTableEntry *ot) {
    MenuDialogAction action;

    RunTimedDrawScript(g_TeamLogoScreenScript2, &g_UiScriptProgress2, 0);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 1) !=
        0) {
        action = ChooseMenuDialogAction(g_PadPressed);
        if (action == MENU_DIALOG_CONFIRM) {
            if (g_MenuSubCursor != 0) {
                PlaySoundCue(2);
                GameMenuBusy = TEAM_LOGO_SAVE_COUNTDOWN;
                g_MenuConfirmTimer = TEAM_LOGO_SAVE_CONFIRM_FRAMES;
            } else {
                PlaySoundCue(3);
                GameMenuBusy = 0;
            }
        } else if (action == MENU_DIALOG_CANCEL) {
            PlaySoundCue(3);
            GameMenuBusy = 0;
        } else if (action == MENU_DIALOG_LEFT && g_MenuSubCursor == 0) {
            PlaySoundCue(1);
            g_MenuSubCursor = 1;
        } else if (action == MENU_DIALOG_RIGHT && g_MenuSubCursor != 0) {
            PlaySoundCue(1);
            g_MenuSubCursor = 0;
        }
        DrawTeamLogoSaveButtons(ot, 0);
    }
    DrawTeamLogoCanvas(1, 0);
}

static void UpdateTeamLogoSaveCountdown(GameOrderingTableEntry *ot) {
    if (g_MenuConfirmTimer <= 0) {
        RunTimedDrawScript(g_TeamLogoScreenScript2, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_TeamLogoSubPanelScript, &g_UiScriptProgress2, 0);
        if (g_UiScriptProgress2 <= 0) {
            GameMenuBusy = TEAM_LOGO_EXIT_TO_SAMPLES;
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
            GameMenuBusy = TEAM_LOGO_PAINT_CLOSING;
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

static void UpdateActiveTeamLogoModal(GameOrderingTableEntry *ot,
                                      TeamLogoScreenState state) {
    switch (state) {
    case TEAM_LOGO_SAVE_PROMPT:
        UpdateTeamLogoSavePrompt(ot);
        break;
    case TEAM_LOGO_SAVE_COUNTDOWN:
        UpdateTeamLogoSaveCountdown(ot);
        break;
    case TEAM_LOGO_PAINTING:
        UpdateTeamLogoPainting();
        break;
    case TEAM_LOGO_PAINT_CLOSING:
        UpdateTeamLogoPaintClosing();
        break;
    default:
        break;
    }
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    RunTimedDrawScript(g_TeamLogoScreenScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void UpdateTeamLogoOutgoing(TeamLogoScreenState state) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_TEAM_LOGO;
    DrawTeamLogoCanvas(state == TEAM_LOGO_EXIT_TO_DESIGN ? -1 : 1, 0);
    RunTimedDrawScript(g_TeamLogoScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_TeamLogoOption);
    if (g_UiScriptProgress > 0) {
        return;
    }

    switch (state) {
    case TEAM_LOGO_EXIT_TO_SAMPLES:
        g_MenuScreen = MENU_SCREEN_LOGO_SAMPLE;
        g_MenuHandlerIndex = MENU_SCREEN_LOGO_SAMPLE;
        DrawLogoSamplePanel(0, 0);
        break;

    case TEAM_LOGO_EXIT_TO_DESIGN:
        g_MenuScreen = MENU_SCREEN_DESIGN_MODE;
        g_MenuHandlerIndex = MENU_SCREEN_DESIGN_MODE;
        g_TeamLogoOption = 0;
        g_TeamLogoClut[0] = 0;
        UploadTeamLogoClut();
        break;

    default:
        break;
    }
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
}

void UpdateTeamLogoScreen(void) {
    TeamLogoScreenState state = (TeamLogoScreenState)GameMenuBusy;

    g_TeamLogoOption = AddClampedMenuValue(
        g_TeamLogoOption, 0, 0, TEAM_LOGO_OPTION_COUNT - 1);
    g_MenuSubCursor = g_MenuSubCursor != 0;
    if (state == TEAM_LOGO_SAVE_COUNTDOWN) {
        g_MenuConfirmTimer = AddClampedMenuValue(
            g_MenuConfirmTimer, 0, 0, TEAM_LOGO_SAVE_CONFIRM_FRAMES);
    }
    g_MenuAltLayout = 0;
    if (state == TEAM_LOGO_IDLE) {
        UpdateTeamLogoIdle();
    } else if (state >= TEAM_LOGO_PAINT_CLOSING &&
               state <= TEAM_LOGO_SAVE_PROMPT) {
        UpdateActiveTeamLogoModal(RENDER_OT_BASE, state);
    } else if (state == TEAM_LOGO_EXIT_TO_SAMPLES ||
               state == TEAM_LOGO_EXIT_TO_DESIGN) {
        UpdateTeamLogoOutgoing(state);
    } else {
        GameMenuBusy = TEAM_LOGO_IDLE;
    }
}
