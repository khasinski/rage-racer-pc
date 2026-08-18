#include "common.h"
#include "rage/compat.h"
#ifdef __psyz
#include <psyz/gpu.h>
#endif
#include <stdio.h>
#include "game/asset.h"
#include "game/audio.h"
#include "game/cd.h"
#include "game/memcard.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "game/game_runtime.h"
#include "psyq/cd.h"
#include "psyq/gpu.h"
#include "psyq/kernel.h"
#include "psyq/snd.h"
/*
 * One-shot boot chain called from MainLoop: sequencer, sound runtime, GPU
 * and DMA, the pad, then the persistent settings block reset to its defaults
 * (NeGcon uncalibrated, both button mappings on preset 0) and the scratchpad
 * camera block primed before the first frame.
 */
void InitSubsystems(void) {
    ssinit();
    InitSoundRuntime();
    ResetGraph(0);
    SetGraphDebug(0);
    SetDispMask(0);
    g_ScreenOffsetY.value = 0;
    g_ScreenOffsetX.value = 0;
    SetDMAInterruptState(1);
    InitGeom();
    GameInitPad();
    RestartMemoryCard();
    g_NegconSteerPlay = 1;
    g_PadMappingIndex = 0;
    g_NegconMappingIndex = 0;
    g_NegconSteerNeutral = 0;
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    g_NegconMaxTwist = 0;
    g_PadErrorState = PAD_ERROR_STATE_NONE;
    g_PadValidateCountdown = 0x21;
    g_PadErrorHoldBits = 0;
    g_MirrorMode = 0;
    ResetReplayFrameCounts();
    ApplyPadButtonMapping();
    InitRecordTables();
    InitRenderState(5);
    InitSaveDefaults();
    RENDER_VIEW_Y = -64;
    RENDER_VIEW_Z = -256;
    g_ExtraGrandPrixUnlocked = 0;
    RENDER_VIEW_X = 0;
    RENDER_VIEW_ANGLE_X = 0x100;
    RENDER_VIEW_ANGLE_Y = 0;
    RENDER_VIEW_ANGLE_Z = 0;
    SetCameraRotMatrix();
}


/* Scene handlers, indexed by g_SceneId. */

/*
 * The PS-EXE `main`. Boots the subsystems, then never returns: each pass picks
 * the frame context, resets its two ordering tables and the scratchpad packet
 * cursor, runs the CD / sequencer / loader services and the current scene
 * handler, waits for the frame deadline, swaps the display and refreshes the
 * pad.
 */
static void PrepareGameFrame(void *user) {
    s32 parity = g_FrameCounter & 1;
    u8 *frame = g_FrameContexts[parity].bytes;
    GameFrameContextAddress frameAddress;
    GameFrameContextAddress drawBuffer;
    (void)user;

    g_DrawBuffer = frame;
    g_FrameParity = parity;
    frameAddress.bytes = frame;
    RENDER_OT_BASE_AS(OT_TYPE) = frameAddress.context->layout.orderingTables[0];
    RENDER_PRIM_CURSOR_AS(u8) = frameAddress.context->layout.primitiveBuffer;
    ClearOTagR(frameAddress.context->layout.orderingTables[0], GAME_FRAME_OT_LENGTH);
    drawBuffer.bytes = g_DrawBuffer;
    ClearOTagR(drawBuffer.context->layout.orderingTables[1], GAME_FRAME_OT_LENGTH);
}

static void ServiceGameSystems(void *user) {
    (void)user;
    TickCdAudio();
    TickSequenceAudio();
    ServiceAssetLoad();
    AdvanceSaveHeaderCounter();
}

static void BeforeGameScene(void *user) {
    (void)user;
#ifdef __psyz
    RagePortBeforeSceneHandler();
#endif
}

static void AfterGameScene(void *user) {
    (void)user;
#ifdef __psyz
    RagePortAfterSceneHandler();
#endif
}

static void PresentGameFrame(void *user) {
    s32 frameLimit;
    s32 elapsed;
    s32 ticks;
    GameFrameContextAddress drawBuffer;
    (void)user;

    DrawSync(0);
    StepTrackTextureSwap();
    frameLimit = g_FrameSyncThreshold;
    while (VSync(1) < frameLimit) {
#ifdef __psyz
        RagePortDuringFrameWait(frameLimit);
#endif
    }
    elapsed = VSync(1);
    ticks = g_GameClock + 1;
    g_GameClock = ticks + elapsed / 256;
    VSync(0);
#ifdef __psyz
    Psyz_GpuTraceContext(g_SceneId, g_SceneTimer);
#endif
    drawBuffer.bytes = g_DrawBuffer;
    PutDrawEnv(&drawBuffer.context->environment.draw);
    PutDispEnv(&drawBuffer.context->environment.display);
    DrawOTag(&drawBuffer.context->layout.orderingTables[0][GAME_FRAME_OT_LENGTH - 1]);
    DrawOTag(&drawBuffer.context->layout.orderingTables[1][GAME_FRAME_OT_LENGTH - 1]);
    RagePortSampleAnalogPad();
    UpdatePadState();
    g_FrameCounter = g_FrameCounter + 1;
}

static s32 ShouldExitGame(void *user) {
    (void)user;
    return RagePortShouldExit(g_FrameCounter);
}

void MainLoop(void) {
    GameRuntime runtime;
    const GameRuntimeServices services = {
        0, PrepareGameFrame, ServiceGameSystems, BeforeGameScene,
        AfterGameScene, PresentGameFrame, ShouldExitGame};

    __main();
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
    g_SceneId = SCENE_BOOT_LOGO;
    GameRuntimeInit(&runtime, &g_SceneId, &g_SceneTimer, g_SceneHandlers,
                    SCENE_COUNT, &services);
    RequestBootAssets();
    g_GameClock = 0;
    g_FrameCounter = 0;
    for (;;) {
        GameRuntimeStepResult result = GameRuntimeStep(&runtime);
        if (result == GAME_RUNTIME_INVALID_SCENE) {
            fprintf(stderr, "rage-port: invalid scene id %d\n", g_SceneId);
            return;
        }
        if (result == GAME_RUNTIME_EXIT) return;
    }
}
