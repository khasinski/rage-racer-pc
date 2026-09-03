#include "game/render.h"
#include "game/scenery_render_internal.h"

#include <stdio.h>

GameRenderState g_RenderState;

static s32 s_xAngle;
static s32 s_yAngle;
static s32 s_zAngle;
static s32 s_multiplyCount;

static void SetMarker(void *matrix, s16 marker) {
    Matrix *typed = matrix;
    typed->m[0][0] = marker;
}

void BuildRotMatrixX(void *matrix, s32 angle) {
    s_xAngle = angle;
    SetMarker(matrix, 2);
}
void BuildRotMatrixY(void *matrix, s32 angle) {
    s_yAngle = angle;
    SetMarker(matrix, 1);
}
void BuildRotMatrixZ(void *matrix, s32 angle) {
    s_zAngle = angle;
    SetMarker(matrix, 4);
}

#undef MulMatrix2
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    right->m[0][0] = (s16)(left->m[0][0] * 10 + right->m[0][0]);
    s_multiplyCount++;
    return right;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    Matrix result = {0};

    g_RenderState.matrix.m[0][0] = 3;
    BuildSceneryObjectMatrix(&result, 0x123, 0x234, 0x345);

    CHECK(s_xAngle == 0x123);
    CHECK(s_yAngle == 0x800 - 0x234);
    CHECK(s_zAngle == 0x345);
    CHECK(s_multiplyCount == 3);
    CHECK(result.m[0][0] == 424);

    puts("scenery object matrix tests passed");
    return 0;
}
