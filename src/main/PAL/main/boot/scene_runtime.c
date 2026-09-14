#include "game/scene_runtime.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

#include <string.h>

static SceneRuntime s_runtime = {.scene = -1};

static void ResetLegacyTransitionState(void) {
    /* These fields are scratch state for a single top-level scene.  Retail
     * code has many direct scene writes; centralising the reset here makes a
     * newly dispatched scene independent of whichever path preceded it. */
    g_SceneTimer = 0;
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_CdTrackEnded = 0;
}

void SceneRuntimeBeforeDispatch(s32 scene) {
    if (s_runtime.scene != scene) {
        const int continueMemoryCard =
            scene == GAME_SCENE_MEMORY_CARD &&
            (s_runtime.scene == GAME_SCENE_ENTER_MEMORY_CARD ||
             s_runtime.scene == GAME_SCENE_ENTER_MEMORY_CARD_LOAD);
        const int continueBgmSelect =
            scene == GAME_SCENE_BGM_SELECT &&
            s_runtime.scene == GAME_SCENE_ENTER_BGM_SELECT;
        MemoryCardSession memoryCard = s_runtime.memoryCard;
        BgmSelect bgmSelect = s_runtime.bgmSelect;
        u32 generation = s_runtime.generation + 1;

        if (generation == 0) generation = 1;
        memset(&s_runtime, 0, sizeof(s_runtime));
        s_runtime.scene = scene;
        s_runtime.generation = generation;
        s_runtime.assetGeneration = AssetLoadTransactionGeneration();
        if (continueMemoryCard) {
            s_runtime.memoryCard = memoryCard;
        }
        if (continueBgmSelect) s_runtime.bgmSelect = bgmSelect;
        ResetLegacyTransitionState();
    }
    s_runtime.transition.timer = g_SceneTimer;
    s_runtime.transition.fadeLevel = g_FadeLevel;
    s_runtime.transition.fadeStep = g_FadeStep;
    s_runtime.transition.frameSyncThreshold = g_FrameSyncThreshold;
    s_runtime.transition.cameraCarIndex = g_CameraCarIndex;
}

void SceneRuntimeAfterDispatch(s32 scene) {
    if (s_runtime.scene != scene || g_SceneId != scene) return;
    s_runtime.transition.timer = g_SceneTimer;
    s_runtime.transition.fadeLevel = g_FadeLevel;
    s_runtime.transition.fadeStep = g_FadeStep;
    s_runtime.transition.frameSyncThreshold = g_FrameSyncThreshold;
    s_runtime.transition.cameraCarIndex = g_CameraCarIndex;
}

const SceneRuntime *SceneRuntimeCurrent(void) {
    return &s_runtime;
}

MemoryCardSession *SceneRuntimeMemoryCard(void) {
    return &s_runtime.memoryCard;
}

BgmSelect *SceneRuntimeBgmSelect(void) {
    return &s_runtime.bgmSelect;
}

const AssetLoadTransaction *SceneRuntimeAssetResult(AssetRequestType request) {
    return AssetLoadTransactionResult(request, s_runtime.assetGeneration);
}

const AssetLoadTransaction *SceneRuntimeActiveAssetResult(void) {
    return AssetLoadTransactionCurrentResult(s_runtime.assetGeneration);
}
