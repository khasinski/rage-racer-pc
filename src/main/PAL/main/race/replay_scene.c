#include "game/audio.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/track.h"
#include "game/track_internal.h"

enum {
    REPLAY_FIRST_FRAME = 1,
};

static void DrawReplayBadge(void) {
    GameOrderingTableEntry *base;
    u8 *next;

    if (!ReplayBadgeVisible(g_SceneTimer, g_SeriesCleared)) {
        return;
    }

    base = GamePrimaryOrderingTable(0);
    next = GameQueueSprite(base, RENDER_PRIM_CURSOR_AS(u8), 0x10, 0x10, 0x48,
                           0x10, 0, 0x68, 0x780D);
    g_RenderState.packetCursor = QueueDrawModePrim(base, next, 9);
}

void UpdateReplayScene(void) {
    s32 resultCue;

    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    g_SceneTimer = (s32)((u32)g_SceneTimer + 1u);
    resultCue = ReplayResultCue(g_SceneTimer, g_GrandPrixMode,
                                g_SeriesCleared,
                                g_PlayerCar.drive.racePosition);
    if (resultCue >= 0) {
        PlaySoundCue(resultCue);
    }

    UpdateReplayFade();

    ApplyReplayFrame(g_ReplayReadCursor, AsRivalCar(&g_PlayerCar),
                     &g_Cars[0]);
    g_ReplayReadCursor = NextReplayReadCursor(
        g_ReplayReadCursor, g_ReplayFrameCount);
    UpdateReplayCars();
    UpdateCamera(CAMERA_VIEW_TRACK,
                 GetCarRenderObject(AsRivalCar(&g_PlayerCar)));
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    if (g_GrandPrixMode != 0) {
        DrawReplayRivalCar();
    }
    DrawCourseObjects();
    DrawPresentationCourseScenery(g_SceneTimer, 1);
    UpdateEnvironment();
    DrawSkyBackground();
    DrawReplayBadge();
    if (g_SceneTimer == REPLAY_FIRST_FRAME) {
        SetTrackTexturePageNow(g_PlayerCar.trackSection);
    }
}
