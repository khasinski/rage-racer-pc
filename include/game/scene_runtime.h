#ifndef GAME_SCENE_RUNTIME_H
#define GAME_SCENE_RUNTIME_H

#include "game/asset.h"
#include "game/bgm_select_state.h"
#include "game/memcard_types.h"
#include "game/memcard_state.h"
#include "game/scene.h"

/*
 * Port-owned scene lifetime.  Retail handlers still use their recovered
 * globals, but cross-scene resources are attached to this generation rather
 * than inferred from whichever globals the previous handler happened to use.
 */
typedef struct SceneTransitionRuntime {
    s32 timer;
    s32 fadeLevel;
    s32 fadeStep;
    s32 frameSyncThreshold;
    s32 cameraCarIndex;
} SceneTransitionRuntime;

typedef struct SceneRuntime {
    s32 scene;
    u32 generation;
    u32 assetGeneration;
    SceneTransitionRuntime transition;
    MemoryCardSession memoryCard;
    BgmSelect bgmSelect;
} SceneRuntime;

void SceneRuntimeBeforeDispatch(s32 scene);
void SceneRuntimeAfterDispatch(s32 scene);
const SceneRuntime *SceneRuntimeCurrent(void);
MemoryCardSession *SceneRuntimeMemoryCard(void);
BgmSelect *SceneRuntimeBgmSelect(void);
/* A result belongs to a scene only when it was the active transaction at the
 * point that scene began. */
const AssetLoadTransaction *SceneRuntimeAssetResult(AssetRequestType request);
const AssetLoadTransaction *SceneRuntimeActiveAssetResult(void);

#endif
