#include "game/asset.h"
#include "game/cd.h"
#include "game/fmv.h"
#include "game/fmv_internal.h"
#include "game/render.h"
#include "game/scene.h"
#include "game/state.h"

void ReturnFromClassFmv(void) {
    StopFmvDiscPlayback();
    g_SceneId = GAME_SCENE_INIT_MENU;
    RequestSelectBgmAssets();
}

void ReturnFromEndingFmv(void) {
    StopFmvDiscPlayback();
    SetDispMask(0);
    SetupDisplay240(0, 0, 0);
    g_FrameSyncThreshold = 0x80;
    g_FadeStep = 4;
    g_FadeLevel = 0;
    g_SceneId = GAME_SCENE_ENDING_STILL;
    g_SceneTimer = 0;
}
