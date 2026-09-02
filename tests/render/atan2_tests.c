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

    puts("table atan2 handles every quadrant and full-width inputs");
    return 0;
}
