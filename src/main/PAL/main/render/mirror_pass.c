#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

void ResetMirrorState(void) {
    g_MirrorViewEnabled = 1;
    g_MirrorPanelY = -0x2C;
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
    s32 visibleHeight = panelY + 0x24;

    g_FrameContexts[0].environment.mirrorDraw.clip.y =
        panelY > 0 ? panelY : 0;
    g_FrameContexts[1].environment.mirrorDraw.clip.y =
        panelY > 0 ? panelY + 0xF0 : 0xF0;

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

    SetGeomOffset(0xA0, 0x24);
    SetGeomScreen(0xC0);

    state->mode = 9;
    /* Retail state+0x6c was shared by the mirror mode and terrain LOD shift. */
    state->faceOtShift = 9;
    state->x0 = 0x56;
    state->y0 = (s16)g_MirrorPanelY;
    state->x1 = 0xEA;
    state->y1 = (s16)(g_MirrorPanelY + 0x24);
    state->primData =
        &g_DrawBuffer->layout.orderingTables[1][0];
    state->orderingFlag ^= 1;

    SetMirrorClip(g_MirrorPanelY);
    g_VisibleCellMask = g_MirrorVisibleCellMask;
    g_VisibleCellList = g_MirrorVisibleCellList;
    state->depth += 0x800;
    return 1;
}

void EndMirrorPass(void) {
    GameRenderState *state = &g_RenderState;

    SetGeomOffset(0xA0, 0x78);
    SetGeomScreen(0x140);

    state->mode = 0xA;
    state->faceOtShift = 0xA;
    state->x0 = 0;
    state->y0 = 0;
    state->x1 = 0x140;
    state->y1 = 0xF0;
    state->primData =
        &g_DrawBuffer->layout.orderingTables[0][0];
    state->depth -= 0x800;
    state->orderingFlag ^= 1;
    state->matrix = g_CameraMatrixSaved;
    g_VisibleCellMask = g_MainVisibleCellMask;
    g_VisibleCellList = g_MainVisibleCellList;
}
