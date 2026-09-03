#include "game/render.h"
#include "game/state.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
ObjectMatrixWork g_ObjectMatrixWork;
s32 g_SceneTimer;

int DiagnosticsEnabled(const char *key) {
    (void)key;
    return 0;
}

const char *DiagnosticsValue(const char *key) {
    (void)key;
    return NULL;
}

int DiagnosticsIntValue(const char *key, int fallback) {
    (void)key;
    return fallback;
}

void Trace(const char *topic, const char *format, ...) {
    (void)topic;
    (void)format;
}

#define CHECK_EQ(actual, expected)                                             \
    do {                                                                       \
        if ((actual) != (expected)) {                                          \
            fprintf(stderr, "%s:%d: got %d, expected %d\n", __FILE__,        \
                    __LINE__, (s32)(actual), (s32)(expected));                 \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    LVec position = {130, 180, 360};
    Matrix rotation;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&g_ObjectMatrixWork, 0, sizeof(g_ObjectMatrixWork));
    memset(&rotation, 0, sizeof(rotation));
    RENDER_VIEW_STATE->position.vector.x = 100;
    RENDER_VIEW_STATE->position.vector.y = 200;
    RENDER_VIEW_STATE->position.vector.z = 300;
    g_RenderState.matrix.m[0][0] = 4096;
    g_RenderState.matrix.m[1][1] = 4096;
    g_RenderState.matrix.m[2][2] = 4096;
    rotation.m[0][0] = 4096;
    rotation.m[1][1] = 4096;
    rotation.m[2][2] = 4096;

    SetGteObjectMatrix(&position, &rotation);

    CHECK_EQ(g_ObjectMatrixWork.relative[0], 30);
    CHECK_EQ(g_ObjectMatrixWork.relative[1], -20);
    CHECK_EQ(g_ObjectMatrixWork.relative[2], 60);
    CHECK_EQ(g_ObjectMatrixWork.view.x, 30);
    CHECK_EQ(g_ObjectMatrixWork.view.y, -20);
    CHECK_EQ(g_ObjectMatrixWork.view.z, 60);
    CHECK_EQ(g_ObjectMatrixWork.mtx.t[0], 120);
    CHECK_EQ(g_ObjectMatrixWork.mtx.t[1], -80);
    CHECK_EQ(g_ObjectMatrixWork.mtx.t[2], 240);

    puts("object matrix tests passed");
    return 0;
}
