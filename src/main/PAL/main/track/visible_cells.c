#include "common.h"
#include "game/race.h"
#include "mirror_pass.h"
#include "game/render.h"
#include "game/scratchpad.h"
#include "game/state.h"
#include "game/track.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/vector.h"
#include "psyq/gte.h"

/*
 * Draw loop over the world-object array g_CourseObjects (g_CourseObjectCount entries). For
 * each visible object (id != -1, passing the per-sector visibility bitmask test
 * against g_VisibleCellMask) it builds a Z-rotation matrix in the scratchpad
 * (0x1F800028), transforms the object position through the GTE
 * (0x1F80011C -> 0x1F800124), sets the primitive shade/semi-trans mode word at
 * 0x1F800084, then dispatches a prim builder (SubmitCourseModel2 / SubmitCourseModel)
 * on the scratchpad OT at 0x1F800000.
 */
void DrawCourseObjects(void) {
    Matrix mtx;
    CourseObject *obj;
    s32 i;
    s32 visShift;
    s32 vis;
    s32 flags;

    obj = g_CourseObjects;
    i = 0;
    if (g_CourseObjectCount <= 0) {
        return;
    }

    do {
        if (obj->modelId == -1) {
        } else {
        visShift = obj->x / 2048;  /* per-sector visibility bit index */
        vis = g_VisibleCellMask[obj->z / 2048] & (1 << visShift);
        if (vis == 0) {
            continue;
        }

        BuildRotMatrixY(&mtx, obj->field2);
        MulMatrix2(SCRATCH_VIEW_MATRIX_GTE, &mtx);
        {
            s32 transformed;
            s32 camera;

            transformed = (u16)obj->x;
            camera = SCRATCH_VIEW_STATE->position.components.x.half.low;
            transformed -= camera;
            SCRATCH_OBJECT_MATRIX_WORK->relative[0] = transformed;
            transformed = (u16)obj->y;
            camera = SCRATCH_VIEW_STATE->position.components.y.half.low;
            transformed -= camera;
            SCRATCH_OBJECT_MATRIX_WORK->relative[1] = transformed;
            transformed = (u16)obj->z;
            camera = SCRATCH_VIEW_STATE->position.components.z.half.low;
            transformed -= camera;
            SCRATCH_OBJECT_MATRIX_WORK->relative[2] = transformed;

            ApplyMatrix(SCRATCH_VIEW_MATRIX_GTE,
                        SCRATCH_OBJECT_MATRIX_WORK->relative,
                        &SCRATCH_OBJECT_MATRIX_WORK->view);
            transformed = SCRATCH_OBJECT_MATRIX_WORK->view.x;
            camera = SCRATCH_OBJECT_MATRIX_WORK->view.z;
            transformed *= 4;
            SCRATCH_OBJECT_MATRIX_WORK->mtx.t[0] = transformed;
            transformed = SCRATCH_OBJECT_MATRIX_WORK->view.y;
            camera *= 4;
            SCRATCH_OBJECT_MATRIX_WORK->mtx.t[2] = camera;
            transformed *= 4;
            SCRATCH_OBJECT_MATRIX_WORK->mtx.t[1] = transformed;
        }
        SetRotMatrix(&mtx);
        SetTransMatrix(&SCRATCH_OBJECT_MATRIX_WORK->mtx);

        flags = obj->flags;
        if (flags & 8) {
            SCRATCH_ENV_MODE4 = ((g_AnimTimer & 0x10) == 0) << 16;
        } else if (flags & 4) {
            SCRATCH_ENV_MODE4 = 0x10000;
        } else {
            SCRATCH_ENV_MODE4 = 0;
        }

        if (g_IsEnvironmentMode4 ? (obj->flags & 2) : (obj->flags % 2)) {
            SubmitCourseModel2(SCRATCHPAD, obj->modelId);
        } else {
            SubmitCourseModel(SCRATCHPAD, obj->modelId);
        }

        }
    } while (i++, obj++, i < g_CourseObjectCount);
}


u32 GetCellRegion(s32 x, s32 z) {
    z = (z * 32) + x;
    return g_TerrainCellGrid[z] >> 10;
}


u32 IsCellVisibleFromRegion(s32 cellX, s32 cellZ, s32 region) {
    u32 visibleRegions;
    u32 mask;

    visibleRegions = g_CellVisibilityTable[cellZ][cellX];
    mask = 1;
    return (mask << region) & visibleRegions;
}

void BuildVisibleCells(s32 near, s32 far) {
    ScratchViewState *view = SCRATCH_VIEW_STATE;
    s32 i;
    s32 j;
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
    j = 0;
    for (; i < 64; j += 2, i++, out++) {
        s32 k;
        s32 dx;
        s32 dy;
        s32 invalid = -1;

        switch (oct / 8) {
        case 0:
            k = j + (oct << 7);
            dx = g_CellScanOffsetX[k];
            dy = g_CellScanOffsetY[k];
            sx = cx + dx;
            sy = cy + dy;
            break;
        case 1:
            k = j + ((8 - (oct % 8)) << 7);
            dx = g_CellScanOffsetX[k];
            dy = g_CellScanOffsetY[k];
            sx = cx + dx;
            sy = cy - dy;
            break;
        case 2:
            k = j + ((oct - 16) << 7);
            dx = g_CellScanOffsetX[k];
            dy = g_CellScanOffsetY[k];
            sx = cx - dx;
            sy = cy - dy;
            break;
        case 3:
            k = oct % 8;
            k = 8 - k;
            k = j + (k << 7);
            dx = g_CellScanOffsetX[k];
            dy = g_CellScanOffsetY[k];
            /* The rear-view pass reflects this quadrant.  Applying its signs
             * to the main view drops the left-hand cells ahead of the car. */
            /* Reflecting the view does not change which cells fall inside
             * the frustum, so a mirrored course scans exactly like a normal
             * one. Only the rear view needs the reflected quadrant. */
            if (RageMirrorRearViewPass(SCRATCH_MIRROR, g_MirrorMode)) {
                sx = cx + dx;
                sy = cy - dy;
            } else {
                sx = cx - dx;
                sy = cy + dy;
            }
            break;
        }
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
                ApplyMatrixLV(SCRATCH_VIEW_MATRIX_GTE, vec, proj);
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
