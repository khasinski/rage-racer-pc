#ifndef GAME_SCENE_RUNTIME_H
#define GAME_SCENE_RUNTIME_H

#include "game/asset.h"
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
    /* The scene that most recently yielded control to `scene`. A value of
     * -1 denotes the first scene in this process. */
    s32 previousScene;
    /* A handler may still assign g_SceneId directly. Record that requested
     * destination at the end of its frame so new code need not infer it from
     * unrelated globals. -1 means the scene remains active. */
    s32 nextScene;
    u32 generation;
    u32 assetGeneration;
    SceneTransitionRuntime transition;
} SceneRuntime;

void SceneRuntimeBeforeDispatch(s32 scene);
void SceneRuntimeAfterDispatch(s32 scene);
/* Request a new top-level scene through one consistent legacy adapter. The
 * next dispatch records the transition and resets scene-local scratch state. */
void SceneRuntimeRequestScene(s32 scene);
/* Use when retail behavior requires a nonzero initial scene timer, such as a
 * fade phase that continues into its destination handler. */
void SceneRuntimeRequestSceneWithTimer(s32 scene, s32 timer);
const SceneRuntime *SceneRuntimeCurrent(void);
/* True after a handler requested a different scene, until that destination
 * begins its first dispatch. */
s32 SceneRuntimeHasPendingTransition(void);
/* A result belongs to a scene only when it was the active transaction at the
 * point that scene began. */
const AssetLoadTransaction *SceneRuntimeAssetResult(AssetRequestType request);
const AssetLoadTransaction *SceneRuntimeActiveAssetResult(void);

#endif
