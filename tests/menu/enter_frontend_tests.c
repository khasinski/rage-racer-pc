#include "common.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/screens.h"

#include <stdio.h>

s32 g_FrameSyncThreshold;
u32 g_FrontendIdleTimer;
s32 g_FrontendState;
s32 g_MainMenuSlide;
s32 g_SceneId;
s32 g_SceneTimer;
s32 g_TitleAttractTimer;
s32 g_TitleExitTimer;
s32 g_TitleFadeLevel;
s32 g_TitlePulse;

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
    g_FrontendIdleTimer = 99;
    g_FrontendState = -1;
    g_MainMenuSlide = -1;
    g_SceneId = -1;
    g_SceneTimer = -1;
    g_TitleAttractTimer = 99;
    g_TitleExitTimer = 99;
    g_TitleFadeLevel = 99;
    g_TitlePulse = 99;
    s_displayMask = -1;

    EnterFrontend();

    CHECK(s_displayMask == 0);
    CHECK(s_audioCloseCalls == 1 && s_textureResetCalls == 1 &&
          s_imageUploadCalls == 1);
    CHECK(g_FrameSyncThreshold == 0x80 && g_SceneId == 4 &&
          g_SceneTimer == 0);
    CHECK(g_FrontendIdleTimer == 0 && g_MainMenuSlide == 0 &&
          g_TitlePulse == 0);
    CHECK(g_FrontendState == FRONTEND_STATE_TITLE);
    CHECK(g_TitleFadeLevel == 0 && g_TitleExitTimer == 0 &&
          g_TitleAttractTimer == -1);
    CHECK(s_classRefreshCalls == 1 && s_reverbCalls == 1);

    puts("enter frontend tests passed");
    return 0;
}
