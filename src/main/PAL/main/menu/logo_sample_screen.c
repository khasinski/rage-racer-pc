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

static void ChooseLogoSampleRow(LogoSample *logo) {
    switch (logo->cursor) {
    case LOGO_SAMPLE_OPTION_CHARACTER:
        PlaySoundCue(2);
        GameMenuBusy = LOGO_SAMPLE_PICK_CHARACTER;
        g_UiScriptProgress2 = 0;
        logo->subPanelScript = g_MenuRow0MarkerScript;
        logo->saved = logo->character;
        break;
    case LOGO_SAMPLE_OPTION_BACKGROUND:
        PlaySoundCue(2);
        GameMenuBusy = LOGO_SAMPLE_PICK_BACKGROUND;
        g_UiScriptProgress2 = 0;
        logo->subPanelScript = g_MenuRow1MarkerScript;
        logo->saved = logo->background;
        break;
    case LOGO_SAMPLE_OPTION_EXIT:
        PlaySoundCue(3);
        GameMenuBusy = LOGO_SAMPLE_EXIT_TO_EDITOR;
        g_MenuOverlayPattern = 2;
        break;
    }
}

static void UpdateLogoSampleIdle(LogoSample *logo) {
    RampTeamLogoCanvas(MenuTeamLogo(), -10, 0);
    DrawLogoSamplePanel(logo, -1, logo->saved + 1);
    RunTimedDrawScript(logo->subPanelScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, logo->cursor);
    RunTimedDrawScript(g_LogoSampleScreenScript, &g_UiScriptProgress, 0);
    if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) == 0 ||
        g_UiScriptProgress2 > 0) {
        return;
    }

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        logo->cursor = WrapMenuIndex(
            logo->cursor, -1, LOGO_SAMPLE_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        logo->cursor = WrapMenuIndex(
            logo->cursor, 1, LOGO_SAMPLE_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseLogoSampleRow(logo);
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = LOGO_SAMPLE_EXIT_TO_EDITOR;
        g_MenuOverlayPattern = 2;
    }
}

static void UpdateLogoSamplePicker(LogoSample *logo, s32 *selection) {
    MenuDialogAction action;

    if (RunTimedDrawScript(logo->subPanelScript, &g_UiScriptProgress2,
                           1) == 0) {
        return;
    }

    action = ChooseMenuDialogAction(g_PadPressed);
    if (action == MENU_DIALOG_CONFIRM) {
        PlaySoundCue(2);
        GameMenuBusy = 0;
        logo->saved = *selection;
    } else if (action == MENU_DIALOG_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 0;
        *selection = logo->saved;
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

static void UpdateLogoSampleModal(LogoSample *logo, s32 state) {
    s32 *selection = state == LOGO_SAMPLE_PICK_CHARACTER
                         ? &logo->character
                         : &logo->background;

    RampTeamLogoCanvas(MenuTeamLogo(), 10, 0);
    UpdateLogoSamplePicker(logo, selection);
    DrawLogoSamplePanel(logo, 1, *selection + 1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, logo->cursor);
    RunTimedDrawScript(g_LogoSampleScreenScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void UpdateLogoSampleOutgoing(LogoSample *logo) {
    MenuBeginExit(MENU_SCREEN_LOGO_SAMPLE);
    DrawLogoSamplePanel(logo, -1, 0);
    RunTimedDrawScript(g_LogoSampleScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, logo->cursor);
    if (g_UiScriptProgress <= 0) {
        MenuActivateScreen(MENU_SCREEN_TEAM_LOGO);
        logo->cursor = 0;
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}

void UpdateLogoSampleScreen(void) {
    LogoSample *logo = MenuLogoSample();
    s32 state = GameMenuBusy;

    logo->cursor = AddClampedMenuValue(
        logo->cursor, 0, 0, LOGO_SAMPLE_OPTION_COUNT - 1);
    logo->character = AddClampedMenuValue(
        logo->character, 0, 0, TEAM_LOGO_SAMPLE_CHOICE_COUNT - 1);
    logo->background = AddClampedMenuValue(
        logo->background, 0, 0, TEAM_LOGO_SAMPLE_CHOICE_COUNT - 1);
    logo->saved = AddClampedMenuValue(
        logo->saved, 0, 0, TEAM_LOGO_SAMPLE_CHOICE_COUNT - 1);
    g_MenuAltLayout = 0;
    ComposeSampleTeamLogo(logo->character, logo->background);
    DrawTeamLogoCanvas(MenuTeamLogo(), 1, 0);

    if (state == LOGO_SAMPLE_IDLE) {
        UpdateLogoSampleIdle(logo);
    } else if (state == LOGO_SAMPLE_PICK_CHARACTER ||
               state == LOGO_SAMPLE_PICK_BACKGROUND) {
        UpdateLogoSampleModal(logo, state);
    } else if (state > LOGO_SAMPLE_IDLE) {
        UpdateLogoSampleOutgoing(logo);
    } else {
        GameMenuBusy = LOGO_SAMPLE_IDLE;
    }
}
