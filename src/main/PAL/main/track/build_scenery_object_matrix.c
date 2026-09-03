#include "game/render.h"
#include "game/scenery_render_internal.h"

void BuildSceneryObjectMatrix(Matrix *matrix, s32 rotationX, s32 rotationY,
                              s32 rotationZ) {
    Matrix rotation;

    BuildRotMatrixY(&rotation, 0x800 - rotationY);
    BuildRotMatrixX(matrix, rotationX);
    MulMatrix2(&rotation, matrix);
    MulMatrix2(&g_RenderState.matrix, matrix);
    BuildRotMatrixZ(&rotation, rotationZ);
    MulMatrix2(matrix, &rotation);
    *matrix = rotation;
}
