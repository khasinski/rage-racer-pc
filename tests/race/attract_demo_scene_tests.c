#include <assert.h>
#include <limits.h>
#include <stddef.h>

#include "game/asset.h"
#include "game/audio_internal.h"
#include "game/car.h"
#include "game/fmv_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/state.h"

u8 *g_AssetBase;
s32 g_AssetLoadFailed;
s32 g_AssetLoadState;
s32 g_AnimTimer;
AttractDemoStep g_AttractDemoStep;
s32 g_BgmShuffleIndex;
u8 g_BgmShuffleOrder[BGM_SHUFFLE_CAPACITY];
s32 g_BgmTrackCount;
s32 g_CameraCarIndex;
s32 g_CourseIndex;
s32 g_FadeLevel;
s32 g_FrameSyncThreshold;
u8 *g_ImageBlockBuffer;
size_t g_ImageBlockSize;
u16 g_PadPressed;
s32 g_SceneId;
s32 g_SceneTimer;
s32 g_StreamReturnScene;
s16 g_AttractTitleDelays[4];

static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;

static s32 s_audioResets;
static s32 s_cameraCycles;
static s32 s_installSucceeds;
static s32 s_uploadSucceeds;
static s32 s_worldUpdates;

void SetDispMask(int enabled) { (void)enabled; }
void SetupDisplay240(s32 red, s32 green, s32 blue) {
    (void)red;
    (void)green;
    (void)blue;
}
s32 UploadImageAsset(const GameImageAssetHeaderWord *asset, size_t size) {
    (void)asset;
    (void)size;
    return s_uploadSucceeds;
}
s32 InstallTrackTextureAssetPack(u8 *base, size_t size) {
    (void)base;
    (void)size;
    return s_installSucceeds;
}
s32 RequestTrackDataAssets(void) { return 1; }
s32 AssetLoadCompletedSuccessfully(void) { return 0; }
void InitTrackScene(void) {}
void AdvanceBgmShuffleBag(u32 track) { (void)track; }
void RequestCdTrack(s32 track) { (void)track; }
void StartCdAudio(void) {}
void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1,
                u16 u0, u16 v0, u8 red, u8 green, u8 blue, u16 clut,
                s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)u0;
    (void)v0;
    (void)red;
    (void)green;
    (void)blue;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
}
void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    (void)color;
    (void)tpage;
}
void StartCdVolumeFade(s32 frames) { (void)frames; }
void ResetCdAudioState(void) { s_audioResets++; }
s32 CycleAttractCameraCar(s32 mask, s32 current) {
    (void)mask;
    s_cameraCycles++;
    return current + 1;
}
void UpdateAndDrawAttractWorld(void) { s_worldUpdates++; }
void ResetAssetLoader(void) {}

static void Reset(void) {
    g_AnimTimer = 10;
    g_AttractDemoStep = ATTRACT_DEMO_STEP_RACE;
    g_CameraCarIndex = 2;
    g_PadPressed = 0;
    g_SceneId = 0x1E;
    g_SceneTimer = 100;
    g_StreamReturnScene = 7;
    s_audioResets = 0;
    s_cameraCycles = 0;
    s_installSucceeds = 1;
    s_uploadSucceeds = 1;
    s_worldUpdates = 0;
    g_AssetLoadFailed = 0;
    g_AssetLoadState = 0;
}

static void TestEntryRejectsInvalidResidentAssets(void) {
    u8 asset[2];

    Reset();
    g_AssetBase = asset;
    g_ImageBlockBuffer = asset;
    g_ImageBlockSize = sizeof(asset);
    EnterAttractDemo();
    assert(g_AssetLoadFailed == 1 && g_AssetLoadState == 0);

    Reset();
    g_AssetBase = asset;
    g_ImageBlockBuffer = asset + sizeof(asset);
    g_ImageBlockSize = sizeof(asset);
    s_installSucceeds = 0;
    EnterAttractDemo();
    assert(g_AssetLoadFailed == 1 && g_AssetLoadState == 0);

    Reset();
    g_AssetBase = asset;
    g_ImageBlockBuffer = asset + sizeof(asset);
    g_ImageBlockSize = sizeof(asset);
    s_uploadSucceeds = 0;
    EnterAttractDemo();
    assert(g_AssetLoadFailed == 1 && g_AssetLoadState == 0);
}

static void TestRaceFrame(void) {
    Reset();
    g_AnimTimer = INT_MAX;

    UpdateAttractDemoScene();

    assert(g_SceneTimer == 101);
    assert(g_AnimTimer == INT_MIN);
    assert(g_CameraCarIndex == 3 && s_cameraCycles == 1);
    assert(s_worldUpdates == 1 && s_audioResets == 0);
}

static void TestReturnFrameStopsRaceUpdate(void) {
    Reset();
    g_SceneTimer = 0x707;

    UpdateAttractDemoScene();

    assert(g_SceneTimer == 0x708);
    assert(g_SceneId == 3 && g_StreamReturnScene == 0);
    assert(s_audioResets == 1);
    assert(g_AnimTimer == 10 && g_CameraCarIndex == 2);
    assert(s_cameraCycles == 0 && s_worldUpdates == 0);
}

static void TestCorruptTimerStillReturns(void) {
    Reset();
    g_SceneTimer = INT_MAX;

    UpdateAttractDemoScene();

    assert(g_SceneTimer == 0x708 && g_SceneId == 3);
    assert(s_audioResets == 1 && s_worldUpdates == 0);
}

int main(void) {
    TestEntryRejectsInvalidResidentAssets();
    TestRaceFrame();
    TestReturnFrameStopsRaceUpdate();
    TestCorruptTimerStillReturns();
    return 0;
}
