#include "game/render.h"
#include "game/state.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

GameRenderState g_RenderState;
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
    ObjectMatrixWork work;
    LVec position = {130, 180, 360};
    Matrix rotation;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&work, 0, sizeof(work));
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

    SetGteObjectMatrix(&work, &position, &rotation);

    CHECK_EQ(work.relative[0], 30);
    CHECK_EQ(work.relative[1], -20);
    CHECK_EQ(work.relative[2], 60);
    CHECK_EQ(work.view.x, 30);
    CHECK_EQ(work.view.y, -20);
    CHECK_EQ(work.view.z, 60);
    CHECK_EQ(work.mtx.t[0], 120);
    CHECK_EQ(work.mtx.t[1], -80);
    CHECK_EQ(work.mtx.t[2], 240);

    puts("object matrix tests passed");
    return 0;
}
