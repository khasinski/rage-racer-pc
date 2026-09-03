#include "common.h"
#include "game/car.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
s32 g_CameraCarIndex;
CameraViewMode g_CameraViewMode;
s32 g_AnimTimer;
s32 g_SceneTimer;
s32 g_FrameSyncThreshold;
s32 g_TrackTextureSectionLo;
s32 g_TrackTextureSectionHi;
const TrackRenderTable *g_TrackRenderTable;

static s32 s_order;
static s32 s_renderOrder;
static s32 s_lightingOrder;
static s32 s_gridOrder;
static s32 s_textureOrder;
static s32 s_environmentOrder;
static s32 s_shuttleOrder;
static s32 s_renderShift;
static s32 s_textureSection;
static s32 s_environmentOffset;

void InitRenderState(s32 otShift) {
    s_renderOrder = ++s_order;
    s_renderShift = otShift;
}
void InitTrackLighting(void) { s_lightingOrder = ++s_order; }
void BuildStartingGrid(void) { s_gridOrder = ++s_order; }
void SetTrackTexturePageNow(s32 section) {
    s_textureOrder = ++s_order;
    s_textureSection = section;
}
void SeekEnvironmentScript(s32 offset) {
    s_environmentOrder = ++s_order;
    s_environmentOffset = offset;
}
void InitShuttleScenery(void) { s_shuttleOrder = ++s_order; }

int main(void) {
    TrackRenderTable renderTable;

    memset(&renderTable, 0, sizeof(renderTable));
    memset(g_Cars, 0, sizeof(g_Cars));
    renderTable.textureSectionLo = 12;
    renderTable.textureSectionHi = 34;
    renderTable.environmentScriptOffset = 567;
    g_TrackRenderTable = &renderTable;
    g_CameraCarIndex = 3;
    g_Cars[3].trackSection = 89;
    g_AnimTimer = 111;
    g_SceneTimer = 222;
    g_FrameSyncThreshold = 333;

    InitTrackScene();

    if (s_renderOrder != 1 || s_lightingOrder != 2 || s_gridOrder != 3 ||
        s_textureOrder != 4 || s_environmentOrder != 5 ||
        s_shuttleOrder != 6 || s_renderShift != 5 ||
        g_TrackTextureSectionLo != 12 || g_TrackTextureSectionHi != 34 ||
        s_textureSection != 89 || s_environmentOffset != 567 ||
        g_CameraViewMode != CAMERA_VIEW_TRACK || g_AnimTimer != 0 ||
        g_SceneTimer != 0 || g_FrameSyncThreshold != 0x180) {
        puts("FAIL: track scene initialization contract changed");
        return 1;
    }

    puts("track scene initializes rendering, assets, and scenery in order");
    return 0;
}
