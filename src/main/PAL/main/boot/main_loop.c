#include "game/diagnostics.h"
#include <psyz/gpu.h>
#include "game/asset.h"
#include "game/audio.h"
#include "game/boot_internal.h"
#include "game/cd.h"
#include "game/memcard.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "psyq/cd.h"
#include "psyq/kernel.h"

static void InitializeGameLoop(void) {
    KernelCallbackSlot3();
    BiosSetMemSize(2);
    CdInit();
    InitSubsystems();
    InitAssetSystem();
    ResetGraph(3);
    InitCdAudio();
    g_FrameSyncThreshold = 0x80;
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_SceneTimer = 0;
    g_SceneId = 1;
    RequestBootAssets();
    g_GameClock = 0;
    g_FrameCounter = 0;
}

static GameFrameContext *BeginGameFrame(void) {
    s32 parity = g_FrameCounter & 1;
    GameFrameContext *frame = &g_FrameContexts[parity];

    g_DrawBuffer = frame;
    g_FrameParity = parity;
    RENDER_OT_BASE = frame->layout.orderingTables[0];
    g_RenderState.packetCursor = frame->layout.primitiveBuffer;
    GameClearOrderingTable(frame->layout.orderingTables[0],
                           GAME_FRAME_OT_LENGTH);
    GameClearOrderingTable(frame->layout.orderingTables[1],
                           GAME_FRAME_OT_LENGTH);
    return frame;
}

static void ServiceGameFrame(void) {
    TickCdAudio();
    TickSequenceAudio();
    ServiceAssetLoad();
    AdvanceSaveHeaderCounter();
    PortBeforeSceneHandler();
    DispatchCurrentScene();
    PortAfterSceneHandler();
    DrawSync(0);
    StepTrackTextureSwap();
}

static s32 WaitForFrameDeadline(void) {
    s32 frameLimit = g_FrameSyncThreshold;

    while (VSync(1) < frameLimit) {
        PortDuringFrameWait(frameLimit);
    }
    return VSync(1);
}

static void PresentGameFrame(GameFrameContext *frame) {
    VSync(0);
    Psyz_GpuTraceContext(g_SceneId, g_SceneTimer);
    PutDrawEnv(&frame->environment.draw);
    PutDispEnv(&frame->environment.display);
    GameDrawOrderingTable(
        &frame->layout.orderingTables[0][GAME_FRAME_OT_LENGTH - 1]);
    GameDrawOrderingTable(
        &frame->layout.orderingTables[1][GAME_FRAME_OT_LENGTH - 1]);
    PortSampleAnalogPad();
    UpdatePadState();
}

/* Boots the game and runs frames until the host requests shutdown. */
void MainLoop(void) {
    InitializeGameLoop();

    for (;;) {
        GameFrameContext *frame = BeginGameFrame();
        s32 elapsed;

        ServiceGameFrame();
        elapsed = WaitForFrameDeadline();
        g_GameClock += 1 + elapsed / 256;
        PresentGameFrame(frame);
        g_FrameCounter++;
        if (PortShouldExit(g_FrameCounter)) {
            return;
        }
    }
}
