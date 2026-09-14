#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/scene_runtime.h"

enum {
    BGM_SELECT_DISPLAY_ENABLE_FRAME = 2,
    BGM_SELECT_FADE_TPAGE = 0x49,
    BGM_SELECT_OPAQUE_FADE = 256,
    BGM_SELECT_EXIT_FADE_STEP = -4,
    BGM_SELECT_CAMERA_MASK = 0xFF,
};

void UpdateBgmSelect(BgmSelect *state) {
    UpdateBgmSelectPlayback(state);

    if (g_SceneTimer == BGM_SELECT_DISPLAY_ENABLE_FRAME) {
        SetDispMask(1);
    }
    if (g_FadeStep == 0) {
        UpdateBgmSelectInput(state);
    } else {
        g_FadeLevel = StepFade(
            g_FadeLevel, 0, BGM_SELECT_OPAQUE_FADE);
        DrawFullscreenFadeTile(g_FadeLevel, BGM_SELECT_FADE_TPAGE);
        g_FadeLevel = StepFade(
            g_FadeLevel, g_FadeStep, BGM_SELECT_OPAQUE_FADE);
        if (g_FadeLevel >= BGM_SELECT_OPAQUE_FADE) {
            RequestOptionScreenAssets();
            state->step = BGM_SELECT_STEP_EXIT;
            g_FadeLevel = BGM_SELECT_OPAQUE_FADE;
            g_FadeStep = BGM_SELECT_EXIT_FADE_STEP;
        }
    }

    if (state->showUi != 0) {
        UpdateBgmSelectBar(state);
        DrawBgmSelectBar(state);
    }
    g_AnimTimer = (s32)((u32)g_AnimTimer + 1);
    g_CameraCarIndex =
        CycleBgmSelectCameraCar(BGM_SELECT_CAMERA_MASK, g_CameraCarIndex);
    UpdateAndDrawAttractWorld();
}

void UpdateBgmSelectScene(void) {
    BgmSelect *state = SceneRuntimeBgmSelect();

    g_SceneTimer = NextBgmSelectTimer(g_SceneTimer);

    switch (state->step) {
    case BGM_SELECT_STEP_INVALID:
        break;
    case BGM_SELECT_STEP_LOAD_ASSETS:
        UpdateBgmSelectLoad(state);
        break;
    case BGM_SELECT_STEP_FADE_IN:
        UpdateBgmSelectFadeIn(state);
        break;
    case BGM_SELECT_STEP_ACTIVE:
        UpdateBgmSelect(state);
        break;
    case BGM_SELECT_STEP_EXIT:
        ExitBgmSelect();
        break;
    }
}
