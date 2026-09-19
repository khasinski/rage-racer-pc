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
const AssetLoadTransaction *AssetLoadTransactionCurrentResult(u32 generation) {
    return generation == s_assets.generation && s_assets.complete
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
    /* The fade survives: the outgoing entry scene arms it for this one. */
    CHECK(g_SceneTimer == 0 && g_FadeLevel == 99 && g_FadeStep == 99 &&
          g_FrameSyncThreshold == 99 && g_CameraCarIndex == 99 &&
          g_CdTrackEnded == 0);
    g_SceneTimer = 11;
    g_FadeLevel = 12;
    g_FadeStep = -4;
    g_FrameSyncThreshold = 128;
    g_CameraCarIndex = 3;
    runtime = SceneRuntimeCurrent();
    CHECK(runtime->scene == GAME_SCENE_BGM_SELECT && runtime->generation == 1 &&
          runtime->assetGeneration == 7);

    s_assets = (AssetLoadTransaction){
        .request = ASSET_REQUEST_SELECT_BGM,
        .generation = 7,
        .complete = 1,
    };
    CHECK(SceneRuntimeAssetResult(ASSET_REQUEST_SELECT_BGM) == &s_assets);
    CHECK(SceneRuntimeActiveAssetResult() == &s_assets);

    g_SceneId = GAME_SCENE_MENU;
    s_assetGeneration = 8;
    SceneRuntimeBeforeDispatch(g_SceneId);
    runtime = SceneRuntimeCurrent();
    CHECK(runtime->scene == GAME_SCENE_MENU && runtime->generation == 2 &&
          runtime->assetGeneration == 8 &&
          SceneRuntimeAssetResult(ASSET_REQUEST_SELECT_BGM) == NULL);

    g_SceneId = GAME_SCENE_ENTER_MEMORY_CARD;
    SceneRuntimeBeforeDispatch(g_SceneId);
    SceneRuntimeMemoryCard()->action.state = 7;
    SceneRuntimeMemoryCard()->poll.ticks = 8;
    SceneRuntimeMemoryCard()->slots.lastSlot = 2;
    g_SceneId = GAME_SCENE_MEMORY_CARD;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeMemoryCard()->action.state == 7 &&
          SceneRuntimeMemoryCard()->poll.ticks == 8 &&
          SceneRuntimeMemoryCard()->slots.lastSlot == 2);

    g_SceneId = GAME_SCENE_MENU;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeMemoryCard()->action.state == 0 &&
          SceneRuntimeMemoryCard()->poll.ticks == 0 &&
          SceneRuntimeMemoryCard()->slots.lastSlot == 0);

    g_SceneId = GAME_SCENE_ENTER_BGM_SELECT;
    SceneRuntimeBeforeDispatch(g_SceneId);
    SceneRuntimeBgmSelect()->cursor = 2;
    SceneRuntimeBgmSelect()->track = 6;
    SceneRuntimeBgmSelect()->randomPlay = 1;
    g_SceneId = GAME_SCENE_BGM_SELECT;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeBgmSelect()->cursor == 2 &&
          SceneRuntimeBgmSelect()->track == 6 &&
          SceneRuntimeBgmSelect()->randomPlay == 1);

    g_SceneId = GAME_SCENE_MENU;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeBgmSelect()->cursor == 0 &&
          SceneRuntimeBgmSelect()->track == 0 &&
          SceneRuntimeBgmSelect()->randomPlay == 0);

    g_SceneId = GAME_SCENE_ENTER_RECORD_ENTRY;
    SceneRuntimeBeforeDispatch(g_SceneId);
    SceneRuntimeRecordEntry()->step = RECORD_ENTRY_STATE_EDIT_LAP_NAME;
    SceneRuntimeRecordEntry()->nameCursor = 3;
    SceneRuntimeRecordEntry()->rankingRow = 1;
    g_SceneId = GAME_SCENE_RECORD_ENTRY;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeRecordEntry()->step ==
              RECORD_ENTRY_STATE_EDIT_LAP_NAME &&
          SceneRuntimeRecordEntry()->nameCursor == 3 &&
          SceneRuntimeRecordEntry()->rankingRow == 1);

    g_SceneId = GAME_SCENE_MENU;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeRecordEntry()->step == 0 &&
          SceneRuntimeRecordEntry()->nameCursor == 0 &&
          SceneRuntimeRecordEntry()->rankingRow == 0);

    g_SceneId = GAME_SCENE_ENTER_PROLOGUE;
    SceneRuntimeBeforeDispatch(g_SceneId);
    SceneRuntimePrologue()->step = PROLOGUE_STEP_ACTIVE;
    SceneRuntimePrologue()->cameraCut = 4;
    g_SceneId = GAME_SCENE_PROLOGUE;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimePrologue()->step == PROLOGUE_STEP_ACTIVE &&
          SceneRuntimePrologue()->cameraCut == 4);

    g_SceneId = GAME_SCENE_MENU;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimePrologue()->step == 0 &&
          SceneRuntimePrologue()->cameraCut == 0);

    g_SceneId = GAME_SCENE_ENTER_ATTRACT_DEMO;
    SceneRuntimeBeforeDispatch(g_SceneId);
    SceneRuntimeAttractDemo()->step = ATTRACT_DEMO_STEP_RACE;
    g_SceneId = GAME_SCENE_ATTRACT_DEMO;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeAttractDemo()->step == ATTRACT_DEMO_STEP_RACE);

    g_SceneId = GAME_SCENE_ENTER_LOST_RACE;
    SceneRuntimeBeforeDispatch(g_SceneId);
    SceneRuntimeLostRace()->choice = 1;
    g_SceneId = GAME_SCENE_LOST_RACE;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeLostRace()->choice == 1);

    g_SceneId = GAME_SCENE_MENU;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeAttractDemo()->step == 0 &&
          SceneRuntimeLostRace()->choice == 0);

    g_SceneId = GAME_SCENE_ENTER_RACE;
    SceneRuntimeBeforeDispatch(g_SceneId);
    SceneRuntimeRace()->pauseDelay = 30;
    SceneRuntimeRace()->optionCursor = 2;
    SceneRuntimeRace()->timeRemaining = 1234;
    SceneRuntimeRace()->fadeTimer = 12;
    SceneRuntimeRace()->timing.sectorIndex = 2;
    SceneRuntimeRace()->timing.splitTargetTime = 4567;
    SceneRuntimeRace()->timing.lapTime = 2345;
    SceneRuntimeRace()->timing.bestLap = 6789;
    g_SceneId = GAME_SCENE_RACE;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeRace()->pauseDelay == 30 &&
          SceneRuntimeRace()->optionCursor == 2 &&
          SceneRuntimeRace()->timeRemaining == 1234 &&
          SceneRuntimeRace()->fadeTimer == 12 &&
          SceneRuntimeRace()->timing.sectorIndex == 2 &&
          SceneRuntimeRace()->timing.splitTargetTime == 4567 &&
          SceneRuntimeRace()->timing.lapTime == 2345 &&
          SceneRuntimeRace()->timing.bestLap == 6789);

    g_SceneId = GAME_SCENE_MENU;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeRace()->pauseDelay == 0 &&
          SceneRuntimeRace()->optionCursor == 0 &&
          SceneRuntimeRace()->timeRemaining == 0 &&
          SceneRuntimeRace()->fadeTimer == 0 &&
          SceneRuntimeRace()->timing.sectorIndex == 0 &&
          SceneRuntimeRace()->timing.splitTargetTime == 0 &&
          SceneRuntimeRace()->timing.lapTime == 0 &&
          SceneRuntimeRace()->timing.bestLap == 0);

    g_SceneId = GAME_SCENE_BOOT_LOGO;
    SceneRuntimeBeforeDispatch(g_SceneId);
    CHECK(SceneRuntimeBootLogo()->state == BOOT_LOGO_STATE_FADE_IN &&
          SceneRuntimeBootLogo()->timer == 0 &&
          SceneRuntimeBootLogo()->holdTimer == BOOT_LOGO_INITIAL_HOLD_FRAMES);

    puts("scene runtime scopes transition state and asset results to one scene");
    return 0;
}
