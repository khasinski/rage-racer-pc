#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"

enum {
    BGM_SELECT_DISPLAY_ENABLE_FRAME = 2,
    BGM_SELECT_FADE_TPAGE = 0x49,
    BGM_SELECT_OPAQUE_FADE = 256,
    BGM_SELECT_EXIT_FADE_STEP = -4,
    BGM_SELECT_CAMERA_MASK = 0xFF,
};

void UpdateBgmSelect(void) {
    UpdateBgmSelectPlayback();

    if (g_SceneTimer == BGM_SELECT_DISPLAY_ENABLE_FRAME) {
        SetDispMask(1);
    }
    if (g_FadeStep == 0) {
        UpdateBgmSelectInput();
    } else {
        g_FadeLevel = StepBgmSelectFade(
            g_FadeLevel, 0, BGM_SELECT_OPAQUE_FADE);
        DrawFullscreenFadeTile(g_FadeLevel, BGM_SELECT_FADE_TPAGE);
        g_FadeLevel = StepBgmSelectFade(
            g_FadeLevel, g_FadeStep, BGM_SELECT_OPAQUE_FADE);
        if (g_FadeLevel >= BGM_SELECT_OPAQUE_FADE) {
            RequestOptionScreenAssets();
            g_BgmSelectStep = BGM_SELECT_STEP_EXIT;
            g_FadeLevel = BGM_SELECT_OPAQUE_FADE;
            g_FadeStep = BGM_SELECT_EXIT_FADE_STEP;
        }
    }

    if (g_BgmSelectShowUi != 0) {
        UpdateBgmSelectBar();
        DrawBgmSelectBar();
    }
    g_AnimTimer = (s32)((u32)g_AnimTimer + 1);
    g_CameraCarIndex =
        CycleBgmSelectCameraCar(BGM_SELECT_CAMERA_MASK, g_CameraCarIndex);
    UpdateAndDrawAttractWorld();
}

void UpdateBgmSelectScene(void) {
    g_SceneTimer = NextBgmSelectTimer(g_SceneTimer);

    switch (g_BgmSelectStep) {
    case BGM_SELECT_STEP_INVALID:
        break;
    case BGM_SELECT_STEP_LOAD_ASSETS:
        UpdateBgmSelectLoad();
        break;
    case BGM_SELECT_STEP_FADE_IN:
        UpdateBgmSelectFadeIn();
        break;
    case BGM_SELECT_STEP_ACTIVE:
        UpdateBgmSelect();
        break;
    case BGM_SELECT_STEP_EXIT:
        ExitBgmSelect();
        break;
    }
}
