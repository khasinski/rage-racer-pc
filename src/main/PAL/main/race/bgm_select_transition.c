#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/track.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/scene.h"
#include "game/scene_runtime.h"

enum {
    BGM_SELECT_FADE_TPAGE = 0x49,
    BGM_SELECT_OPAQUE_FADE = 257,
    BGM_SELECT_FRAME_SYNC_THRESHOLD = 0x80,
    BGM_SELECT_INITIAL_FADE = 0x13C,
    BGM_SELECT_FADE_IN_STEP = -4,
    BGM_SELECT_FADE_OUT_STEP = 4,
    BGM_SELECT_DEFAULT_CURSOR = 1,
    BGM_SELECT_INITIAL_TRACK = 0,
    BGM_SELECT_INITIAL_CHANGE_DELAY = 30,
    BGM_SELECT_DISPLAY_ENABLE_FRAME = 15,
};

void EnterBgmSelectScreen(void) {
    BgmSelect *state = SceneRuntimeBgmSelect();

    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = BGM_SELECT_FRAME_SYNC_THRESHOLD;
    g_FadeLevel = BGM_SELECT_INITIAL_FADE;
    g_FadeStep = BGM_SELECT_FADE_IN_STEP;
    g_SceneId = GAME_SCENE_BGM_SELECT;
    state->cursor = BGM_SELECT_DEFAULT_CURSOR;
    state->showUi = 1;
    state->cdTrack = BgmCdTrack(BGM_SELECT_INITIAL_TRACK);
    state->step = BGM_SELECT_STEP_LOAD_ASSETS;
    g_SceneTimer = 0;
    state->track = BGM_SELECT_INITIAL_TRACK;
    state->changeDelay = BGM_SELECT_INITIAL_CHANGE_DELAY;
    g_CdTrackEnded = 0;
    g_CameraCarIndex = 0;
}

static s32 AdvanceBgmSelectFade(void) {
    if (g_FadeStep == 0) {
        return 0;
    }

    g_FadeLevel = StepFade(
        g_FadeLevel, g_FadeStep, BGM_SELECT_OPAQUE_FADE);
    if (g_FadeLevel == 0 && g_FadeStep < 0) {
        g_FadeStep = 0;
    }
    DrawFullscreenFadeTile(g_FadeLevel, BGM_SELECT_FADE_TPAGE);
    /* A fade-in starts above the PSX opaque range and moves down. It must
     * never complete the transition: the track data requested by the loader
     * is still in flight. Only the subsequent positive fade reaches the
     * point where InitTrackScene may build the presentation world. */
    return g_FadeStep > 0 && g_FadeLevel >= BGM_SELECT_OPAQUE_FADE;
}

static void UpdateBgmSelectTransition(BgmSelect *state) {
    if (g_SceneTimer == BGM_SELECT_DISPLAY_ENABLE_FRAME) {
        SetDispMask(1);
    }

    if (AdvanceBgmSelectFade()) {
        SetDispMask(0);
        InitTrackScene();
        g_FadeStep = 0;
        g_FadeLevel = 0;
        state->step = BGM_SELECT_STEP_ACTIVE;
    }

}

void UpdateBgmSelectLoad(BgmSelect *state) {
    const AssetLoadTransaction *assets;
    const AssetLoadSpan *texturePack = NULL;

    if (AssetLoadCompletedSuccessfully()) {
        assets = SceneRuntimeActiveAssetResult();
        /* SELECT.BIN supplies audio only.  g_ImageBlockBuffer still marks the
         * texture-pack boundary inherited from OPTION.BIN, but it is not an
         * image in SELECT.BIN and must never be uploaded as one.  Entering
         * through Options instead supplies the named COURSE_TEXTURES result.
         */
        if (assets != NULL) {
            if (assets->request == ASSET_REQUEST_SELECT_BGM) {
                texturePack = &assets->payload.selectBgm.texturePack;
            } else if (assets->request == ASSET_REQUEST_COURSE_TEXTURES) {
                texturePack = &assets->payload.race.texturePack;
            }
        }
        if (texturePack == NULL || texturePack->data == NULL ||
            texturePack->size == 0 ||
            !InstallTrackTextureAssetPack(
                texturePack->data, texturePack->size)) {
            FailAssetLoad();
        } else {
            RequestTrackDataAssets();
            state->step = BGM_SELECT_STEP_FADE_IN;
        }
    }
    UpdateBgmSelectTransition(state);
}

void UpdateBgmSelectFadeIn(BgmSelect *state) {
    if (AssetLoadCompletedSuccessfully()) {
        g_FadeStep = BGM_SELECT_FADE_OUT_STEP;
    }
    UpdateBgmSelectTransition(state);
}

void ExitBgmSelect(void) {
    if (AssetLoadCompletedSuccessfully()) {
        g_FadeStep = BGM_SELECT_FADE_OUT_STEP;
    }

    if (AdvanceBgmSelectFade()) {
        g_SceneId = GAME_SCENE_ENTER_ATTRACT;
    }

}
