#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track.h"

void InitTrackScene(void) {
    InitRenderState(5);
    LoadTrackTexturePageRange();
    InitTrackLighting();
    g_TrackWalkStart = g_TrackEventData->trackWalkStart;
    BuildStartingGrid();
    SetTrackTexturePageNow(g_Cars[g_CameraCarIndex].trackSection);
    SeekEnvironmentScript(g_TrackRenderTable->environmentScriptOffset);
    g_CameraViewMode = CAMERA_VIEW_TRACK;
    g_AnimTimer = 0;
    g_SceneTimer = 0;
    g_FrameSyncThreshold = 0x180;
    InitShuttleScenery();
}
