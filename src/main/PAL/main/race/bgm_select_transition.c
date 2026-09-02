#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/state.h"

enum {
    BGM_SELECT_FADE_TPAGE = 0x49,
    BGM_SELECT_OPAQUE_FADE = 257,
    OPTION_SCENE_ID = 0x16,
};

void EnterBgmSelectScreen(void) {
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = 0x80;
    g_FadeLevel = 0x13C;
    g_FadeStep = -4;
    g_SceneId = 0x1C;
    g_BgmSelectCursor = 1;
    g_BgmSelectShowUi = 1;
    g_BgmSelectCdTrack = 3;
    g_BgmSelectStep = BGM_SELECT_STEP_LOAD_ASSETS;
    g_SceneTimer = 0;
    g_BgmSelectTrack = 0;
    g_BgmChangeDelay = 0x1E;
    g_CdTrackEnded = 0;
    g_CameraCarIndex = 0;
}

static s32 AdvanceBgmSelectFade(void) {
    if (g_FadeStep == 0) {
        return 0;
    }

    g_FadeLevel += g_FadeStep;
    if (g_FadeLevel < 0) {
        g_FadeLevel = 0;
        g_FadeStep = 0;
    }
    DrawFullscreenFadeTile(g_FadeLevel, BGM_SELECT_FADE_TPAGE);
    return g_FadeLevel >= BGM_SELECT_OPAQUE_FADE;
}

static void UpdateBgmSelectTransition(void) {
    if (g_SceneTimer == 0xF) {
        SetDispMask(1);
    }

    if (AdvanceBgmSelectFade()) {
        SetDispMask(0);
        InitTrackScene();
        g_FadeStep = 0;
        g_FadeLevel = 0;
        g_BgmSelectStep = BGM_SELECT_STEP_ACTIVE;
    }

    DrawProportionalText(0x5E, 0x72, g_TextNowLoading, 0x7812);
}

void UpdateBgmSelectLoad(void) {
    if (g_AssetLoadState == 0) {
        InstallCourseAssets();
        RequestTrackDataAssets();
        g_BgmSelectStep = BGM_SELECT_STEP_FADE_IN;
    }
    UpdateBgmSelectTransition();
}

void UpdateBgmSelectFadeIn(void) {
    if (g_AssetLoadState == 0) {
        g_FadeStep = 4;
    }
    UpdateBgmSelectTransition();
}

void ExitBgmSelect(void) {
    if (g_AssetLoadState == 0) {
        g_FadeStep = 4;
    }

    if (AdvanceBgmSelectFade()) {
        g_SceneId = OPTION_SCENE_ID;
    }

    DrawProportionalText(0x5E, 0x72, g_TextNowLoading, 0x7812);
}
