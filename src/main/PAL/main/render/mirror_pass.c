#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

enum {
    MIRROR_X = 0x56,
    MIRROR_WIDTH = 0x94,
    MIRROR_HEIGHT = 0x24,
    MIRROR_PROJECTION_DISTANCE = 0xC0,
    MIRROR_DEPTH_BIAS = 0x800,
};

void ResetMirrorState(void) {
    g_MirrorViewEnabled = 1;
    g_MirrorPanelY = -44;
    g_MirrorUnlocked = 0;
}

static s32 MirrorPassIsAvailable(void) {
    return g_MirrorUnlocked != 0 &&
           g_MirrorViewEnabled != 0 &&
           g_CameraViewMode == CAMERA_VIEW_CAR &&
           g_GrandPrixMode != 0 &&
           g_RacePhase == 2;
}

static void SetMirrorClip(s32 panelY) {
    s32 visibleHeight = panelY + MIRROR_HEIGHT;

    g_FrameContexts[0].environment.mirrorDraw.clip.y =
        panelY > 0 ? panelY : 0;
    g_FrameContexts[1].environment.mirrorDraw.clip.y =
        panelY > 0 ? panelY + SCREEN_HEIGHT : SCREEN_HEIGHT;

    if (visibleHeight > 0) {
        visibleHeight -= g_FrameContexts[0].environment.mirrorDraw.clip.y;
    } else {
        visibleHeight = 0;
    }
    g_FrameContexts[0].environment.mirrorDraw.clip.h = visibleHeight;
    g_FrameContexts[1].environment.mirrorDraw.clip.h = visibleHeight;
}

s32 BeginMirrorPass(void) {
    GameRenderState *state;

    if (!MirrorPassIsAvailable()) {
        return 0;
    }

    state = &g_RenderState;
    g_CameraMatrixSaved = state->matrix;
    state->matrix = g_MirrorViewMatrix;

    SetGeomOffset(SCREEN_WIDTH / 2, MIRROR_HEIGHT);
    SetGeomScreen(MIRROR_PROJECTION_DISTANCE);

    state->mode = GAME_RENDER_PASS_MIRROR;
    /* Retail state+0x6c was shared by the mirror mode and terrain LOD shift. */
    state->faceOtShift = GAME_RENDER_PASS_MIRROR;
    state->x0 = MIRROR_X;
    state->y0 = (s16)g_MirrorPanelY;
    state->x1 = MIRROR_X + MIRROR_WIDTH;
    state->y1 = (s16)(g_MirrorPanelY + MIRROR_HEIGHT);
    state->primData =
        &g_DrawBuffer->layout.orderingTables[1][0];
    state->orderingFlag ^= 1;

    SetMirrorClip(g_MirrorPanelY);
    g_VisibleCellMask = g_MirrorVisibleCellMask;
    g_VisibleCellList = g_MirrorVisibleCellList;
    state->depth += MIRROR_DEPTH_BIAS;
    return 1;
}

void EndMirrorPass(void) {
    GameRenderState *state = &g_RenderState;

    SetGeomOffset(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    SetGeomScreen(SCREEN_WIDTH);

    state->mode = GAME_RENDER_PASS_MAIN;
    state->faceOtShift = GAME_RENDER_PASS_MAIN;
    state->x0 = 0;
    state->y0 = 0;
    state->x1 = SCREEN_WIDTH;
    state->y1 = SCREEN_HEIGHT;
    state->primData =
        &g_DrawBuffer->layout.orderingTables[0][0];
    state->depth -= MIRROR_DEPTH_BIAS;
    state->orderingFlag ^= 1;
    state->matrix = g_CameraMatrixSaved;
    g_VisibleCellMask = g_MainVisibleCellMask;
    g_VisibleCellList = g_MainVisibleCellList;
}
