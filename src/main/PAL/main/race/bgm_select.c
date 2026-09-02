#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"

void UpdateBgmSelect(void) {
    UpdateBgmSelectPlayback();

    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    if (g_FadeStep == 0) {
        UpdateBgmSelectInput();
    } else {
        DrawFullscreenFadeTile(g_FadeLevel, 0x49);
        g_FadeLevel += g_FadeStep;
        if (g_FadeLevel >= 256) {
            RequestOptionScreenAssets();
            g_BgmSelectStep = BGM_SELECT_STEP_EXIT;
            g_FadeLevel = 256;
            g_FadeStep = -4;
        }
    }

    if (g_BgmSelectShowUi != 0) {
        DrawBgmSelectBar();
    }
    g_AnimTimer++;
    g_CameraCarIndex = CycleBgmSelectCameraCar(0xff, g_CameraCarIndex);
    UpdateAndDrawAttractWorld();
}

void UpdateBgmSelectScene(void) {
    g_SceneTimer++;

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
