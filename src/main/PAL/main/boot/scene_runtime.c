#include "game/scene_runtime.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"

#include <string.h>

static SceneRuntime s_runtime = {.scene = -1};

static int ContinuesScene(s32 previous, s32 scene) {
    switch (scene) {
    case GAME_SCENE_MEMORY_CARD:
        return previous == GAME_SCENE_ENTER_MEMORY_CARD ||
               previous == GAME_SCENE_ENTER_MEMORY_CARD_LOAD;
    case GAME_SCENE_BGM_SELECT:
        return previous == GAME_SCENE_ENTER_BGM_SELECT;
    case GAME_SCENE_RECORD_ENTRY:
        return previous == GAME_SCENE_ENTER_RECORD_ENTRY;
    case GAME_SCENE_PROLOGUE:
        return previous == GAME_SCENE_ENTER_PROLOGUE;
    case GAME_SCENE_ATTRACT_DEMO:
        return previous == GAME_SCENE_ENTER_ATTRACT_DEMO;
    case GAME_SCENE_LOST_RACE:
        return previous == GAME_SCENE_ENTER_LOST_RACE;
    case GAME_SCENE_RACE:
        return previous == GAME_SCENE_ENTER_RACE;
    default:
        return 0;
    }
}

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
        const int continues = ContinuesScene(s_runtime.scene, scene);
        SceneState state = s_runtime.state;
        u32 generation = s_runtime.generation + 1;

        if (generation == 0) generation = 1;
        memset(&s_runtime, 0, sizeof(s_runtime));
        s_runtime.scene = scene;
        s_runtime.generation = generation;
        s_runtime.assetGeneration = AssetLoadTransactionGeneration();
        if (continues) s_runtime.state = state;
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
    return &s_runtime.state.memoryCard;
}

BgmSelect *SceneRuntimeBgmSelect(void) {
    return &s_runtime.state.bgmSelect;
}

RecordEntry *SceneRuntimeRecordEntry(void) {
    return &s_runtime.state.recordEntry;
}

Prologue *SceneRuntimePrologue(void) {
    return &s_runtime.state.prologue;
}

AttractDemo *SceneRuntimeAttractDemo(void) {
    return &s_runtime.state.attractDemo;
}

LostRace *SceneRuntimeLostRace(void) {
    return &s_runtime.state.lostRace;
}

RaceScene *SceneRuntimeRace(void) {
    return &s_runtime.state.race;
}

const AssetLoadTransaction *SceneRuntimeAssetResult(AssetRequestType request) {
    return AssetLoadTransactionResult(request, s_runtime.assetGeneration);
}

const AssetLoadTransaction *SceneRuntimeActiveAssetResult(void) {
    return AssetLoadTransactionCurrentResult(s_runtime.assetGeneration);
}
