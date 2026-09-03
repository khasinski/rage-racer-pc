#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/track.h"

void UpdateAndDrawAttractWorld(void) {
    s32 cameraCarIndex = g_CameraCarIndex;
    GameCarRuntime *cameraCar;

    if ((u32)cameraCarIndex >= RACE_CAR_SLOT_COUNT) {
        cameraCarIndex = 0;
        g_CameraCarIndex = cameraCarIndex;
    }
    UpdateAttractCars();
    cameraCar = &g_Cars[cameraCarIndex];
    RequestTrackTexturePage(cameraCar->trackSection);
    UpdateCamera(g_CameraViewMode, GetCarRenderObject(cameraCar));

    DrawCars();
    UpdateEnvironment();
    DrawSkyBackground();
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    DrawCourseObjects();
    DrawPresentationCourseScenery(g_AnimTimer, 1);
}
