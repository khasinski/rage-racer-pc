#include "game/render.h"
#include "psyz/gte.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
Camera g_Camera;

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            fprintf(stderr, "%s:%d: got %d, expected %d\n", __FILE__,        \
                    __LINE__, (s32)(actual), (s32)(expected));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static int TestObjectTranslation(void) {
    LVec position = {130, 180, 360};
    Matrix rotation;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&g_Camera, 0, sizeof(g_Camera));
    memset(&rotation, 0, sizeof(rotation));
    g_Camera.view.x = 100;
    g_Camera.view.y = 200;
    g_Camera.view.z = 300;
    g_RenderState.geometry.matrix.m[0][0] = 4096;
    g_RenderState.geometry.matrix.m[1][1] = 4096;
    g_RenderState.geometry.matrix.m[2][2] = 4096;
    rotation.m[0][0] = 4096;
    rotation.m[1][1] = 4096;
    rotation.m[2][2] = 4096;

    SetGteObjectMatrix(&position, &rotation);

    CHECK_EQ((s32)Psyz_GteCtrlRead(5), 120);
    CHECK_EQ((s32)Psyz_GteCtrlRead(6), -80);
    CHECK_EQ((s32)Psyz_GteCtrlRead(7), 240);

    return 0;
}

static int TestPositionSubtractionWrapsLikeThePs1(void) {
    LVec position = {INT_MIN, INT_MAX, 0};
    Matrix rotation;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&g_Camera, 0, sizeof(g_Camera));
    memset(&rotation, 0, sizeof(rotation));
    g_RenderState.geometry.matrix.m[0][0] = 4096;
    g_RenderState.geometry.matrix.m[1][1] = 4096;
    g_RenderState.geometry.matrix.m[2][2] = 4096;
    g_Camera.view.x = INT_MAX;
    g_Camera.view.y = INT_MIN;

    SetGteObjectMatrix(&position, &rotation);

    CHECK_EQ((s32)Psyz_GteCtrlRead(5), 4);
    CHECK_EQ((s32)Psyz_GteCtrlRead(6), -4);
    CHECK_EQ((s32)Psyz_GteCtrlRead(7), 0);
    return 0;
}

int main(void) {
    if (TestObjectTranslation() != 0 ||
        TestPositionSubtractionWrapsLikeThePs1() != 0) {
        return 1;
    }
    puts("object matrix tests passed");
    return 0;
}
