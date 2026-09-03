#include "game/asset.h"
#include "game/audio.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"

/* Scene 2: reset title/menu state, then hand over to UpdateFrontend. */
void EnterFrontend(void) {
    SetDispMask(0);
    CloseLoadedAudioSlots();
    ResetTrackTextureSwap();
    UploadLoadBufferImage();

    g_FrameSyncThreshold = 0x80;
    g_SceneId = 4;
    g_SceneTimer = 0;
    g_FrontendIdleTimer = 0;
    g_TitleFadeLevel = 0;
    g_MainMenuSlide = 0;
    g_TitlePulse = 0;
    g_FrontendState = FRONTEND_STATE_TITLE;
    g_TitleExitTimer = 0;
    g_TitleAttractTimer = -1;

    RefreshClassWinState();
    SetDefaultReverbDepth();
}
