#include "game/asset.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"

static void SeedReplayCarTrackState(GameCarRuntime *car) {
    car->trackPointIndex = FindTrackSegment(car, car->trackPointIndex);
    SeedCarLapProgress(car, 1);
    AccumulateLapProgress(car);
    ResetCarTrackState(car);
}

static void UpdateReplayCarTrackState(GameCarRuntime *car) {
    AccumulateLapProgress(car);
    ResetCarTrackState(car);
}

void SeedReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    GameCarRuntime *rival = &g_Cars[0];

    InitShuttleScenery();
    ApplyReplayFrameAndTilt(g_ReplayReadCursor, player, rival);

    SeedReplayCarTrackState(player);

    if (g_GrandPrixMode != 0) {
        SeedReplayCarTrackState(rival);
    }
}

void UpdateReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);

    UpdateReplayCarTrackState(player);

    if (g_GrandPrixMode != 0) {
        UpdateReplayCarTrackState(&g_Cars[0]);
    }

    RequestTrackTexturePage(player->trackSection);
}
