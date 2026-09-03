#include "game/render.h"
#include "game/state.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

#include <limits.h>

enum {
    TERRAIN_CELL_SIZE = 2048,
    TERRAIN_CELL_HALF_SIZE = TERRAIN_CELL_SIZE / 2,
    VIEW_ANGLE_PER_SCAN_DIRECTION = 128,
};

static u32 GetCellRegion(s32 cellX, s32 cellZ) {
    return g_TerrainCellGrid[cellZ * TERRAIN_CELL_GRID_SIZE + cellX] >>
           TERRAIN_CELL_REGION_SHIFT;
}

static int IsCellVisibleFromRegion(s32 cellX, s32 cellZ, u32 region) {
    if (region >= TERRAIN_CELL_REGION_COUNT) {
        return 0;
    }
    return g_CellVisibilityTable[cellZ][cellX] & (1u << region);
}

static void ClearVisibleCellOutput(void) {
    s32 index;

    if (g_VisibleCellMask != NULL) {
        for (index = 0; index < TERRAIN_CELL_GRID_SIZE; index++) {
            g_VisibleCellMask[index] = 0;
        }
    }
    if (g_VisibleCellList != NULL) {
        for (index = 0; index < VISIBLE_CELL_COUNT; index++) {
            g_VisibleCellList[index].cellIndex = -1;
        }
    }
}

static s32 NegatedTimesFour(s32 value) {
    u32 bits = 0u - (u32)value * 4u;

    return bits <= INT32_MAX
        ? (s32)bits
        : -(s32)(UINT32_MAX - bits) - 1;
}

void BuildVisibleCells(s32 near, s32 far) {
    GameViewState *view = RENDER_VIEW_STATE;
    const s32 direction =
        (view->angleY / VIEW_ANGLE_PER_SCAN_DIRECTION) & 0x1F;
    const s32 cameraCellX =
        view->position.components.x.value / TERRAIN_CELL_SIZE;
    const s32 cameraCellZ =
        view->position.components.z.value / TERRAIN_CELL_SIZE;
    u32 cameraRegion;
    s32 index;

    ClearVisibleCellOutput();
    if (g_VisibleCellMask == NULL || g_VisibleCellList == NULL ||
        g_TerrainCellGrid == NULL || g_CellVisibilityTable == NULL) {
        return;
    }
    if ((u32)cameraCellX >= TERRAIN_CELL_GRID_SIZE ||
        (u32)cameraCellZ >= TERRAIN_CELL_GRID_SIZE) {
        return;
    }
    cameraRegion = GetCellRegion(cameraCellX, cameraCellZ);

    for (index = 0; index < VISIBLE_CELL_COUNT; index++) {
        VisibleTerrainCell *out = &g_VisibleCellList[index];
        s32 offset[2];
        s32 cellX;
        s32 cellZ;
        s32 cellIndex;
        s32 worldOffset[3];
        s32 projected[3];

        GetVisibleCellScanOffset(direction, index,
                                 g_RenderState.orderingFlag, offset);
        cellX = cameraCellX + offset[0];
        cellZ = cameraCellZ + offset[1];
        if ((u32)cellX >= TERRAIN_CELL_GRID_SIZE ||
            (u32)cellZ >= TERRAIN_CELL_GRID_SIZE ||
            !IsCellVisibleFromRegion(cellX, cellZ, cameraRegion)) {
            continue;
        }

        cellIndex =
            g_TerrainCellGrid[(TERRAIN_CELL_GRID_SIZE - 1 - cellZ) *
                                  TERRAIN_CELL_GRID_SIZE +
                              cellX] &
            TERRAIN_CELL_INDEX_MASK;
        g_VisibleCellMask[cellZ] |= 1u << cellX;
        if (cellIndex == TERRAIN_MISSING_CELL_INDEX) {
            continue;
        }

        /* Retail uses signed MIPS shifts here. Multiplication has the same
         * result for this bounded cell/camera range without C's undefined
         * left-shift-of-negative behaviour. */
        worldOffset[0] =
            (cellX * TERRAIN_CELL_SIZE -
             (view->position.components.x.value - TERRAIN_CELL_HALF_SIZE)) *
            4;
        worldOffset[1] = NegatedTimesFour(
            view->position.components.y.value);
        worldOffset[2] =
            (cellZ * TERRAIN_CELL_SIZE -
             (view->position.components.z.value - TERRAIN_CELL_HALF_SIZE)) *
            4;
        ApplyMatrixLV(&g_RenderState.matrix, worldOffset, projected);
        if (projected[2] < near || projected[2] > far) {
            continue;
        }

        out->x = projected[0];
        out->y = projected[1];
        out->z = projected[2];
        out->cellIndex = cellIndex;
    }
}
