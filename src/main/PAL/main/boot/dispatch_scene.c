#include "game/boot_internal.h"
#include "game/diagnostics.h"
#include "game/state.h"

void DispatchCurrentScene(void) {
    const u32 sceneHandlerCount =
        sizeof(g_SceneHandlers) / sizeof(g_SceneHandlers[0]);
    s32 sceneId = g_SceneId;

    if ((u32)sceneId >= sceneHandlerCount ||
        g_SceneHandlers[sceneId] == NULL) {
        Trace("scene-unhandled", "id=%d timer=%d", sceneId, g_SceneTimer);
        return;
    }

    g_SceneHandlers[sceneId]();
}
