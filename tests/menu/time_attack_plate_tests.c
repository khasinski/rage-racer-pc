#include "game/menu.h"
#include "game/render.h"
#include "game/render_state.h"

#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
s32 g_TimeAttackPlateProgress;

static s32 s_drawCount;
static s16 s_top;
static s16 s_bottom;

void GameDrawTexturedQuad(
    GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2, u16 y2, u16 x3,
    u16 y3, u8 u0, u8 v0, u8 u1, u8 v1, u8 u2, u8 v2, u8 u3, u8 v3,
    u8 r, u8 g, u8 b, u16 clut, s32 shadeTex, s32 semiTrans, u16 tpage) {
    (void)x0; (void)x1; (void)y1; (void)x2; (void)x3;
    (void)u0; (void)v0; (void)u1; (void)v1; (void)u2; (void)v2;
    (void)u3; (void)v3; (void)r; (void)g; (void)b; (void)clut;
    (void)shadeTex; (void)semiTrans; (void)tpage;
    if (ot == g_RenderState.primData) {
        s_drawCount++;
        s_top = y0;
        s_bottom = (s16)y2;
        if ((s16)y3 != s_bottom) s_drawCount = -100;
    }
}

int main(void) {
    GameOrderingTableEntry orderingTable;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    g_RenderState.primData = &orderingTable;
    g_TimeAttackPlateProgress = 8;
    DrawTimeAttackPlate(0);
    if (g_TimeAttackPlateProgress != 0 || s_drawCount != 0) return 1;

    DrawTimeAttackPlate(5);
    if (g_TimeAttackPlateProgress != 5 || s_drawCount != 0) return 1;
    DrawTimeAttackPlate(3);
    if (g_TimeAttackPlateProgress != 8 || s_drawCount != 1 ||
        s_top != 0xD2 || s_bottom != 0xDD) return 1;
    DrawTimeAttackPlate(20);
    if (g_TimeAttackPlateProgress != 0xC || s_drawCount != 2) return 1;
    DrawTimeAttackPlate(-20);
    if (g_TimeAttackPlateProgress != 0 || s_drawCount != 2) return 1;

    puts("time attack plate animation preserved");
    return 0;
}
