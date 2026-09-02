#include "game/render.h"
#include "game/state.h"
#include "game/render_internal.h"
#include "game/track_internal.h"

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

        cellX = obj->x / 2048;
        cellZ = obj->z / 2048;
        if ((g_VisibleCellMask[cellZ] & (1 << cellX)) == 0) continue;

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


static u32 GetCellRegion(s32 x, s32 z) {
    z = (z * 32) + x;
    return g_TerrainCellGrid[z] >> 10;
}


static u32 IsCellVisibleFromRegion(s32 cellX, s32 cellZ, s32 region) {
    u32 visibleRegions;
    u32 mask;

    visibleRegions = g_CellVisibilityTable[cellZ][cellX];
    mask = 1;
    return (mask << region) & visibleRegions;
}

void BuildVisibleCells(s32 near, s32 far) {
    GameViewState *view = RENDER_VIEW_STATE;
    s32 i;
    s32 oct;
    s32 cx, cy;
    u32 ret0;
    Vec4 *out;
    s32 sx;
    s32 sy;
    s32 center;
    s32 vec[3];
    s32 proj[3];

    for (i = 31; i >= 0; i--) {
        g_VisibleCellMask[i] = 0;
    }

    oct = (view->angleY / 128) & 0x1F;
    cx = view->position.components.x.value / 2048;
    cy = view->position.components.z.value / 2048;
    ret0 = GetCellRegion(cx, cy);

    i = 0;
    out = g_VisibleCellList;
    for (; i < VISIBLE_CELL_COUNT; i++, out++) {
        s32 offset[2];
        s32 invalid = -1;

        GetVisibleCellScanOffset(oct, i, g_RenderState.orderingFlag, offset);
        sx = cx + offset[0];
        sy = cy + offset[1];
        if ((u32)sx < 32U && (u32)sy < 32U &&
            IsCellVisibleFromRegion(sx, sy, ret0)) {
            s32 clut = g_TerrainCellGrid[((31 - sy) << 5) + sx] & 0x3FF;

            out->w = clut;
            g_VisibleCellMask[sy] |= 1 << sx;
            center = 1024;
            if (clut != 0x3FF) {
                /* Retail uses signed MIPS shifts here.  Multiplication has the
                 * same result for this bounded cell/camera range without C's
                 * undefined left-shift-of-negative behaviour. */
                vec[0] = ((sx << 11) - (view->position.components.x.value - center)) * 4;
                vec[1] = (-view->position.components.y.value) * 4;
                vec[2] = ((sy << 11) - (view->position.components.z.value - center)) * 4;
                ApplyMatrixLV((&g_RenderState.matrix), vec, proj);
                if (proj[2] >= near && far >= proj[2]) {
                    out->x = proj[0];
                    out->y = proj[1];
                    out->z = proj[2];
                    continue;
                }
                out->w = invalid;
                continue;
            }
            out->w = invalid;
            continue;
        }
        out->w = invalid;
    }
}
