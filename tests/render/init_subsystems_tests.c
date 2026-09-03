#include "game/input_internal.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

ControllerMappingIndex g_PadMappingIndex;
ControllerMappingIndex g_NegconMappingIndex;
NegconCalibrationValue g_NegconMaxTwist;
NegconCalibrationValue g_NegconSteerPlay;
NegconCalibrationValue g_NegconSteerNeutral;
NegconCalibrationValue g_NegconNeutralI;
NegconCalibrationValue g_NegconNeutralII;
NegconCalibrationValue g_NegconNeutralL;
PadErrorState g_PadErrorState;
s32 g_PadValidateCountdown;
s32 g_PadErrorHoldBits;
s32 g_MirrorMode;
s16 g_ExtraGrandPrixUnlocked;
ScreenOffset g_ScreenOffsetX;
ScreenOffset g_ScreenOffsetY;
GameRenderState g_RenderState;

enum InitCall {
    INIT_SOUND_LIBRARY,
    INIT_SOUND_RUNTIME,
    RESET_GRAPH,
    SET_GRAPH_DEBUG,
    HIDE_DISPLAY,
    ENABLE_DMA_INTERRUPTS,
    INIT_GEOMETRY,
    INIT_PAD,
    INIT_MEMORY_CARD,
    APPLY_PAD_MAPPING,
    INIT_RECORDS,
    INIT_RENDER_STATE,
    INIT_SAVE_DEFAULTS,
    SET_CAMERA_MATRIX,
};

static enum InitCall s_calls[16];
static s32 s_callCount;
static s32 s_resetGraphMode;
static s32 s_renderOtShift;

static void RecordCall(enum InitCall call) {
    s_calls[s_callCount++] = call;
}

void ssinit(void) { RecordCall(INIT_SOUND_LIBRARY); }
void InitSoundRuntime(void) { RecordCall(INIT_SOUND_RUNTIME); }
s32 ResetGraph(s32 mode) {
    s_resetGraphMode = mode;
    RecordCall(RESET_GRAPH);
    return 0;
}
int SetGraphDebug(int level) {
    (void)level;
    RecordCall(SET_GRAPH_DEBUG);
    return 0;
}
void SetDispMask(int enabled) {
    (void)enabled;
    RecordCall(HIDE_DISPLAY);
}
long SetDMAInterruptState(long enabled) {
    (void)enabled;
    RecordCall(ENABLE_DMA_INTERRUPTS);
    return 0;
}
void InitGeom(void) { RecordCall(INIT_GEOMETRY); }
void GameInitPad(void) { RecordCall(INIT_PAD); }
void RestartMemoryCard(void) { RecordCall(INIT_MEMORY_CARD); }
void ApplyPadButtonMapping(void) { RecordCall(APPLY_PAD_MAPPING); }
void InitRecordTables(void) { RecordCall(INIT_RECORDS); }
void InitRenderState(s32 otShift) {
    s_renderOtShift = otShift;
    RecordCall(INIT_RENDER_STATE);
}
void InitSaveDefaults(void) { RecordCall(INIT_SAVE_DEFAULTS); }
void SetCameraRotMatrix(void) { RecordCall(SET_CAMERA_MATRIX); }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    static const enum InitCall expectedCalls[] = {
        INIT_SOUND_LIBRARY, INIT_SOUND_RUNTIME, RESET_GRAPH,
        SET_GRAPH_DEBUG, HIDE_DISPLAY, ENABLE_DMA_INTERRUPTS,
        INIT_GEOMETRY, INIT_PAD, INIT_MEMORY_CARD, APPLY_PAD_MAPPING,
        INIT_RECORDS, INIT_RENDER_STATE,
        INIT_SAVE_DEFAULTS, SET_CAMERA_MATRIX,
    };

    memset(&g_RenderState, 0x7F, sizeof(g_RenderState));
    g_ScreenOffsetX.value = 12;
    g_ScreenOffsetY.value = -8;
    g_PadMappingIndex = 7;
    g_NegconMappingIndex = 7;
    g_NegconMaxTwist = 7;
    g_NegconSteerPlay = 7;
    g_NegconSteerNeutral = 7;
    g_NegconNeutralI = 7;
    g_NegconNeutralII = 7;
    g_NegconNeutralL = 7;
    g_PadErrorState = PAD_ERROR_STATE_INVALID_INPUT;
    g_PadValidateCountdown = -1;
    g_PadErrorHoldBits = -1;
    g_MirrorMode = 1;
    g_ExtraGrandPrixUnlocked = 1;

    InitSubsystems();

    CHECK(s_callCount == (s32)(sizeof(expectedCalls) / sizeof(expectedCalls[0])));
    CHECK(memcmp(s_calls, expectedCalls, sizeof(expectedCalls)) == 0);
    CHECK(s_resetGraphMode == 0 && s_renderOtShift == 5);
    CHECK(g_ScreenOffsetX.value == 0 && g_ScreenOffsetY.value == 0);
    CHECK(g_PadMappingIndex == 0 && g_NegconMappingIndex == 0);
    CHECK(g_NegconSteerPlay == 1);
    CHECK(g_NegconSteerNeutral == 0 && g_NegconMaxTwist == 0);
    CHECK(g_NegconNeutralI == 0 && g_NegconNeutralII == 0);
    CHECK(g_NegconNeutralL == 0);
    CHECK(g_PadErrorState == PAD_ERROR_STATE_NONE);
    CHECK(g_PadValidateCountdown == 0x21 && g_PadErrorHoldBits == 0);
    CHECK(g_MirrorMode == 0 && g_ExtraGrandPrixUnlocked == 0);
    CHECK(g_RenderState.viewX == 0 && g_RenderState.viewY == -64);
    CHECK(g_RenderState.viewZ == -256);
    CHECK(g_RenderState.viewAngleX == 0x100);
    CHECK(g_RenderState.viewAngleY == 0 && g_RenderState.viewAngleZ == 0);

    puts("subsystem initialization tests passed");
    return 0;
}
