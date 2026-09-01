#include "game/render.h"
#include "game/render_internal.h"
#include "game/terrain_internal.h"

static void DrawTerrainCellsFrom(s32 viewOffset) {
    BuildVisibleCells(viewOffset, 0x14000);
    SetRotMatrix(&g_RenderState.matrix);
    SubmitTerrainCells(&g_RenderState, g_VisibleCellList, 0x40);
}

void DrawTerrainCells(void) {
    DrawTerrainCellsFrom(-12288);
}

void DrawTerrainCellsWide(void) {
    DrawTerrainCellsFrom((s32)0xFFFF6000);
}
