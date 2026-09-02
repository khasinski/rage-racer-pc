#include "game/asset.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"

void SeedReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    GameCarRuntime *rival = &g_Cars[0];

    InitShuttleScenery();
    ApplyReplayFrameAndTilt(g_ReplayReadCursor, player, rival);

    player->trackPointIndex = FindTrackSegment(player, player->trackPointIndex);
    SeedCarLapProgress(player, 1);
    AccumulateLapProgress(player);
    ResetCarTrackState(player);

    if (g_GrandPrixMode == 1) {
        rival->trackPointIndex = FindTrackSegment(rival, rival->trackPointIndex);
        SeedCarLapProgress(rival, 1);
        AccumulateLapProgress(rival);
        ResetCarTrackState(rival);
    }
}

void UpdateReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);

    AccumulateLapProgress(player);
    ResetCarTrackState(player);

    if (g_GrandPrixMode == 1) {
        AccumulateLapProgress(&g_Cars[0]);
        ResetCarTrackState(&g_Cars[0]);
    }

    RequestTrackTexturePage(player->trackSection);
}
