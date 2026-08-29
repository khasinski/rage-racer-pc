#include "game/render_internal.h"
#include "rage/render_world_game.h"


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

    GameRenderWorldSetCamera(SCRATCH_VIEW_X, SCRATCH_VIEW_Y, SCRATCH_VIEW_Z,
                                 SCRATCH_VIEW_ANGLE_X, SCRATCH_VIEW_ANGLE_Y,
                                 SCRATCH_VIEW_ANGLE_Z);
    BuildRotMatrixY(scratch, SCRATCH_VIEW_ANGLE_Y);
    BuildRotMatrixX(&mtx, SCRATCH_VIEW_ANGLE_X);
    MulMatrix2(&mtx, scratch);
    BuildRotMatrixZ(&mtx, SCRATCH_VIEW_ANGLE_Z);
    MulMatrix2(&mtx, scratch);
    BuildRotMatrixY(&mtx, 0x800);
    MulMatrix0(&mtx, scratch, &g_MirrorViewMatrix);
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

/*
 * Rotate a full-width vector by a matrix. The GTE's own vector op is
 * 16-bit, so the game keeps this one for the camera, whose offsets and
 * distances do not fit in a short.
 */
void ApplyMatrixLV(void *matrix, const s32 *input, s32 *output) {
    const s16 *m = matrix;
    int row;
    for (row = 0; row < 3; row++) {
        int64_t value = (int64_t)m[row * 3] * input[0]
                      + (int64_t)m[row * 3 + 1] * input[1]
                      + (int64_t)m[row * 3 + 2] * input[2];
        output[row] = (s32)(value >> 12);
    }
}
