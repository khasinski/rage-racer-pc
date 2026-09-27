#include "common.h"
#include "game/render_internal.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/boot_internal.h"
#include "psyq/gpu.h"

#include <limits.h>
#include <stdio.h>

GameFrameContext g_FrameContexts[2];
GameFrameContext *g_DrawBuffer;
s32 g_FrameCounter;
s32 g_FrameParity;
s32 g_FrameSyncThreshold;
s32 g_GameClock;
GameRenderState g_RenderState;
s32 g_SceneId;
s32 g_SceneTimer;
u16 g_PadPressed, g_PadPressedRepeat, g_PadHeld;

static s32 s_assetServices;
static s32 s_audioTicks;
static s32 s_clearCalls;
static s32 s_dispatchCalls;
static s32 s_drawCalls;
static s32 s_padUpdates;
static s32 s_presentCalls;
static s32 s_saveTicks;
static s32 s_textureTicks;
static s32 s_transferPublication, s_transferOrder;
static s32 s_drawSyncCalls, s_vsyncCalls;
static s32 s_bootSceneAtRequest;
static s32 s_requestMultiplayer, s_multiplayerCalls, s_returnScene;
static s32 s_bootRequests;
static s32 s_returnInputClean;
static int s_multiplayerResult;
static int s_textCalls, s_backgroundCalls;

long CdInit(void) { return 1; }
void InitSubsystems(void) {}
void InitAssetSystem(void) {}
s32 ResetGraph(s32 mode) { return mode; }
void InitCdAudio(void) {}
void SetDispMask(int enabled) { (void)enabled; }
void SetupDisplay240(s32 r, s32 g, s32 b) {
    (void)r;
    (void)g;
    (void)b;
}
s32 RequestBootAssets(void) {
    s_bootRequests++;
    s_bootSceneAtRequest = g_SceneId;
    return 1;
}
void TickCdAudio(void) { s_audioTicks++; }
void TickSequenceAudio(void) { s_audioTicks++; }
void ServiceAssetLoad(void) { s_assetServices++; }
void AdvanceSaveHeaderCounter(void) { s_saveTicks++; }
void PortBeforeSceneHandler(void) {
    g_GameClock = INT_MAX;
    g_FrameCounter = INT_MAX;
}
void DispatchCurrentScene(void) {
    s_dispatchCalls++;
    if (s_requestMultiplayer && s_dispatchCalls == 1) g_SceneId = GAME_SCENE_MULTIPLAYER;
    else if (s_requestMultiplayer) {
        s_returnScene = g_SceneId;
        s_returnInputClean = g_PadPressed == 0 && g_PadPressedRepeat == 0 &&
            g_PadHeld == (PAD_START | PAD_SELECT);
    }
}
int PortRunMultiplayer(int interactive) {
    if (!interactive) return 0;
    s_multiplayerCalls++;
    g_PadHeld = g_PadPressed = g_PadPressedRepeat = PAD_START | PAD_SELECT;
    return s_multiplayerResult;
}
void DrawText8x8(s32 x, s32 y, const char *text, s32 clut) {
    (void)x; (void)y; (void)text; (void)clut;
    s_textCalls++;
}
void DrawSolidRect(GameOrderingTableEntry *ot, s32 x, s32 y, s32 w, s32 h,
                   s32 r, s32 g, s32 b, s32 mode) {
    (void)ot; (void)x; (void)y; (void)w; (void)h;
    (void)r; (void)g; (void)b; (void)mode;
    s_backgroundCalls++;
}
void PortAfterSceneHandler(void) {}
int DrawSync(int mode) {
    (void)mode;
    s_drawSyncCalls++;
    return 0;
}
void StepTrackTextureSwap(void) { s_textureTicks++; }
void PortAfterFrameTransfers(void) {
    s_transferPublication++;
    s_transferOrder = s_textureTicks == 1 && s_drawSyncCalls == 1 &&
        s_presentCalls == 0 && s_vsyncCalls == 0;
}
int VSync(int mode) {
    (void)mode;
    s_vsyncCalls++;
    return 128;
}
void PortDuringFrameWait(int frameLimit) { (void)frameLimit; }
void PortProfileFramePhase(const char *phase) { (void)phase; }
void Psyz_GpuTraceContext(int scene, int timer) {
    (void)scene;
    (void)timer;
}
DrawEnv *PutDrawEnv(DrawEnv *env) {
    s_presentCalls++;
    return env;
}
DispEnv *PutDispEnv(DispEnv *env) {
    s_presentCalls++;
    return env;
}
void DrawOTag(OT_TYPE *ot) {
    (void)ot;
    s_drawCalls++;
}
void PortSampleAnalogPad(void) {}
void PortUpdateForceFeedback(void) {}
void UpdatePadState(void) { s_padUpdates++; }
int PortShouldExit(int frameNumber) {
    return frameNumber == INT_MIN && (!s_requestMultiplayer || s_multiplayerCalls);
}
OT_TYPE *ClearOTagR(OT_TYPE *ot, int count) {
    (void)count;
    s_clearCalls++;
    return ot;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    MainLoop();
    CHECK(s_transferPublication == 1 && s_transferOrder);

    CHECK(g_DrawBuffer == &g_FrameContexts[0] && g_FrameParity == 0);
    CHECK(g_RenderState.draw.packetCursor ==
          g_FrameContexts[0].layout.primitiveBuffer);
    CHECK(s_clearCalls == 2);
    CHECK(s_audioTicks == 2 && s_assetServices == 1 && s_saveTicks == 1);
    CHECK(s_dispatchCalls == 1 && s_textureTicks == 1);
    CHECK(s_bootSceneAtRequest == GAME_SCENE_BOOT_LOGO);
    CHECK(s_presentCalls == 2 && s_drawCalls == 2 && s_padUpdates == 1);
    CHECK(g_GameClock == INT_MIN && g_FrameCounter == INT_MIN);

    s_requestMultiplayer = 1;
    for (s_multiplayerResult = 0; s_multiplayerResult <= 1; ++s_multiplayerResult) {
        s_dispatchCalls = 0;
        s_multiplayerCalls = 0;
        s_returnInputClean = 0;
        s_returnScene = -1;
        const s32 boots = s_bootRequests;
        MainLoop();
        CHECK(s_multiplayerCalls == 1);
        CHECK(s_returnScene == GAME_SCENE_ENTER_TITLE);
        CHECK(s_returnInputClean);
        CHECK(g_SceneTimer == 0);
        CHECK(s_dispatchCalls == 2);
        CHECK(s_bootRequests == boots + 1); /* Returning does not boot again. */
    }

    const s32 scenes = s_dispatchCalls, saves = s_saveTicks, assets = s_assetServices;
    const s32 audio = s_audioTicks, clock = g_GameClock, scene = g_SceneId, timer = g_SceneTimer;
    const s32 frames = g_FrameCounter, input = s_padUpdates;
    DrawHostMenuFrame("MULTIPLAYER", "CAR 01 / 32", "SELECT", "READY");
    CHECK(s_textCalls == 4 && s_backgroundCalls == 1);
    CHECK(s_dispatchCalls == scenes && s_saveTicks == saves && s_assetServices == assets);
    CHECK(s_audioTicks == audio && g_GameClock == clock);
    CHECK(g_SceneId == scene && g_SceneTimer == timer);
    CHECK(g_FrameCounter == frames + 1 && s_padUpdates == input + 1);

    puts("main loop frame tests passed");
    return 0;
}
