#include "common.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"
#include "psyq/gte.h"


void BuildRotMatrixZ(Matrix *mtx, s32 angle) {
    s32 s;
    s32 c;

    s = rsin(angle);
    c = rcos(angle);
    mtx->m[0][0] = c;
    mtx->m[0][1] = -s;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = s;
    mtx->m[1][1] = c;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = 0x1000;
}


void BuildRotMatrixY(Matrix *mtx, s32 angle) {
    s32 s;
    s32 c;

    s = rsin(angle);
    c = rcos(angle);
    mtx->m[0][0] = c;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = -s;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = 0x1000;
    mtx->m[1][2] = 0;
    mtx->m[2][0] = s;
    mtx->m[2][1] = 0;
    mtx->m[2][2] = c;
}


void BuildRotMatrixX(Matrix *mtx, s32 angle) {
    s32 s;
    s32 c;

    s = rsin(angle);
    c = rcos(angle);
    mtx->m[0][0] = 0x1000;
    mtx->m[0][1] = 0;
    mtx->m[0][2] = 0;
    mtx->m[1][0] = 0;
    mtx->m[1][1] = c;
    mtx->m[1][2] = -s;
    mtx->m[2][0] = 0;
    mtx->m[2][1] = s;
    mtx->m[2][2] = c;
}


void SetCameraRotMatrix(void) {
    Matrix mtx;
    Matrix *scratch = SCRATCH_VIEW_MATRIX_GTE;

    BuildRotMatrixY(scratch, SCRATCH_VIEW_ANGLE_Y);
    BuildRotMatrixX(&mtx, SCRATCH_VIEW_ANGLE_X);
    MulMatrix2(&mtx, scratch);
    BuildRotMatrixZ(&mtx, SCRATCH_VIEW_ANGLE_Z);
    MulMatrix2(&mtx, scratch);
    BuildRotMatrixY(&mtx, 0x800);
    MulMatrix0(&mtx, scratch, &g_MirrorViewMatrix);
    /* A mirrored course reflects the whole main view, so install that here,
     * where the camera matrix is built, rather than leaving it to whoever draws
     * first. The race frame draws the rival cars before the terrain, and the
     * terrain dispatcher used to be what applied it, so the cars were composed
     * with an unreflected matrix while the winding tests already expected a
     * reflected one and turned them inside out.
     *
     * g_MirrorViewMatrix is built just above and stays unreflected: the
     * rear-view pass swaps it in whole and the dispatcher reflects that one. */
    if (g_MirrorMode) {
        scratch->m[0][0] = -scratch->m[0][0];
        scratch->m[0][1] = -scratch->m[0][1];
        scratch->m[0][2] = -scratch->m[0][2];
    }
    SetRotMatrix(scratch);
}


s32 Atan2(s32 x, s32 y) {
    if (x == 0) {
        if (y == 0) {
            return 0;
        }
        if (y > 0) {
            return 0x400;
        }
        return -0x400;
    }

    if (x > 0) {
        if (y >= 0) {
            if (x < y) {
                return 0x400 - g_AtanTable[(x << 10) / y];
            }
            return g_AtanTable[(y << 10) / x];
        }

        y = -y;
        if (x < y) {
            return g_AtanTable[(x << 10) / y] - 0x400;
        }
        return -g_AtanTable[(y << 10) / x];
    }

    x = -x;
    if (y >= 0) {
        if (x < y) {
            return g_AtanTable[(x << 10) / y] + 0x400;
        }
        return 0x800 - g_AtanTable[(y << 10) / x];
    }

    y = -y;
    if (x < y) {
        return 0xC00 - g_AtanTable[(x << 10) / y];
    }
    return g_AtanTable[(y << 10) / x] + 0x800;
}
