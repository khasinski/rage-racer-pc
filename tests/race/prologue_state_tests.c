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

#include <stdio.h>

u8 *g_AssetBase;
u8 *g_ImageBlockBuffer;
s32 g_AnimTimer;
s32 g_CameraCarIndex;
CameraViewMode g_CameraViewMode;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
s32 g_CourseIndex;
GameFrameContext *g_DrawBuffer;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_FrameSyncThreshold;
s32 g_IsEnvironmentMode4;
u16 g_PadPressed;
PrologueCameraCut g_PrologueCameraCuts[1];
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
static size_t s_installedSize;
static s32 s_startAudioCalls;
static s32 s_trackDataRequests;
static s32 s_trackInitCalls;

void EnterPrologue(void);
void TickPrologueStep(void);

s32 AssetLoadCompletedSuccessfully(void) { return s_assetReady; }
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
    return 1;
}
s32 IsPrologueWorldActive(s32 timer) {
    (void)timer;
    return 0;
}
void PauseCdAudio(void) {}
s32 PrologueLineIntensity(s32 y) {
    (void)y;
    return 0;
}
u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    return prim;
}
void RequestCdTrack(s32 track) {
    (void)track;
    s_cdRequests++;
}
s32 RequestSelectBgmAssets(void) { return 1; }
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

    puts("prologue state tests passed");
    return 0;
}
