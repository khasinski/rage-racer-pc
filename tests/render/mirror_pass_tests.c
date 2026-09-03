#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
GameFrameContext g_FrameContexts[2];
Matrix g_CameraMatrixSaved;
Matrix g_MirrorViewMatrix;
u32 g_MainVisibleCellMask[1];
VisibleTerrainCell g_MainVisibleCellList[1];
u32 g_MirrorVisibleCellMask[1];
VisibleTerrainCell g_MirrorVisibleCellList[1];
u32 *g_VisibleCellMask;
VisibleTerrainCell *g_VisibleCellList;
CameraViewMode g_CameraViewMode;
GameFrameContext *g_DrawBuffer;
s16 g_GrandPrixMode;
s16 g_RacePhase;
s16 g_MirrorViewEnabled;
s32 g_MirrorPanelY;
s32 g_MirrorUnlocked;

static long s_geomX;
static long s_geomY;
static long s_geomScreen;

void SetGeomOffset(long x, long y) {
    s_geomX = x;
    s_geomY = y;
}

void SetGeomScreen(long distance) {
    s_geomScreen = distance;
}

static void SetAvailable(s32 panelY) {
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(g_FrameContexts, 0, sizeof(g_FrameContexts));
    memset(&g_CameraMatrixSaved, 0, sizeof(g_CameraMatrixSaved));
    memset(&g_MirrorViewMatrix, 0x35, sizeof(g_MirrorViewMatrix));
    g_DrawBuffer = &g_FrameContexts[0];
    g_MirrorUnlocked = 1;
    g_MirrorViewEnabled = 1;
    g_CameraViewMode = CAMERA_VIEW_CAR;
    g_GrandPrixMode = 1;
    g_RacePhase = 2;
    g_MirrorPanelY = panelY;
    g_RenderState.depth = 100;
    g_RenderState.orderingFlag = 1;
}

static int TestAvailability(void) {
    GameRenderState original;

    SetAvailable(0);
    original = g_RenderState;
    g_MirrorUnlocked = 0;
    if (BeginMirrorPass() != 0 ||
        memcmp(&g_RenderState, &original, sizeof(original)) != 0) return 0;
    g_MirrorUnlocked = 1;
    g_MirrorViewEnabled = 0;
    if (BeginMirrorPass() != 0 ||
        memcmp(&g_RenderState, &original, sizeof(original)) != 0) return 0;
    g_MirrorViewEnabled = 1;
    g_CameraViewMode = CAMERA_VIEW_CHASE;
    if (BeginMirrorPass() != 0 ||
        memcmp(&g_RenderState, &original, sizeof(original)) != 0) return 0;
    g_CameraViewMode = CAMERA_VIEW_CAR;
    g_GrandPrixMode = 0;
    if (BeginMirrorPass() != 0 ||
        memcmp(&g_RenderState, &original, sizeof(original)) != 0) return 0;
    g_GrandPrixMode = 1;
    g_RacePhase = 1;
    return BeginMirrorPass() == 0 &&
           memcmp(&g_RenderState, &original, sizeof(original)) == 0;
}

static int TestHiddenPanelClip(void) {
    SetAvailable(-20);
    if (BeginMirrorPass() != 1) return 0;
    return g_RenderState.mode == GAME_RENDER_PASS_MIRROR &&
           g_RenderState.faceOtShift == GAME_RENDER_PASS_MIRROR &&
           g_RenderState.x0 == 0x56 && g_RenderState.y0 == -20 &&
           g_RenderState.x1 == 0xEA && g_RenderState.y1 == 16 &&
           g_RenderState.depth == 100 + 0x800 &&
           g_RenderState.orderingFlag == 0 &&
           g_FrameContexts[0].environment.mirrorDraw.clip.y == 0 &&
           g_FrameContexts[1].environment.mirrorDraw.clip.y == 0xF0 &&
           g_FrameContexts[0].environment.mirrorDraw.clip.h == 16 &&
           g_FrameContexts[1].environment.mirrorDraw.clip.h == 16 &&
           g_VisibleCellMask == g_MirrorVisibleCellMask &&
           g_VisibleCellList == g_MirrorVisibleCellList &&
           s_geomX == 0xA0 && s_geomY == 0x24 && s_geomScreen == 0xC0;
}

static int TestVisiblePanelAndRestore(void) {
    Matrix original;

    SetAvailable(10);
    memset(&g_RenderState.matrix, 0x72, sizeof(g_RenderState.matrix));
    original = g_RenderState.matrix;
    if (BeginMirrorPass() != 1 ||
        g_FrameContexts[0].environment.mirrorDraw.clip.y != 10 ||
        g_FrameContexts[1].environment.mirrorDraw.clip.y != 250 ||
        g_FrameContexts[0].environment.mirrorDraw.clip.h != 0x24) {
        return 0;
    }
    EndMirrorPass();
    return g_RenderState.mode == GAME_RENDER_PASS_MAIN &&
           g_RenderState.faceOtShift == GAME_RENDER_PASS_MAIN &&
           g_RenderState.x0 == 0 && g_RenderState.y0 == 0 &&
           g_RenderState.x1 == 0x140 && g_RenderState.y1 == 0xF0 &&
           g_RenderState.depth == 100 && g_RenderState.orderingFlag == 1 &&
           memcmp(&g_RenderState.matrix, &original, sizeof(original)) == 0 &&
           g_VisibleCellMask == g_MainVisibleCellMask &&
           g_VisibleCellList == g_MainVisibleCellList &&
           s_geomX == 0xA0 && s_geomY == 0x78 && s_geomScreen == 0x140;
}

static int TestPanelClipLimits(void) {
    SetAvailable(-44);
    if (BeginMirrorPass() != 1 ||
        g_FrameContexts[0].environment.mirrorDraw.clip.h != 0 ||
        g_FrameContexts[1].environment.mirrorDraw.clip.h != 0) {
        return 0;
    }
    EndMirrorPass();

    SetAvailable(18);
    return BeginMirrorPass() == 1 &&
           g_FrameContexts[0].environment.mirrorDraw.clip.y == 18 &&
           g_FrameContexts[1].environment.mirrorDraw.clip.y == 258 &&
           g_FrameContexts[0].environment.mirrorDraw.clip.h == 36 &&
           g_FrameContexts[1].environment.mirrorDraw.clip.h == 36;
}

int main(void) {
    ResetMirrorState();
    if (g_MirrorViewEnabled != 1 || g_MirrorPanelY != -0x2C ||
        g_MirrorUnlocked != 0 || !TestAvailability() ||
        !TestHiddenPanelClip() || !TestVisiblePanelAndRestore() ||
        !TestPanelClipLimits()) {
        puts("mirror pass state failed");
        return 1;
    }
    puts("mirror pass state preserved");
    return 0;
}
