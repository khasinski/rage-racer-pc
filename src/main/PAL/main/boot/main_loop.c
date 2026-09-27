#include "game/diagnostics.h"
#include <psyz/gpu.h>
#include "game/asset.h"
#include "game/audio.h"
#include "game/boot_internal.h"
#include "game/cd.h"
#include "game/memcard.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scene.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "psyq/cd.h"

static void InitializeGameLoop(void) {
    CdInit();
    InitSubsystems();
    InitAssetSystem();
    ResetGraph(3);
    InitCdAudio();
    g_FrameSyncThreshold = 0x80;
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_SceneTimer = 0;
    g_SceneId = GAME_SCENE_BOOT_LOGO;
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
    g_RenderState.draw.packetCursor = frame->layout.primitiveBuffer;
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
    PortProfileFramePhase("scene");
    DispatchCurrentScene();
    PortProfileFramePhase("publish");
    PortAfterSceneHandler();
    PortProfileFramePhase("draw_sync");
    DrawSync(0);
    PortProfileFramePhase("texture_swap");
    StepTrackTextureSwap();
    PortProfileFramePhase("presentation_snapshot");
    PortAfterFrameTransfers();
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
    PortUpdateForceFeedback();
    UpdatePadState();
}

void DrawHostMenuFrame(const char *title, const char *choice,
                       const char *controls, const char *status) {
    GameFrameContext *frame = BeginGameFrame();
    DrawText8x8(24, 64, title, 0x78CC);
    DrawText8x8(24, 96, choice, 0x78CC);
    DrawText8x8(24, 128, controls, 0x78CC);
    DrawText8x8(24, 160, status, 0x78CC);
    DrawSolidRect(GamePrimaryOrderingTable(0), 0, 0, 320, 480, 12, 18, 32, 0);
    DrawSync(0);
    PortAfterFrameTransfers();
    PresentGameFrame(frame);
    g_FrameCounter = (s32)((u32)g_FrameCounter + 1U);
}

/* Boots the game and runs frames until the host requests shutdown. */
void MainLoop(void) {
    InitializeGameLoop();

    for (;;) {
        if (g_SceneId == GAME_SCENE_MULTIPLAYER) {
            PortRunMultiplayer(1);
            /* Consume the session's exit edge, retaining held buttons so
             * returning to the title does not manufacture another press. */
            g_PadPressed = 0;
            g_PadPressedRepeat = 0;
            g_SceneId = GAME_SCENE_ENTER_TITLE;
            g_SceneTimer = 0;
            continue;
        }
        PortProfileFramePhase("service");
        GameFrameContext *frame = BeginGameFrame();
        s32 elapsed;

        ServiceGameFrame();
        PortProfileFramePhase("wait");
        elapsed = WaitForFrameDeadline();
        g_GameClock = (s32)((u32)g_GameClock + 1U +
                            (u32)(elapsed / 256));
        PortProfileFramePhase("present_ot");
        PresentGameFrame(frame);
        PortProfileFramePhase(NULL);
        g_FrameCounter = (s32)((u32)g_FrameCounter + 1U);
        if (PortShouldExit(g_FrameCounter)) {
            return;
        }
    }
}
