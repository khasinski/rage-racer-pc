#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track.h"

enum {
    TRACK_SCENE_RENDER_OT_SHIFT = 5,
    TRACK_SCENE_FRAME_SYNC_THRESHOLD = 0x180,
};

void InitTrackScene(void) {
    if ((u32)g_CameraCarIndex >= RACE_CAR_SLOT_COUNT) {
        g_CameraCarIndex = 0;
    }

    InitRenderState(TRACK_SCENE_RENDER_OT_SHIFT);
    ApplyTrackTextureSectionRange();
    InitTrackLighting();
    BuildStartingGrid();
    SetTrackTexturePageNow(g_Cars[g_CameraCarIndex].trackSection);
    SeekEnvironmentScript(g_TrackRenderTable->environmentScriptOffset);
    g_CameraViewMode = CAMERA_VIEW_TRACK;
    g_AnimTimer = 0;
    g_SceneTimer = 0;
    g_FrameSyncThreshold = TRACK_SCENE_FRAME_SYNC_THRESHOLD;
    InitShuttleScenery();
}
