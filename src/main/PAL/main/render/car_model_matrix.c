#include "game/car_model_matrix.h"
#include "game/render_internal.h"

void FlipMatrixXZColumns(Matrix *destination, const Matrix *source) {
    s32 row;

    *destination = *source;
    for (row = 0; row < 3; row++) {
        destination->m[row][0] = WrapRenderCoordinate16(
            -(int64_t)destination->m[row][0]);
        destination->m[row][2] = WrapRenderCoordinate16(
            -(int64_t)destination->m[row][2]);
    }
}
