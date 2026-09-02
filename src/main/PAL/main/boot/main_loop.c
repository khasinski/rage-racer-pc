#include "game/diagnostics.h"
#include <psyz/gpu.h>
#include "game/asset.h"
#include "game/audio.h"
#include "game/cd.h"
#include "game/memcard.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/input_internal.h"
#include "psyq/cd.h"
#include "psyq/kernel.h"

/* Scene handlers, indexed by g_SceneId. */

/*
 * The PS-EXE `main`. Boots the subsystems, then never returns: each pass picks
 * the frame context, resets its two ordering tables and the render state packet
 * cursor, runs the CD / sequencer / loader services and the current scene
 * handler, waits for the frame deadline, swaps the display and refreshes the
 * pad.
 */
void MainLoop(void) {
    s32 frameLimit;
    s32 elapsed;
    s32 ticks;

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
    for (;;) {
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
        TickCdAudio();
        TickSequenceAudio();
        ServiceAssetLoad();
        AdvanceSaveHeaderCounter();
        PortBeforeSceneHandler();
        /* The scene id is written from several dozen places, including the
         * host's own scenario control, and six of the forty slots are empty.
         * An id that lands on one of those used to be an indirect call
         * through a null pointer, which says nothing about who set it. */
        if ((unsigned)g_SceneId >=
                sizeof(g_SceneHandlers) / sizeof(g_SceneHandlers[0]) ||
            g_SceneHandlers[g_SceneId] == NULL) {
            Trace("scene-unhandled", "id=%d timer=%d", g_SceneId, g_SceneTimer);
        } else {
            g_SceneHandlers[g_SceneId]();
        }
        PortAfterSceneHandler();
        DrawSync(0);
        StepTrackTextureSwap();
        frameLimit = g_FrameSyncThreshold;
        while (VSync(1) < frameLimit) {
            PortDuringFrameWait(frameLimit);
        }
        elapsed = VSync(1);
        ticks = g_GameClock + 1;
        g_GameClock = ticks + elapsed / 256;
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
        g_FrameCounter++;
        if (PortShouldExit(g_FrameCounter)) {
            return;
        }
    }
}
