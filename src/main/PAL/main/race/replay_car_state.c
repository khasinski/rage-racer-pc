#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/replay_internal.h"
#include "game/render.h"
#include "game/track.h"

static void SeedReplayCarTrackState(GameCarRuntime *car) {
    car->trackPointIndex = FindTrackSegment(car, car->trackPointIndex);
    SeedCarLapProgress(car, 1);
    AccumulateLapProgress(car);
    ReconstructReplayCarTrackState(car);
}

static void AdvanceReplayCarTrackState(GameCarRuntime *car) {
    AccumulateLapProgress(car);
    ReconstructReplayCarTrackState(car);
}

void SeedReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    GameCarRuntime *rival = &g_Cars[0];

    InitShuttleScenery();
    if (g_ReplayFrameCount <= 0) {
        return;
    }
    ApplyReplayFrameAndTrackPoint(g_ReplayReadCursor, player, rival);

    SeedReplayCarTrackState(player);

    if (g_GrandPrixMode != 0) {
        SeedReplayCarTrackState(rival);
    }
}

void UpdateReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);

    AdvanceReplayCarTrackState(player);

    if (g_GrandPrixMode != 0) {
        AdvanceReplayCarTrackState(&g_Cars[0]);
    }

    RequestTrackTexturePage(player->trackSection);
}
