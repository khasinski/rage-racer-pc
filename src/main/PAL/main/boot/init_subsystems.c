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
    DEFAULT_PAD_VALIDATION_FRAMES = 0x21,
    DEFAULT_RENDER_OT_SHIFT = 5,
};

static void ResetInputDefaults(void) {
    memset(&g_PadState, 0, sizeof(g_PadState));
    g_PadType = 0;
    g_PadPrevHeld = 0;
    g_PadHeld = 0;
    g_PadPressed = 0;
    g_PadPressedRepeat = 0;
    g_PadRepeatTimer = 0;
    g_NegconAnalogI = 0;
    g_NegconAnalogII = 0;
    g_NegconAnalogL = 0;
    g_NegconSteer = 0;

    g_NegconSteerPlay = 1;
    g_PadMappingIndex = 0;
    g_NegconMappingIndex = 0;
    g_NegconSteerNeutral = 0;
    g_NegconNeutralI = 0;
    g_NegconNeutralII = 0;
    g_NegconNeutralL = 0;
    g_NegconMaxTwist = 0;
    g_PadValidation.error = PAD_ERROR_STATE_NONE;
    g_PadValidation.countdown = DEFAULT_PAD_VALIDATION_FRAMES;
    g_PadValidation.holdBits = 0;
    g_PadValidation.lastValidType = PAD_TYPE_DIGITAL;
}

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
    ResetInputDefaults();

    g_MirrorMode = 0;
    LoadPadButtonMapping(g_PadMappingIndex, g_NegconMappingIndex);
    InitRecordTables();
    InitRenderState(DEFAULT_RENDER_OT_SHIFT);
    InitSaveDefaults();
    g_Camera.view.y = -64;
    g_Camera.view.z = -256;
    g_ExtraGrandPrixUnlocked = 0;
    FinalizeBootCamera();
}
