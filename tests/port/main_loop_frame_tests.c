#include "common.h"
#include "game/render_internal.h"
#include "game/state.h"
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

static s32 s_assetServices;
static s32 s_audioTicks;
static s32 s_clearCalls;
static s32 s_dispatchCalls;
static s32 s_drawCalls;
static s32 s_padUpdates;
static s32 s_presentCalls;
static s32 s_saveTicks;
static s32 s_textureTicks;

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
s32 RequestBootAssets(void) { return 1; }
void TickCdAudio(void) { s_audioTicks++; }
void TickSequenceAudio(void) { s_audioTicks++; }
void ServiceAssetLoad(void) { s_assetServices++; }
void AdvanceSaveHeaderCounter(void) { s_saveTicks++; }
void PortBeforeSceneHandler(void) {
    g_GameClock = INT_MAX;
    g_FrameCounter = INT_MAX;
}
void DispatchCurrentScene(void) { s_dispatchCalls++; }
void PortAfterSceneHandler(void) {}
int DrawSync(int mode) {
    (void)mode;
    return 0;
}
void StepTrackTextureSwap(void) { s_textureTicks++; }
int VSync(int mode) {
    (void)mode;
    return 128;
}
void PortDuringFrameWait(int frameLimit) { (void)frameLimit; }
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
void UpdatePadState(void) { s_padUpdates++; }
int PortShouldExit(int frameNumber) {
    return frameNumber == INT_MIN;
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

    CHECK(g_DrawBuffer == &g_FrameContexts[0] && g_FrameParity == 0);
    CHECK(g_RenderState.packetCursor ==
          g_FrameContexts[0].layout.primitiveBuffer);
    CHECK(s_clearCalls == 2);
    CHECK(s_audioTicks == 2 && s_assetServices == 1 && s_saveTicks == 1);
    CHECK(s_dispatchCalls == 1 && s_textureTicks == 1);
    CHECK(s_presentCalls == 2 && s_drawCalls == 2 && s_padUpdates == 1);
    CHECK(g_GameClock == INT_MIN && g_FrameCounter == INT_MIN);

    puts("main loop frame tests passed");
    return 0;
}
