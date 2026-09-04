#include "common.h"
#include "game/boot_internal.h"
#include "game/scene.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void (*g_SceneHandlers[GAME_SCENE_HANDLER_COUNT])(void);
s32 g_SceneId;
s32 g_SceneTimer;

static s32 s_handlerCalls;
static s32 s_traceCalls;
static char s_traceTopic[32];
static char s_traceMessage[64];

static void SceneHandler(void) {
    s_handlerCalls++;
}

void Trace(const char *topic, const char *format, ...) {
    va_list arguments;

    s_traceCalls++;
    snprintf(s_traceTopic, sizeof(s_traceTopic), "%s", topic);
    va_start(arguments, format);
    vsnprintf(s_traceMessage, sizeof(s_traceMessage), format, arguments);
    va_end(arguments);
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(g_SceneHandlers, 0, sizeof(g_SceneHandlers));
    s_handlerCalls = 0;
    s_traceCalls = 0;
    s_traceTopic[0] = '\0';
    s_traceMessage[0] = '\0';
}

int main(void) {
    Reset();
    g_SceneId = 7;
    g_SceneHandlers[7] = SceneHandler;
    DispatchCurrentScene();
    CHECK(s_handlerCalls == 1 && s_traceCalls == 0);

    Reset();
    g_SceneId = 7;
    g_SceneTimer = 123;
    DispatchCurrentScene();
    CHECK(s_handlerCalls == 0 && s_traceCalls == 1);
    CHECK(strcmp(s_traceTopic, "scene-unhandled") == 0);
    CHECK(strcmp(s_traceMessage, "id=7 timer=123") == 0);

    Reset();
    g_SceneId = -1;
    g_SceneTimer = 4;
    DispatchCurrentScene();
    CHECK(s_traceCalls == 1);
    CHECK(strcmp(s_traceMessage, "id=-1 timer=4") == 0);

    Reset();
    g_SceneId = GAME_SCENE_HANDLER_COUNT;
    DispatchCurrentScene();
    CHECK(s_traceCalls == 1);

    puts("scene dispatch rejects empty and out-of-range handlers");
    return 0;
}
