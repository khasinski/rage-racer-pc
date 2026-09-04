#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/team_logo.h"

enum LogoSampleScreenState {
    LOGO_SAMPLE_IDLE = 0,
    LOGO_SAMPLE_EXIT_TO_EDITOR = 1,
    LOGO_SAMPLE_PICK_CHARACTER = -1,
    LOGO_SAMPLE_PICK_BACKGROUND = -2,
};

enum LogoSampleOption {
    LOGO_SAMPLE_OPTION_CHARACTER,
    LOGO_SAMPLE_OPTION_BACKGROUND,
    LOGO_SAMPLE_OPTION_EXIT,
    LOGO_SAMPLE_OPTION_COUNT,
};

static void ChooseLogoSampleRow(void) {
    switch (g_LogoSampleCursor) {
    case LOGO_SAMPLE_OPTION_CHARACTER:
        PlaySoundCue(2);
        GameMenuBusy = LOGO_SAMPLE_PICK_CHARACTER;
        g_UiScriptProgress2 = 0;
        g_LogoSampleSubPanelScript = g_MenuRow0MarkerScript;
        g_LogoSampleSavedIndex = g_LogoSampleCharIndex;
        break;
    case LOGO_SAMPLE_OPTION_BACKGROUND:
        PlaySoundCue(2);
        GameMenuBusy = LOGO_SAMPLE_PICK_BACKGROUND;
        g_UiScriptProgress2 = 0;
        g_LogoSampleSubPanelScript = g_MenuRow1MarkerScript;
        g_LogoSampleSavedIndex = g_LogoSampleBackIndex;
        break;
    case LOGO_SAMPLE_OPTION_EXIT:
        PlaySoundCue(3);
        GameMenuBusy = LOGO_SAMPLE_EXIT_TO_EDITOR;
        g_MenuOverlayPattern = 2;
        break;
    }
}

static void UpdateLogoSampleIdle(void) {
    RampTeamLogoCanvas(-10, 0);
    DrawLogoSamplePanel(-1, g_LogoSampleSavedIndex + 1);
    RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
    RunTimedDrawScript(g_LogoSampleScreenScript, &g_UiScriptProgress, 0);
    if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) == 0 ||
        g_UiScriptProgress2 > 0) {
        return;
    }

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_LogoSampleCursor = WrapMenuIndex(
            g_LogoSampleCursor, -1, LOGO_SAMPLE_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_LogoSampleCursor = WrapMenuIndex(
            g_LogoSampleCursor, 1, LOGO_SAMPLE_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseLogoSampleRow();
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = LOGO_SAMPLE_EXIT_TO_EDITOR;
        g_MenuOverlayPattern = 2;
    }
}

static void UpdateLogoSamplePicker(s32 *selection) {
    MenuDialogAction action;

    if (RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2,
                           1) == 0) {
        return;
    }

    action = ChooseMenuDialogAction(g_PadPressed);
    if (action == MENU_DIALOG_CONFIRM) {
        PlaySoundCue(2);
        GameMenuBusy = 0;
        g_LogoSampleSavedIndex = *selection;
    } else if (action == MENU_DIALOG_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 0;
        *selection = g_LogoSampleSavedIndex;
    } else if (action == MENU_DIALOG_LEFT) {
        PlaySoundCue(1);
        *selection = WrapMenuIndex(*selection, -1,
                                   TEAM_LOGO_SAMPLE_CHOICE_COUNT);
    } else if (action == MENU_DIALOG_RIGHT) {
        PlaySoundCue(1);
        *selection = WrapMenuIndex(*selection, 1,
                                   TEAM_LOGO_SAMPLE_CHOICE_COUNT);
    }
}

static void UpdateLogoSampleModal(s32 state) {
    s32 *selection = state == LOGO_SAMPLE_PICK_CHARACTER
                         ? &g_LogoSampleCharIndex
                         : &g_LogoSampleBackIndex;

    RampTeamLogoCanvas(10, 0);
    UpdateLogoSamplePicker(selection);
    DrawLogoSamplePanel(1, *selection + 1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
    RunTimedDrawScript(g_LogoSampleScreenScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void UpdateLogoSampleOutgoing(void) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_LOGO_SAMPLE;
    DrawLogoSamplePanel(-1, 0);
    RunTimedDrawScript(g_LogoSampleScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_LogoSampleCursor);
    if (g_UiScriptProgress <= 0) {
        g_MenuScreen = MENU_SCREEN_TEAM_LOGO;
        g_MenuHandlerIndex = MENU_SCREEN_TEAM_LOGO;
        g_LogoSampleCursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}

void UpdateLogoSampleScreen(void) {
    s32 state = GameMenuBusy;

    g_LogoSampleCursor = AddClampedMenuValue(
        g_LogoSampleCursor, 0, 0, LOGO_SAMPLE_OPTION_COUNT - 1);
    g_LogoSampleCharIndex = AddClampedMenuValue(
        g_LogoSampleCharIndex, 0, 0, TEAM_LOGO_SAMPLE_CHOICE_COUNT - 1);
    g_LogoSampleBackIndex = AddClampedMenuValue(
        g_LogoSampleBackIndex, 0, 0, TEAM_LOGO_SAMPLE_CHOICE_COUNT - 1);
    g_LogoSampleSavedIndex = AddClampedMenuValue(
        g_LogoSampleSavedIndex, 0, 0, TEAM_LOGO_SAMPLE_CHOICE_COUNT - 1);
    g_MenuAltLayout = 0;
    ComposeSampleTeamLogo(g_LogoSampleCharIndex, g_LogoSampleBackIndex);
    DrawTeamLogoCanvas(1, 0);

    if (state == LOGO_SAMPLE_IDLE) {
        UpdateLogoSampleIdle();
    } else if (state == LOGO_SAMPLE_PICK_CHARACTER ||
               state == LOGO_SAMPLE_PICK_BACKGROUND) {
        UpdateLogoSampleModal(state);
    } else if (state > LOGO_SAMPLE_IDLE) {
        UpdateLogoSampleOutgoing();
    } else {
        GameMenuBusy = LOGO_SAMPLE_IDLE;
    }
}

s32 DrawLogoSampleScreen(s32 step) {
    return AdvanceMenuFade(&g_LogoSampleScreenFade, step);
}
