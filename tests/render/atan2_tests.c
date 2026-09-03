#include "game/render.h"
#include "game/render_internal.h"

#include <limits.h>
#include <stdio.h>

GameRenderState g_RenderState;
Matrix g_MirrorViewMatrix;
s16 g_AtanTable[1026];

void GameRenderWorldSetCamera(s32 x, s32 y, s32 z, s32 pitch, s32 yaw,
                              s32 roll) {
    (void)x;
    (void)y;
    (void)z;
    (void)pitch;
    (void)yaw;
    (void)roll;
}

MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *output) {
    (void)left;
    (void)right;
    return output;
}

#undef MulMatrix2
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right) {
    (void)left;
    return right;
}

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            fprintf(stderr, "%s:%d: got %d, expected %d\n", __FILE__,       \
                    __LINE__, (s32)(actual), (s32)(expected));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    Matrix matrix = {0};
    s32 input[3] = {100, -200, 300};
    s32 output[3];
    s32 row;
    s32 column;
    s32 index;

    for (index = 0; index < 1026; index++) {
        g_AtanTable[index] = (s16)(index / 2);
    }

    CHECK_EQ(Atan2(0, 0), 0);
    CHECK_EQ(Atan2(4, 0), 0);
    CHECK_EQ(Atan2(0, 4), 0x400);
    CHECK_EQ(Atan2(0, -4), -0x400);

    CHECK_EQ(Atan2(8, 4), 0x100);
    CHECK_EQ(Atan2(4, 8), 0x300);
    CHECK_EQ(Atan2(8, -4), -0x100);
    CHECK_EQ(Atan2(4, -8), -0x300);
    CHECK_EQ(Atan2(-8, 4), 0x700);
    CHECK_EQ(Atan2(-4, 8), 0x500);
    CHECK_EQ(Atan2(-8, -4), 0x900);
    CHECK_EQ(Atan2(-4, -8), 0xB00);

    CHECK_EQ(Atan2(INT_MAX, INT_MIN), -0x201);
    CHECK_EQ(Atan2(INT_MIN, INT_MAX), 0x601);
    CHECK_EQ(Atan2(INT_MIN, INT_MIN), 0xA00);

    matrix.m[0][0] = 0x1000;
    matrix.m[1][1] = 0x1000;
    matrix.m[2][2] = 0x1000;
    ApplyMatrixLV(&matrix, input, output);
    CHECK_EQ(output[0], 100);
    CHECK_EQ(output[1], -200);
    CHECK_EQ(output[2], 300);

    for (row = 0; row < 3; row++) {
        for (column = 0; column < 3; column++) {
            matrix.m[row][column] = INT16_MAX;
        }
    }
    input[0] = INT_MAX;
    input[1] = INT_MAX;
    input[2] = INT_MAX;
    ApplyMatrixLV(&matrix, input, output);
    CHECK_EQ(output[0], -1572888);
    CHECK_EQ(output[1], -1572888);
    CHECK_EQ(output[2], -1572888);

    puts("rotation math handles full-width inputs");
    return 0;
}
