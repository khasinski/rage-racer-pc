#include "game/audio.h"
#include "game/menu.h"
#include "game/team_logo.h"

enum { LOGO_SAMPLE_COUNT = 20 };

static s32 WrapLogoSampleIndex(s32 index, s32 step, s32 count) {
    index += step;
    if (index < 0) {
        return count - 1;
    }
    return index >= count ? 0 : index;
}

static void ChooseLogoSampleRow(void) {
    switch (g_LogoSampleCursor) {
    case 0:
        PlaySoundCue(2);
        GameMenuBusy = -1;
        g_UiScriptProgress2 = 0;
        g_LogoSampleSubPanelScript = g_MenuRow0MarkerScript;
        g_LogoSampleSavedIndex = g_LogoSampleCharIndex;
        break;
    case 1:
        PlaySoundCue(2);
        GameMenuBusy = -2;
        g_UiScriptProgress2 = 0;
        g_LogoSampleSubPanelScript = g_MenuRow1MarkerScript;
        g_LogoSampleSavedIndex = g_LogoSampleBackIndex;
        break;
    case 2:
        PlaySoundCue(3);
        GameMenuBusy = 1;
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
        g_LogoSampleCursor = WrapLogoSampleIndex(g_LogoSampleCursor, -1, 3);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_LogoSampleCursor = WrapLogoSampleIndex(g_LogoSampleCursor, 1, 3);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseLogoSampleRow();
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 1;
        g_MenuOverlayPattern = 2;
    }
}

static void UpdateLogoSamplePicker(s32 *selection) {
    if (RunTimedDrawScript(g_LogoSampleSubPanelScript, &g_UiScriptProgress2,
                           1) == 0) {
        return;
    }

    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        GameMenuBusy = 0;
        g_LogoSampleSavedIndex = *selection;
    }
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 0;
        *selection = g_LogoSampleSavedIndex;
    }
    if (g_PadPressed & PAD_LEFT) {
        PlaySoundCue(1);
        *selection = WrapLogoSampleIndex(*selection, -1, LOGO_SAMPLE_COUNT);
    }
    if (g_PadPressed & PAD_RIGHT) {
        PlaySoundCue(1);
        *selection = WrapLogoSampleIndex(*selection, 1, LOGO_SAMPLE_COUNT);
    }
}

static void UpdateLogoSampleModal(void) {
    s32 *selection = GameMenuBusy == -1 ? &g_LogoSampleCharIndex
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

    g_MenuAltLayout = 0;
    ComposeSampleTeamLogo(g_LogoSampleCharIndex, g_LogoSampleBackIndex);
    DrawTeamLogoCanvas(1, 0);

    if (state == 0) {
        UpdateLogoSampleIdle();
    } else if (state < 0) {
        UpdateLogoSampleModal();
    } else {
        UpdateLogoSampleOutgoing();
    }
}

s32 DrawLogoSampleScreen(s32 step) {
    if (step == 0) {
        g_LogoSampleScreenFade = 0;
    } else {
        g_LogoSampleScreenFade += step;
        if (g_LogoSampleScreenFade >= MENU_FADE_COMPLETE) {
            g_LogoSampleScreenFade = MENU_FADE_MAX;
        } else if (g_LogoSampleScreenFade < 0) {
            g_LogoSampleScreenFade = 0;
        }
    }

    return g_LogoSampleScreenFade;
}
