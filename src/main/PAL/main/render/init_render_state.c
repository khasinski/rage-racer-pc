#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"

void InitRenderState(s32 otShift) {
    g_RenderState.faceOtShift = 0xA;
    g_RenderState.mode = GAME_RENDER_PASS_MAIN;
    g_RenderState.ft4Color[0] = 0x80;
    g_RenderState.ft4Color[1] = 0x80;
    g_RenderState.ft4Color[2] = 0x80;
    g_RenderState.ft4Color[3] = POLY_FT4_CODE;
    g_RenderState.gt4Color[0] = 0xFF;
    g_RenderState.gt4Color[1] = 0xFF;
    g_RenderState.gt4Color[2] = 0xFF;
    g_RenderState.gt4Color[3] = POLY_GT4_CODE;
    g_RenderState.x0 = 0;
    g_RenderState.y0 = 0;
    g_RenderState.x1 = SCREEN_WIDTH;
    g_RenderState.y1 = SCREEN_HEIGHT;
    g_RenderState.otShift = otShift;
    g_RenderState.orderingFlag = g_MirrorMode;
    g_VisibleCellMask = g_MainVisibleCellMask;
    g_VisibleCellList = g_MainVisibleCellList;
}
