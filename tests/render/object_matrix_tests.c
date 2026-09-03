#include "game/render.h"
#include "game/state.h"
#include "psyz/gte.h"

#include <limits.h>
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

static int TestObjectTranslation(void) {
    LVec position = {130, 180, 360};
    Matrix rotation;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
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

    CHECK_EQ((s32)Psyz_GteCtrlRead(5), 120);
    CHECK_EQ((s32)Psyz_GteCtrlRead(6), -80);
    CHECK_EQ((s32)Psyz_GteCtrlRead(7), 240);

    return 0;
}

static int TestPositionSubtractionWrapsLikeThePs1(void) {
    LVec position = {INT_MIN, INT_MAX, 0};
    Matrix rotation;

    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(&rotation, 0, sizeof(rotation));
    g_RenderState.matrix.m[0][0] = 4096;
    g_RenderState.matrix.m[1][1] = 4096;
    g_RenderState.matrix.m[2][2] = 4096;
    RENDER_VIEW_STATE->position.vector.x = INT_MAX;
    RENDER_VIEW_STATE->position.vector.y = INT_MIN;

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
