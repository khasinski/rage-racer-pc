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

void DrawTerrainCellsInRange(s32 nearDepth, s32 farDepth) {
    BuildVisibleCells(nearDepth, farDepth);
    SetRotMatrix(&g_RenderState.matrix);
    SubmitTerrainCells(&g_RenderState, g_VisibleCellList,
                       TERRAIN_SUBMISSION_CAPACITY);
}

void DrawTerrainCells(void) {
    DrawTerrainCellsInRange(RACE_TERRAIN_NEAR_DEPTH, TERRAIN_FAR_DEPTH);
}

void DrawTerrainCellsWide(void) {
    DrawTerrainCellsInRange(WIDE_TERRAIN_NEAR_DEPTH, TERRAIN_FAR_DEPTH);
}
