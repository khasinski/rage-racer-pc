#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track.h"

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
    UpdateAttractCars();
    RequestTrackTexturePage(g_Cars[g_CameraCarIndex].trackSection);
    UpdateCamera(g_CameraViewMode,
                 GetCarRenderObject(&g_Cars[g_CameraCarIndex]));
    DrawCars();
    UpdateEnvironment();
    DrawSkyBackground();
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    DrawCourseObjects();
    DrawCourseScenery2(g_AnimTimer, 1);
}
