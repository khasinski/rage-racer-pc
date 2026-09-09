#include <assert.h>
#include <stddef.h>

#include "game/asset.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/scene.h"

s32 g_FrameSyncThreshold;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_SceneId;
s32 g_SceneTimer;
s32 g_CameraCarIndex;
s32 g_BgmSelectCursor;
s32 g_BgmSelectShowUi;
s32 g_BgmSelectCdTrack;
BgmSelectStep g_BgmSelectStep;
s32 g_BgmSelectTrack;
s32 g_BgmChangeDelay;
s32 g_CdTrackEnded;
s32 g_AssetLoadState;
s32 g_AssetLoadFailed;
u8 *g_AssetBase;
u8 *g_ImageBlockBuffer;
size_t g_ImageBlockSize;
const char *g_TextNowLoading = "loading";
static u8 s_assetBuffer[128];

static int s_assetReady, s_uploads, s_installs, s_trackRequests, s_trackInit;
static int s_displayMask, s_fades, s_failed;

void SetDispMask(s32 enabled) { s_displayMask = enabled; }
void SetupDisplay240(s32 a, s32 b, s32 c) { assert(a == 0 && b == 0 && c == 0); }
s32 AssetLoadCompletedSuccessfully(void) { return s_assetReady; }
s32 UploadImageAsset(const GameImageAssetHeaderWord *header, size_t size) { (void)header; (void)size; s_uploads++; return 1; }
s32 AssetSpanSize(const void *base, const void *end, size_t *size) { (void)base; (void)end; *size = 64; return 1; }
s32 InstallTrackTextureAssetPack(u8 *base, size_t size) { (void)base; assert(size == 64); s_installs++; return 1; }
s32 RequestTrackDataAssets(void) { s_trackRequests++; return 1; }
void FailAssetLoad(void) { s_failed++; }
void InitTrackScene(void) { s_trackInit++; }
void DrawFullscreenFadeTile(s32 level, s32 tpage) { assert(tpage == 0x49); (void)level; s_fades++; }
void DrawProportionalText(s32 x, s32 y, const char *text, s32 color) { (void)x; (void)y; (void)text; (void)color; }

static void Reset(void) {
    s_assetReady = s_uploads = s_installs = s_trackRequests = s_trackInit = 0;
    s_displayMask = -1; s_fades = s_failed = 0;
    g_AssetBase = s_assetBuffer;
    g_ImageBlockBuffer = s_assetBuffer + 64;
    g_ImageBlockSize = 64;
    g_SceneTimer = 0; g_FadeLevel = 316; g_FadeStep = -4;
}

static void TestEntryAndTexturePreparation(void) {
    Reset();
    EnterBgmSelectScreen();
    assert(s_displayMask == 0 && g_SceneId == GAME_SCENE_BGM_SELECT);
    assert(g_BgmSelectStep == BGM_SELECT_STEP_LOAD_ASSETS &&
           g_CameraCarIndex == 0 && g_BgmSelectCdTrack == 3);
    s_assetReady = 1;
    assert(AssetLoadCompletedSuccessfully());
    UpdateBgmSelectLoad();
    assert(s_uploads == 1);
    assert(s_installs == 1);
    assert(s_trackRequests == 1);
    assert(s_failed == 0);
    assert(g_BgmSelectStep == BGM_SELECT_STEP_FADE_IN);
    assert(s_trackInit == 0);
}

static void TestTrackWorldStartsOnlyAfterTrackAssets(void) {
    Reset();
    g_BgmSelectStep = BGM_SELECT_STEP_FADE_IN;
    g_FadeLevel = 253; g_FadeStep = 0;
    UpdateBgmSelectFadeIn();
    assert(s_trackInit == 0 && g_BgmSelectStep == BGM_SELECT_STEP_FADE_IN);
    s_assetReady = 1;
    UpdateBgmSelectFadeIn();
    assert(s_trackInit == 1 && s_displayMask == 0 &&
           g_BgmSelectStep == BGM_SELECT_STEP_ACTIVE && g_FadeLevel == 0);
}

static void TestFadeInDoesNotBuildAWorldWhileLoading(void) {
    Reset();
    g_BgmSelectStep = BGM_SELECT_STEP_FADE_IN;
    g_FadeLevel = 316;
    g_FadeStep = -4;
    UpdateBgmSelectFadeIn();
    assert(s_trackInit == 0 && g_BgmSelectStep == BGM_SELECT_STEP_FADE_IN);
    assert(g_FadeLevel == 257 && g_FadeStep == -4);
}

int main(void) {
    TestEntryAndTexturePreparation();
    TestTrackWorldStartsOnlyAfterTrackAssets();
    TestFadeInDoesNotBuildAWorldWhileLoading();
    return 0;
}
