#include "game/render.h"
#include "game/state.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

enum {
    TERRAIN_CELL_SIZE = 2048,
    TERRAIN_CELL_HALF_SIZE = TERRAIN_CELL_SIZE / 2,
    TERRAIN_REGION_COUNT = 32,
    TERRAIN_MISSING_CLUT = 0x3FF,
    VIEW_ANGLE_PER_SCAN_DIRECTION = 128,
};

/*
 * Draw loop over the world-object array g_CourseObjects (g_CourseObjectCount entries). For
 * each visible object (id != -1, passing the per-sector visibility bitmask test
 * against g_VisibleCellMask) it builds a Z-rotation matrix in the render state
 * (0x1F800028), transforms the object position through the GTE
 * (0x1F80011C -> 0x1F800124), sets the primitive shade/semi-trans mode word at
 * 0x1F800084, then dispatches a prim builder (SubmitCourseModel2 / SubmitCourseModel)
 * on the render state's OT.
 */
void DrawCourseObjects(void) {
    Matrix mtx;
    s32 i;

    for (i = 0; i < g_CourseObjectCount; i++) {
        CourseObject *obj = &g_CourseObjects[i];
        s32 cellX;
        s32 cellZ;
        s32 transformed;
        s32 camera;
        s32 flags;

        if (obj->modelId == -1) continue;

        cellX = obj->x / TERRAIN_CELL_SIZE;
        cellZ = obj->z / TERRAIN_CELL_SIZE;
        if (!CellVisibilityMaskContains(g_VisibleCellMask, cellX, cellZ)) {
            continue;
        }

        BuildRotMatrixY(&mtx, obj->field2);
        MulMatrix2(&g_RenderState.matrix, &mtx);

        transformed = (u16)obj->x;
        camera = RENDER_VIEW_STATE->position.components.x.half.low;
        g_ObjectMatrixWork.relative[0] = transformed - camera;
        transformed = (u16)obj->y;
        camera = RENDER_VIEW_STATE->position.components.y.half.low;
        g_ObjectMatrixWork.relative[1] = transformed - camera;
        transformed = (u16)obj->z;
        camera = RENDER_VIEW_STATE->position.components.z.half.low;
        g_ObjectMatrixWork.relative[2] = transformed - camera;

        ApplyMatrix(&g_RenderState.matrix, g_ObjectMatrixWork.relative,
                    &g_ObjectMatrixWork.view);
        g_ObjectMatrixWork.mtx.t[0] = g_ObjectMatrixWork.view.x * 4;
        g_ObjectMatrixWork.mtx.t[1] = g_ObjectMatrixWork.view.y * 4;
        g_ObjectMatrixWork.mtx.t[2] = g_ObjectMatrixWork.view.z * 4;

        SetRotMatrix(&mtx);
        SetTransMatrix(&g_ObjectMatrixWork.mtx);

        flags = obj->flags;
        if (flags & 8) {
            g_RenderState.envMode4 = ((g_AnimTimer & 0x10) == 0) << 16;
        } else if (flags & 4) {
            g_RenderState.envMode4 = 0x10000;
        } else {
            g_RenderState.envMode4 = 0;
        }

        if (g_IsEnvironmentMode4 ? (obj->flags & 2) : (obj->flags & 1)) {
            SubmitCourseModel2(&g_RenderState, obj->modelId);
        } else {
            SubmitCourseModel(&g_RenderState, obj->modelId);
        }
    }
}


static u32 GetCellRegion(s32 cellX, s32 cellZ) {
    return g_TerrainCellGrid[cellZ * TERRAIN_CELL_GRID_SIZE + cellX] >> 10;
}

static int IsCellVisibleFromRegion(s32 cellX, s32 cellZ, u32 region) {
    if (region >= TERRAIN_REGION_COUNT) {
        return 0;
    }
    return g_CellVisibilityTable[cellZ][cellX] & (1u << region);
}

static void ClearVisibleCellOutput(void) {
    s32 index;

    for (index = 0; index < TERRAIN_CELL_GRID_SIZE; index++) {
        g_VisibleCellMask[index] = 0;
    }
    for (index = 0; index < VISIBLE_CELL_COUNT; index++) {
        g_VisibleCellList[index].w = -1;
    }
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
    if ((u32)cameraCellX >= TERRAIN_CELL_GRID_SIZE ||
        (u32)cameraCellZ >= TERRAIN_CELL_GRID_SIZE) {
        return;
    }
    cameraRegion = GetCellRegion(cameraCellX, cameraCellZ);

    for (index = 0; index < VISIBLE_CELL_COUNT; index++) {
        Vec4 *out = &g_VisibleCellList[index];
        s32 offset[2];
        s32 cellX;
        s32 cellZ;
        s32 clut;
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

        clut = g_TerrainCellGrid[(TERRAIN_CELL_GRID_SIZE - 1 - cellZ) *
                                     TERRAIN_CELL_GRID_SIZE +
                                 cellX] &
               TERRAIN_MISSING_CLUT;
        g_VisibleCellMask[cellZ] |= 1u << cellX;
        if (clut == TERRAIN_MISSING_CLUT) {
            continue;
        }

        /* Retail uses signed MIPS shifts here. Multiplication has the same
         * result for this bounded cell/camera range without C's undefined
         * left-shift-of-negative behaviour. */
        worldOffset[0] =
            (cellX * TERRAIN_CELL_SIZE -
             (view->position.components.x.value - TERRAIN_CELL_HALF_SIZE)) *
            4;
        worldOffset[1] = -view->position.components.y.value * 4;
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
        out->w = clut;
    }
}
