#include "game/asset.h"
#include "game/cd.h"
#include "game/fmv.h"
#include "game/render.h"
#include "game/state.h"
#include "psyq/cd.h"

static void StopFmvCdPlayback(void) {
    CdSync(0, 0);
    CdControl(CD_DRIVE_PAUSE, 0, 0);
}

void ReturnFromClassFmv(void) {
    StopFmvCdPlayback();
    g_SceneId = 6;
    RequestSelectBgmAssets();
}

void ReturnFromEndingFmv(void) {
    StopFmvCdPlayback();
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = 0x80;
    g_FadeStep = 4;
    g_FadeLevel = 0;
    g_SceneId = 0x22;
    g_SceneTimer = 0;
}
