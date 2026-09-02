#include "game/render.h"
#include "game/render_internal.h"
#include "game/terrain_internal.h"
#include "game/track.h"

enum {
    TERRAIN_FAR_DEPTH = 0x14000,
    RACE_TERRAIN_NEAR_DEPTH = -0x3000,
    WIDE_TERRAIN_NEAR_DEPTH = -0xA000,
    TERRAIN_SUBMISSION_CAPACITY = 0x40,
};

static void DrawTerrainCellsFrom(s32 viewOffset) {
    BuildVisibleCells(viewOffset, TERRAIN_FAR_DEPTH);
    SetRotMatrix(&g_RenderState.matrix);
    SubmitTerrainCells(&g_RenderState, g_VisibleCellList,
                       TERRAIN_SUBMISSION_CAPACITY);
}

void DrawTerrainCells(void) {
    DrawTerrainCellsFrom(RACE_TERRAIN_NEAR_DEPTH);
}

void DrawTerrainCellsWide(void) {
    DrawTerrainCellsFrom(WIDE_TERRAIN_NEAR_DEPTH);
}
