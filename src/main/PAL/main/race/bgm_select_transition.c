#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/track.h"
#include "game/render_internal.h"
#include "game/state.h"

enum {
    BGM_SELECT_FADE_TPAGE = 0x49,
    BGM_SELECT_OPAQUE_FADE = 257,
    BGM_SELECT_FRAME_SYNC_THRESHOLD = 0x80,
    BGM_SELECT_INITIAL_FADE = 0x13C,
    BGM_SELECT_FADE_IN_STEP = -4,
    BGM_SELECT_FADE_OUT_STEP = 4,
    BGM_SELECT_SCENE_ID = 0x1C,
    BGM_SELECT_DEFAULT_CURSOR = 1,
    BGM_SELECT_INITIAL_TRACK = 0,
    BGM_SELECT_INITIAL_CHANGE_DELAY = 30,
    BGM_SELECT_DISPLAY_ENABLE_FRAME = 15,
    OPTION_SCENE_ID = 0x16,
};

void EnterBgmSelectScreen(void) {
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = BGM_SELECT_FRAME_SYNC_THRESHOLD;
    g_FadeLevel = BGM_SELECT_INITIAL_FADE;
    g_FadeStep = BGM_SELECT_FADE_IN_STEP;
    g_SceneId = BGM_SELECT_SCENE_ID;
    g_BgmSelectCursor = BGM_SELECT_DEFAULT_CURSOR;
    g_BgmSelectShowUi = 1;
    g_BgmSelectCdTrack = BgmCdTrack(BGM_SELECT_INITIAL_TRACK);
    g_BgmSelectStep = BGM_SELECT_STEP_LOAD_ASSETS;
    g_SceneTimer = 0;
    g_BgmSelectTrack = BGM_SELECT_INITIAL_TRACK;
    g_BgmChangeDelay = BGM_SELECT_INITIAL_CHANGE_DELAY;
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
    if (g_SceneTimer == BGM_SELECT_DISPLAY_ENABLE_FRAME) {
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
        g_FadeStep = BGM_SELECT_FADE_OUT_STEP;
    }
    UpdateBgmSelectTransition();
}

void ExitBgmSelect(void) {
    if (g_AssetLoadState == 0) {
        g_FadeStep = BGM_SELECT_FADE_OUT_STEP;
    }

    if (AdvanceBgmSelectFade()) {
        g_SceneId = OPTION_SCENE_ID;
    }

    DrawProportionalText(0x5E, 0x72, g_TextNowLoading, 0x7812);
}
