#include "game/prim.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/player_car_internal.h"
#include "game/track_internal.h"

void RecordReplayFrame(void) {
    const GameCarRuntime *player = AsRivalCar(&g_PlayerCar);

    if (g_GrandPrixMode != 0) {
        StoreReplayCarFrame(g_ReplayWriteCursor, player, &g_Cars[0]);
    } else {
        StoreReplayTimeAttackFrame(g_ReplayWriteCursor, player);
    }

    g_ReplayWriteCursor++;
    if (g_ReplayWriteCursor == g_ReplayFrameCount) {
        g_ReplayWriteCursor = 0;
        g_ReplayBufferWrapped = 1;
    }
}

void BeginReplay(void) {
    g_FadeLevel = 0xFF;
    g_SceneTimer = 0;
    g_FadeStep = -4;

    if (g_ReplayBufferWrapped != 0) {
        g_ReplayReadCursor = (g_ReplayWriteCursor & -2) + 2;
    } else {
        g_ReplayReadCursor = 0;
        g_ReplayFrameCount = g_ReplayWriteCursor - 2;
    }

    if (g_ReplayReadCursor >= g_ReplayFrameCount) {
        g_ReplayReadCursor = 0;
    }

    if (g_GrandPrixMode != 0) {
        if (g_GrandPrixClass != 5) {
            SeekEnvironmentScript(g_EnvScriptClock - 1800);
        }
    } else {
        if (g_GrandPrixClass != 5) {
            SeekEnvironmentScript(g_EnvScriptClock - 3000);
        }
    }

    SeedReplayCars();
}

void DrawReplayBadge(void) {
    u8 *base;
    u8 *next;

    if ((g_SceneTimer & 0x10) && (g_SeriesCleared == 0)) {
        base = (u8 *)GamePrimaryOrderingTable(0);
        next = GameQueueSprite(base, RENDER_PRIM_CURSOR_AS(u8),
                               0x10, 0x10, 0x48, 0x10,
                               0, 0x68, 0x780D);
        RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, next, 9);
    }
}
