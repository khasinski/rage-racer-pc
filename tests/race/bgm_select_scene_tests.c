#include <assert.h>
#include <limits.h>

#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"

s32 g_AnimTimer;
s32 g_BgmSelectShowUi;
BgmSelectStep g_BgmSelectStep;
s32 g_CameraCarIndex;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_SceneTimer;

static s32 s_assetRequests;
static s32 s_displayMask;
static s32 s_fadeLevel;
static s32 s_inputUpdates;
static s32 s_loadUpdates;
static s32 s_fadeInUpdates;
static s32 s_exitUpdates;
static s32 s_playbackUpdates;
static s32 s_uiDraws;
static s32 s_uiUpdates;
static s32 s_worldUpdates;

void UpdateBgmSelectPlayback(void) { s_playbackUpdates++; }
void UpdateBgmSelectInput(void) { s_inputUpdates++; }
void RequestOptionScreenAssets(void) { s_assetRequests++; }
void SetDispMask(s32 enabled) { s_displayMask = enabled; }
void DrawFullscreenFadeTile(s32 level, s32 tpage) {
    /* The assertion is the only reader, and a release build compiles it
     * away, which leaves the parameter unused and the build refusing it. */
    (void)tpage;
    assert(tpage == 0x49);
    s_fadeLevel = level;
}
void DrawBgmSelectBar(void) { s_uiDraws++; }
void UpdateBgmSelectBar(void) { s_uiUpdates++; }
s32 CycleBgmSelectCameraCar(s32 mask, s32 current) {
    (void)mask;
    assert(mask == 0xFF);
    return current + 1;
}
void UpdateAndDrawAttractWorld(void) { s_worldUpdates++; }
void UpdateBgmSelectLoad(void) { s_loadUpdates++; }
void UpdateBgmSelectFadeIn(void) { s_fadeInUpdates++; }
void ExitBgmSelect(void) { s_exitUpdates++; }

static void Reset(void) {
    g_AnimTimer = 10;
    g_BgmSelectShowUi = 1;
    g_BgmSelectStep = BGM_SELECT_STEP_ACTIVE;
    g_CameraCarIndex = 2;
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_SceneTimer = 1;
    s_assetRequests = 0;
    s_displayMask = -1;
    s_fadeLevel = -1;
    s_inputUpdates = 0;
    s_loadUpdates = 0;
    s_fadeInUpdates = 0;
    s_exitUpdates = 0;
    s_playbackUpdates = 0;
    s_uiDraws = 0;
    s_uiUpdates = 0;
    s_worldUpdates = 0;
}

static void TestActiveFrame(void) {
    Reset();
    g_SceneTimer = 2;

    UpdateBgmSelect();

    assert(s_playbackUpdates == 1 && s_inputUpdates == 1);
    assert(s_displayMask == 1 && s_uiUpdates == 1 && s_uiDraws == 1);
    assert(g_AnimTimer == 11 && g_CameraCarIndex == 3);
    assert(s_worldUpdates == 1);
}

static void TestExitFadeRequest(void) {
    Reset();
    g_FadeLevel = 254;
    g_FadeStep = 4;

    UpdateBgmSelect();

    assert(s_inputUpdates == 0);
    assert(s_fadeLevel == 254 && s_assetRequests == 1);
    assert(g_BgmSelectStep == BGM_SELECT_STEP_EXIT);
    assert(g_FadeLevel == 256 && g_FadeStep == -4);
}

static void TestSceneDispatch(void) {
    Reset();
    g_BgmSelectStep = BGM_SELECT_STEP_LOAD_ASSETS;
    UpdateBgmSelectScene();
    assert(g_SceneTimer == 2 && s_loadUpdates == 1);

    g_BgmSelectStep = BGM_SELECT_STEP_FADE_IN;
    UpdateBgmSelectScene();
    assert(s_fadeInUpdates == 1);

    g_BgmSelectStep = BGM_SELECT_STEP_EXIT;
    UpdateBgmSelectScene();
    assert(s_exitUpdates == 1);

    Reset();
    g_SceneTimer = INT_MAX;
    UpdateBgmSelectScene();
    assert(g_SceneTimer == 10000);
}

static void TestExtremeFadeAndAnimationState(void) {
    Reset();
    g_AnimTimer = INT_MAX;
    g_FadeLevel = INT_MAX;
    g_FadeStep = INT_MAX;

    UpdateBgmSelect();

    assert(s_fadeLevel == 256 && s_assetRequests == 1);
    assert(g_FadeLevel == 256 && g_FadeStep == -4);
    assert(g_AnimTimer == INT_MIN);
}

int main(void) {
    TestActiveFrame();
    TestExitFadeRequest();
    TestSceneDispatch();
    TestExtremeFadeAndAnimationState();
    return 0;
}
