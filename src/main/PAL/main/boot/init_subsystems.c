#include <psyz/gpu.h>

#include "game/audio.h"
#include "game/input_internal.h"
#include "game/memcard.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/state.h"

enum {
    DEFAULT_PAD_VALIDATION_FRAMES = 0x21,
    DEFAULT_RENDER_OT_SHIFT = 5,
};

static void ResetInputDefaults(void) {
    g_NegconSteerPlay = 1;
    g_PadMappingIndex = 0;
    g_NegconMappingIndex = 0;
    g_NegconSteerNeutral = 0;
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    g_NegconMaxTwist = 0;
    g_PadErrorState = PAD_ERROR_STATE_NONE;
    g_PadValidateCountdown = DEFAULT_PAD_VALIDATION_FRAMES;
    g_PadErrorHoldBits = 0;
}

static void FinalizeBootCamera(void) {
    g_RenderState.viewX = 0;
    g_RenderState.viewAngleX = 0x100;
    g_RenderState.viewAngleY = 0;
    g_RenderState.viewAngleZ = 0;
    SetCameraRotMatrix();
}

void InitSubsystems(void) {
    /* Keep this order explicit: save defaults also applies audio settings,
     * while the camera matrix must see the final boot view. */
    InitSoundRuntime();

    ResetGraph(0);
    SetGraphDebug(0);
    SetDispMask(0);
    g_ScreenOffsetY = 0;
    g_ScreenOffsetX = 0;
    InitGeom();

    GameInitPad();
    RestartMemoryCard();
    ResetInputDefaults();

    g_MirrorMode = 0;
    ApplyPadButtonMapping();
    InitRecordTables();
    InitRenderState(DEFAULT_RENDER_OT_SHIFT);
    InitSaveDefaults();
    g_RenderState.viewY = -64;
    g_RenderState.viewZ = -256;
    g_ExtraGrandPrixUnlocked = 0;
    FinalizeBootCamera();
}
