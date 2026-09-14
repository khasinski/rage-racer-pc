#include "common.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/screens.h"

#include <stdio.h>

static Frontend s_frontend;

s32 g_FrameSyncThreshold;
s32 g_SceneId;
s32 g_SceneTimer;

static s32 s_audioCloseCalls;
static s32 s_classRefreshCalls;
static s32 s_displayMask;
static s32 s_imageUploadCalls;
static s32 s_reverbCalls;
static s32 s_textureResetCalls;

void CloseLoadedAudioSlots(void) { s_audioCloseCalls++; }
void RefreshClassWinState(void) { s_classRefreshCalls++; }
void ResetTrackTextureSwap(void) { s_textureResetCalls++; }
void SetDefaultReverbDepth(void) { s_reverbCalls++; }
void SetDispMask(s32 enabled) { s_displayMask = enabled; }
void UploadLoadBufferImage(void) { s_imageUploadCalls++; }

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    g_FrameSyncThreshold = -1;
    s_frontend.idleTimer = 99;
    s_frontend.state = -1;
    s_frontend.menuSlide = -1;
    g_SceneId = -1;
    g_SceneTimer = -1;
    s_frontend.attractTimer = 99;
    s_frontend.exitTimer = 99;
    s_frontend.fade = 99;
    s_frontend.pulse = 99;
    s_displayMask = -1;

    EnterFrontend();

    CHECK(s_displayMask == 0);
    CHECK(s_audioCloseCalls == 1 && s_textureResetCalls == 1 &&
          s_imageUploadCalls == 1);
    CHECK(g_FrameSyncThreshold == 0x80 && g_SceneId == 4 &&
          g_SceneTimer == 0);
    CHECK(s_frontend.idleTimer == 0 && s_frontend.menuSlide == 0 &&
          s_frontend.pulse == 0);
    CHECK(s_frontend.state == FRONTEND_STATE_TITLE);
    CHECK(s_frontend.fade == 0 && s_frontend.exitTimer == 0 &&
          s_frontend.attractTimer == -1);
    CHECK(s_classRefreshCalls == 1 && s_reverbCalls == 1);

    puts("enter frontend tests passed");
    return 0;
}

Frontend *MenuFrontend(void) { return &s_frontend; }
