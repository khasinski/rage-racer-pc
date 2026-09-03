#include "game/asset.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"
#include "game/track.h"

#include <limits.h>
#include <stdio.h>

u8 *g_AssetBase;
u8 *g_ImageBlockBuffer;
s32 g_AssetLoadFailed;
s32 g_AssetLoadState;
s32 g_AnimTimer;
s32 g_CameraCarIndex;
CameraViewMode g_CameraViewMode;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
s32 g_CourseIndex;
static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_FrameSyncThreshold;
s32 g_IsEnvironmentMode4;
u16 g_PadPressed;
PrologueCameraCut g_PrologueCameraCuts[PROLOGUE_CAMERA_CUT_COUNT];
s32 g_PrologueCutIndex;
PrologueLine g_PrologueLines[17];
s32 g_PrologueLineCount;
s32 g_PrologueStep;
GameRenderState g_RenderState;
s32 g_SceneId;
s32 g_SceneTimer;
char g_TextNowLoading[] = "NOW LOADING";

static s32 s_assetReady;
static s32 s_cdRequests;
static s32 s_displayMask;
static s32 s_installCalls;
static s32 s_installSucceeds = 1;
static size_t s_installedSize;
static s32 s_startAudioCalls;
static s32 s_trackDataRequests;
static s32 s_trackInitCalls;
static s32 s_exitRequests;
static s32 s_pauseCalls;

void EnterPrologue(void);
void TickPrologueStep(void);

s32 AssetLoadCompletedSuccessfully(void) {
    return s_assetReady && !g_AssetLoadFailed;
}
void DrawCars(void) {}
void DrawCourseObjects(void) {}
void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    (void)color;
    (void)tpage;
}
void DrawPresentationCourseScenery(s32 timer, s32 animate) {
    (void)timer;
    (void)animate;
}
void DrawProportionalText(s32 x, s32 y, const char *text, s32 clut) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
}
void DrawSkyBackground(void) {}
void DrawTerrainCellsWide(void) {}
void GameDrawText8x8Shaded(s32 x, s32 y, const char *text, s32 clut,
                           u8 intensity) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
    (void)intensity;
}
u8 *GameQueueTileTrans(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                       s32 w, s32 h, s32 r, s32 g, s32 b) {
    (void)ot;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)r;
    (void)g;
    (void)b;
    return prim;
}
void InitTrackScene(void) { s_trackInitCalls++; }
s32 InstallTrackTextureAssetPack(u8 *base, size_t size) {
    (void)base;
    s_installCalls++;
    s_installedSize = size;
    return s_installSucceeds;
}
void PauseCdAudio(void) { s_pauseCalls++; }
u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    return prim;
}
void RequestCdTrack(s32 track) {
    (void)track;
    s_cdRequests++;
}
s32 RequestSelectBgmAssets(void) {
    s_exitRequests++;
    return 1;
}
s32 RequestTrackDataAssets(void) {
    s_trackDataRequests++;
    return 1;
}
void RequestTrackTexturePage(s32 section) { (void)section; }
void SetDispMask(s32 enabled) { s_displayMask = enabled; }
void SetupDisplay240(s32 r, s32 g, s32 b) {
    (void)r;
    (void)g;
    (void)b;
}
void StartCdAudio(void) { s_startAudioCalls++; }
void UpdateAttractCars(void) {}
void UpdateCamera(CameraViewMode mode, GameRenderObject *car) {
    (void)mode;
    (void)car;
}
void UpdateEnvironment(void) {}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,       \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

int main(void) {
    u8 asset[17];
    s32 i;

    g_AssetBase = asset;
    g_ImageBlockBuffer = asset + sizeof(asset);
    EnterPrologue();
    CHECK(g_PrologueStep == PROLOGUE_STEP_LOAD_TEXTURES);
    CHECK(g_SceneId == 0x20 && g_SceneTimer == 0);
    CHECK(g_CameraCarIndex == 3 && g_FadeLevel == 0x108 && g_FadeStep == -4);
    CHECK(s_displayMask == 0);

    g_FadeLevel = 0;
    g_FadeStep = 0;
    s_assetReady = 1;
    TickPrologueStep();
    CHECK(g_PrologueStep == PROLOGUE_STEP_LOAD_TRACK);
    CHECK(s_installCalls == 1 && s_installedSize == sizeof(asset));
    CHECK(s_trackDataRequests == 1);

    EnterPrologue();
    g_ImageBlockBuffer = g_AssetBase;
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_AssetLoadFailed = 0;
    TickPrologueStep();
    CHECK(g_AssetLoadFailed == 1 && g_AssetLoadState == 0);
    CHECK(g_PrologueStep == PROLOGUE_STEP_LOAD_TEXTURES);

    EnterPrologue();
    g_ImageBlockBuffer = asset + sizeof(asset);
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_AssetLoadFailed = 0;
    s_installSucceeds = 0;
    TickPrologueStep();
    CHECK(g_AssetLoadFailed == 1 && s_trackDataRequests == 1);
    CHECK(g_PrologueStep == PROLOGUE_STEP_LOAD_TEXTURES);
    s_installSucceeds = 1;

    EnterPrologue();
    g_ImageBlockBuffer = asset + sizeof(asset);
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_AssetLoadFailed = 0;

    TickPrologueStep();
    CHECK(g_PrologueStep == PROLOGUE_STEP_LOAD_TRACK);
    TickPrologueStep();
    CHECK(g_PrologueStep == PROLOGUE_STEP_WAIT_FOR_FADE);
    CHECK(s_cdRequests == 1 && g_FadeLevel == 4 && g_FadeStep == 4);

    for (i = 0; i < 63; i++) TickPrologueStep();
    CHECK(g_PrologueStep == PROLOGUE_STEP_WAIT_FOR_FADE);
    CHECK(g_FadeLevel == 0x100);
    TickPrologueStep();
    CHECK(g_PrologueStep == PROLOGUE_STEP_ACTIVE);
    CHECK(g_FadeLevel == 0x100 && g_FadeStep == 0);
    CHECK(g_CourseIndex == 0 && s_trackInitCalls == 1);
    CHECK(s_startAudioCalls == 1 && s_displayMask == 0);

    g_PrologueStep = PROLOGUE_STEP_ACTIVE;
    g_SceneTimer = 100;
    g_PrologueCutIndex = INT_MAX;
    g_CameraCarIndex = INT_MAX;
    g_PrologueLineCount = INT_MAX;
    g_RenderState.packetCursor = s_frame.layout.primitiveBuffer;
    TickPrologueStep();
    CHECK(g_SceneTimer == 101);
    CHECK(g_PrologueCutIndex == PROLOGUE_CAMERA_CUT_COUNT - 1);
    CHECK(g_CameraCarIndex == 0);

    g_SceneTimer = 100;
    g_PrologueCutIndex = -1;
    TickPrologueStep();
    CHECK(g_PrologueCutIndex == 0);

    g_SceneTimer = 1279;
    TickPrologueStep();
    CHECK(g_SceneTimer == 1280 && g_SceneId == 6);
    CHECK(s_exitRequests == 1 && s_pauseCalls == 1);

    puts("prologue state tests passed");
    return 0;
}
