#include "common.h"
#include "game/fmv_internal.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"
#include "game/screens.h"

#include <limits.h>
#include <stdio.h>

static Frontend s_frontend;

s32 g_AnimTimer;
GameFrameContext *g_DrawBuffer;
s32 g_FrameSyncThreshold;
u16 g_PadPressed;
GameRenderState g_RenderState;
s32 g_SceneId;
s32 g_SceneTimer;
Fmv g_Fmv;

static GameFrameContext s_frame;
static s32 s_audioFadeCalls;
static s32 s_classRefreshCalls;
static s32 s_displayMaskCalls;
static s32 s_imageUploadCalls;
static s32 s_lastFadeBrightness;
static s32 s_lastSinAngle;
static s32 s_reverbCalls;
static s32 s_setupCalls;
static s32 s_soundCue;
static s32 s_textureResetCalls;
static s32 s_textureResetBeforeUpload;

void SetupDisplay240(s32 r, s32 g, s32 b) {
    (void)r;
    (void)g;
    (void)b;
    s_setupCalls++;
}
void SetDispMask(s32 enabled) {
    (void)enabled;
    s_displayMaskCalls++;
}
void ResetTrackTextureSwap(void) { s_textureResetCalls++; }
void UploadLoadBufferImage(void) {
    s_textureResetBeforeUpload = s_textureResetCalls;
    s_imageUploadCalls++;
}
void RefreshClassWinState(void) { s_classRefreshCalls++; }
void SetDefaultReverbDepth(void) { s_reverbCalls++; }
void StartCdVolumeFade(s32 frames) {
    (void)frames;
    s_audioFadeCalls++;
}
void PlaySoundCue(s32 cue) { s_soundCue = cue; }
s32 rsin(s32 angle) {
    s_lastSinAngle = angle;
    return 0;
}
u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                       s32 width, s32 height, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)g;
    (void)b;
    s_lastFadeBrightness = r;
    return prim;
}
u8 *GameQueueShadedSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x,
                          s32 y, s32 width, s32 height, s32 u, s32 v,
                          s32 clut, s32 intensity) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    (void)intensity;
    return prim;
}
u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    return prim;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetCalls(void) {
    g_DrawBuffer = &s_frame;
    g_RenderState.draw.packetCursor = s_frame.layout.primitiveBuffer;
    s_audioFadeCalls = 0;
    s_classRefreshCalls = 0;
    s_displayMaskCalls = 0;
    s_imageUploadCalls = 0;
    s_lastFadeBrightness = -1;
    s_reverbCalls = 0;
    s_setupCalls = 0;
    s_soundCue = 0;
    s_textureResetCalls = 0;
    s_textureResetBeforeUpload = 0;
}

int main(void) {
    ResetCalls();
    g_Fmv.returnScene = 0;
    EnterTitleScreen();
    CHECK(s_setupCalls == 1 && s_displayMaskCalls == 1 &&
          s_imageUploadCalls == 1);
    CHECK(s_textureResetCalls == 1 && s_textureResetBeforeUpload == 1);
    CHECK(s_frontend.fade == 0 && s_frontend.attractTimer == 0 &&
          s_frontend.exitTimer == 30);
    CHECK(s_frontend.state == FRONTEND_STATE_TITLE && g_SceneId == 4 &&
          g_SceneTimer == 0);
    CHECK(s_classRefreshCalls == 1 && s_reverbCalls == 1);

    ResetCalls();
    g_Fmv.returnScene = 1;
    EnterTitleScreen();
    CHECK(s_setupCalls == 1 && s_displayMaskCalls == 0 &&
          s_imageUploadCalls == 0);
    CHECK(s_textureResetCalls == 1);
    CHECK(s_frontend.fade == 253 && s_frontend.attractTimer == 400 &&
          s_frontend.exitTimer == 0);
    CHECK(s_lastFadeBrightness == 255);

    s_frontend.fade = 1;
    DrawPressStartPrompt();
    CHECK(s_frontend.fade == 0 && s_lastFadeBrightness == 1);

    g_AnimTimer = INT_MAX;
    DrawPressStartPrompt();
    CHECK(s_lastSinAngle >= 0 && s_lastSinAngle <= 0xFE0);

    g_PadPressed = PAD_START;
    s_frontend.attractTimer = 10;
    s_frontend.selection = 4;
    UpdateTitleScreen();
    CHECK(s_frontend.state == FRONTEND_STATE_MENU_OPENING);
    CHECK(s_frontend.idleTimer == 0 && s_frontend.selection == 0);
    CHECK(s_frontend.attractTimer == 0 && s_audioFadeCalls == 1);
    CHECK(s_soundCue == 2);

    puts("title screen state tests passed");
    return 0;
}

Frontend *MenuFrontend(void) { return &s_frontend; }
