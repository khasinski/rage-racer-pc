#include "game/render_internal.h"
#include "rage/render_world_game.h"

#include <stdint.h>

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
    Matrix *viewMatrix = (&g_RenderState.geometry.matrix);

    GameRenderWorldSetCamera(g_RenderState.camera.x, g_RenderState.camera.y, g_RenderState.camera.z,
                                 g_RenderState.camera.angleX, g_RenderState.camera.angleY,
                                 g_RenderState.camera.angleZ);
    BuildRotMatrixY(viewMatrix, g_RenderState.camera.angleY);
    BuildRotMatrixX(&mtx, g_RenderState.camera.angleX);
    MulMatrix2(&mtx, viewMatrix);
    BuildRotMatrixZ(&mtx, g_RenderState.camera.angleZ);
    MulMatrix2(&mtx, viewMatrix);
    BuildRotMatrixY(&mtx, 0x800);
    MulMatrix0(&mtx, viewMatrix, &g_MirrorViewMatrix);
    SetRotMatrix(viewMatrix);
}


/*
 * Rotate a full-width vector by a matrix. The GTE's own vector op is
 * 16-bit, so the game keeps this one for the camera, whose offsets and
 * distances do not fit in a short.
 */
void ApplyMatrixLV(const Matrix *matrix, const s32 *input, s32 *output) {
    int row;
    for (row = 0; row < 3; row++) {
        int64_t value = (int64_t)matrix->m[row][0] * input[0]
                      + (int64_t)matrix->m[row][1] * input[1]
                      + (int64_t)matrix->m[row][2] * input[2];
        output[row] = WrapSigned32(value >> 12);
    }
}
