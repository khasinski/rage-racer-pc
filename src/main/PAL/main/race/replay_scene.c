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

static void DrawReplayBadge(void) {
    GameOrderingTableEntry *base;
    u8 *next;

    if ((g_SceneTimer & 0x10) == 0 || g_SeriesCleared != 0) {
        return;
    }

    base = GamePrimaryOrderingTable(0);
    next = GameQueueSprite(base, RENDER_PRIM_CURSOR_AS(u8), 0x10, 0x10, 0x48,
                           0x10, 0, 0x68, 0x780D);
    g_RenderState.packetCursor = QueueDrawModePrim(base, next, 9);
}

void UpdateReplayScene(void) {
    g_AnimTimer++;
    g_SceneTimer++;
    if (g_SceneTimer == 0x3C && g_GrandPrixMode != 0 &&
        g_SeriesCleared == 0) {
        PlaySoundCue(g_PlayerCar.drive.racePosition == 1 ? 0x40 : 0x41);
    }

    UpdateReplayFade();

    ApplyReplayFrame(g_ReplayReadCursor, AsRivalCar(&g_PlayerCar),
                     &g_Cars[0]);
    g_ReplayReadCursor++;
    if (g_ReplayReadCursor == g_ReplayFrameCount) {
        g_ReplayReadCursor = 0;
    }
    UpdateReplayCars();
    UpdateCamera(CAMERA_VIEW_TRACK,
                 GetCarRenderObject(AsRivalCar(&g_PlayerCar)));
    g_RenderState.envMode4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    if (g_GrandPrixMode != 0) {
        DrawPlayerCarOnly();
    }
    DrawCourseObjects();
    DrawCourseScenery2(g_SceneTimer, 1);
    UpdateEnvironment();
    DrawSkyBackground();
    DrawReplayBadge();
    if (g_SceneTimer == 1) {
        SetTrackTexturePageNow(g_PlayerCar.trackSection);
    }
}
