#include <assert.h>
#include <limits.h>

#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/state.h"

PlayerCarRuntime g_PlayerCar;
GameCarRuntime g_Cars[RACE_CAR_SLOT_COUNT];
GameRenderState g_RenderState;
static GameFrameContext s_FrameContext;
GameFrameContext *g_DrawBuffer = &s_FrameContext;

s32 g_AnimTimer;
s32 g_SceneTimer;
s32 g_SeriesCleared;
s16 g_GrandPrixMode;
s32 g_ReplayReadCursor;
s32 g_ReplayFrameCount;
s32 g_IsEnvironmentMode4;

static s32 s_AppliedCursor;
static s32 s_FadeUpdates;
static s32 s_CarUpdates;
static s32 s_CameraUpdates;
static s32 s_TerrainDraws;
static s32 s_RivalCarDraws;
static s32 s_ObjectDraws;
static s32 s_SceneryDraws;
static s32 s_EnvironmentUpdates;
static s32 s_SkyDraws;
static s32 s_TextureSets;
static s32 s_TextureSection;
static s32 s_SpriteDraws;
static s32 s_DrawModeDraws;
static s32 s_SoundCues;
static s32 s_LastSoundCue;

void PlaySoundCue(s32 cue) {
    s_SoundCues++;
    s_LastSoundCue = cue;
}
void UpdateReplayFade(void) { s_FadeUpdates++; }
void ApplyReplayFrame(s32 subframe, GameCarRuntime *player,
                      GameCarRuntime *rival) {
    assert(player == AsRivalCar(&g_PlayerCar));
    assert(rival == &g_Cars[0]);
    s_AppliedCursor = subframe;
}
void UpdateReplayCars(void) { s_CarUpdates++; }
void UpdateCamera(CameraViewMode mode, GameRenderObject *car) {
    assert(mode == CAMERA_VIEW_TRACK);
    assert(car == GetCarRenderObject(AsRivalCar(&g_PlayerCar)));
    s_CameraUpdates++;
}
void DrawTerrainCellsWide(void) { s_TerrainDraws++; }
void DrawReplayRivalCar(void) { s_RivalCarDraws++; }
void DrawCourseObjects(void) { s_ObjectDraws++; }
void DrawPresentationCourseScenery(s32 timer, s32 animate) {
    assert(timer == g_SceneTimer);
    assert(animate == 1);
    s_SceneryDraws++;
}
void UpdateEnvironment(void) { s_EnvironmentUpdates++; }
void DrawSkyBackground(void) { s_SkyDraws++; }
void SetTrackTexturePageNow(s32 trackSection) {
    s_TextureSets++;
    s_TextureSection = trackSection;
}
u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                    s32 w, s32 h, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    (void)u;
    (void)v;
    (void)clut;
    s_SpriteDraws++;
    return prim + 1;
}
u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    s_DrawModeDraws++;
    return prim + 1;
}

static void ResetState(void) {
    g_AnimTimer = 0;
    g_SceneTimer = 0;
    g_SeriesCleared = 0;
    g_GrandPrixMode = 0;
    g_ReplayReadCursor = 7;
    g_ReplayFrameCount = 20;
    g_IsEnvironmentMode4 = 3;
    g_PlayerCar.trackSection = 12;
    g_PlayerCar.drive.racePosition = 1;
    g_RenderState.packetCursor = s_FrameContext.layout.primitiveBuffer;
    s_AppliedCursor = -1;
    s_FadeUpdates = 0;
    s_CarUpdates = 0;
    s_CameraUpdates = 0;
    s_TerrainDraws = 0;
    s_RivalCarDraws = 0;
    s_ObjectDraws = 0;
    s_SceneryDraws = 0;
    s_EnvironmentUpdates = 0;
    s_SkyDraws = 0;
    s_TextureSets = 0;
    s_TextureSection = -1;
    s_SpriteDraws = 0;
    s_DrawModeDraws = 0;
    s_SoundCues = 0;
    s_LastSoundCue = -1;
}

static void TestFirstTimeAttackFrame(void) {
    ResetState();

    UpdateReplayScene();

    assert(g_AnimTimer == 1 && g_SceneTimer == 1);
    assert(s_AppliedCursor == 7 && g_ReplayReadCursor == 8);
    assert(s_FadeUpdates == 1 && s_CarUpdates == 1);
    assert(s_CameraUpdates == 1 && s_TerrainDraws == 1);
    assert(s_RivalCarDraws == 0);
    assert(s_ObjectDraws == 1 && s_SceneryDraws == 1);
    assert(s_EnvironmentUpdates == 1 && s_SkyDraws == 1);
    assert(g_RenderState.envMode4 == 3);
    assert(s_TextureSets == 1 && s_TextureSection == 12);
    assert(s_SpriteDraws == 0 && s_DrawModeDraws == 0);
}

static void TestGrandPrixResultCueAndCar(void) {
    ResetState();
    g_GrandPrixMode = 1;
    g_SceneTimer = 59;

    UpdateReplayScene();

    assert(s_RivalCarDraws == 1);
    assert(s_SoundCues == 1 && s_LastSoundCue == 0x40);
    assert(s_TextureSets == 0);
}

static void TestReplayBadgeVisibility(void) {
    ResetState();
    g_SceneTimer = 15;

    UpdateReplayScene();

    assert(s_SpriteDraws == 1 && s_DrawModeDraws == 1);

    ResetState();
    g_SceneTimer = 15;
    g_SeriesCleared = 1;

    UpdateReplayScene();

    assert(s_SpriteDraws == 0 && s_DrawModeDraws == 0);
}

static void TestSceneCountersWrap(void) {
    ResetState();
    g_AnimTimer = INT_MAX;
    g_SceneTimer = INT_MAX;

    UpdateReplayScene();

    assert(g_AnimTimer == INT_MIN && g_SceneTimer == INT_MIN);
}

int main(void) {
    TestFirstTimeAttackFrame();
    TestGrandPrixResultCueAndCar();
    TestReplayBadgeVisibility();
    TestSceneCountersWrap();
    return 0;
}
