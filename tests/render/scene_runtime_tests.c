#include "common.h"
#include "game/scene_runtime.h"
#include "game/state.h"

#include <stdio.h>

s32 g_SceneId;
s32 g_SceneTimer;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_FrameSyncThreshold;
s32 g_CameraCarIndex;
s32 g_CdTrackEnded;
static u32 s_assetGeneration;
static AssetLoadTransaction s_assets;

u32 AssetLoadTransactionGeneration(void) { return s_assetGeneration; }
const AssetLoadTransaction *AssetLoadTransactionResult(
    AssetRequestType request, u32 generation) {
    return request == s_assets.request && generation == s_assets.generation &&
                   s_assets.complete
               ? &s_assets
               : NULL;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    const SceneRuntime *runtime;

    s_assetGeneration = 7;
    g_SceneTimer = 99;
    g_FadeLevel = 99;
    g_FadeStep = 99;
    g_FrameSyncThreshold = 99;
    g_CameraCarIndex = 99;
    g_CdTrackEnded = 1;
    g_SceneId = GAME_SCENE_BGM_SELECT;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(g_SceneTimer == 0 && g_FadeLevel == 0 && g_FadeStep == 0 &&
          g_FrameSyncThreshold == 0x80 && g_CameraCarIndex == 0 &&
          g_CdTrackEnded == 0);
    g_SceneTimer = 11;
    g_FadeLevel = 12;
    g_FadeStep = -4;
    g_FrameSyncThreshold = 128;
    g_CameraCarIndex = 3;
    SceneRuntimeAfterDispatch(g_SceneId);
    runtime = SceneRuntimeCurrent();
    CHECK(runtime->scene == GAME_SCENE_BGM_SELECT && runtime->generation == 1 &&
          runtime->assetGeneration == 7 && runtime->transition.timer == 11 &&
          runtime->transition.fadeLevel == 12 && runtime->transition.fadeStep == -4 &&
          runtime->transition.frameSyncThreshold == 128 &&
          runtime->transition.cameraCarIndex == 3);

    s_assets = (AssetLoadTransaction){
        .request = ASSET_REQUEST_SELECT_BGM,
        .generation = 7,
        .complete = 1,
    };
    CHECK(SceneRuntimeAssetResult(ASSET_REQUEST_SELECT_BGM) == &s_assets);

    g_SceneId = GAME_SCENE_MENU;
    s_assetGeneration = 8;
    SceneRuntimeBeforeDispatch(g_SceneId);
    runtime = SceneRuntimeCurrent();
    CHECK(runtime->scene == GAME_SCENE_MENU && runtime->generation == 2 &&
          runtime->assetGeneration == 8 && runtime->transition.timer == 0 &&
          runtime->transition.fadeLevel == 0 &&
          SceneRuntimeAssetResult(ASSET_REQUEST_SELECT_BGM) == NULL);

    puts("scene runtime scopes transition state and asset results to one scene");
    return 0;
}
