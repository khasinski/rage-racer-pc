#include <psyz/gpu.h>
#include <string.h>

#include "game/audio.h"
#include "game/input_internal.h"
#include "game/memcard.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/state.h"

enum {
    DEFAULT_RENDER_OT_SHIFT = 5,
};

static void FinalizeBootCamera(void) {
    g_Camera.view.x = 0;
    g_Camera.view.angleX = 0x100;
    g_Camera.view.angleY = 0;
    g_Camera.view.angleZ = 0;
    SetCameraRotMatrix(&g_RenderState, &g_Camera.view);
}

void InitSubsystems(void) {
    /* Keep this order explicit: save defaults also applies audio settings,
     * while the camera matrix must see the final boot view. */
    InitSoundRuntime();

    ResetGraph(0);
    SetGraphDebug(0);
    SetDispMask(0);
    InitGeom();

    GameInitPad();
    RestartMemoryCard();

    g_MirrorMode = 0;
    InitRecordTables();
    InitRenderState(DEFAULT_RENDER_OT_SHIFT);
    InitSaveDefaults();
    g_Camera.view.y = -64;
    g_Camera.view.z = -256;
    g_ExtraGrandPrixUnlocked = 0;
    FinalizeBootCamera();
}
