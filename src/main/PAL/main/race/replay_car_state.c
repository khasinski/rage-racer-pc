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
    s32 i;

    InitShuttleScenery();
    if (g_Replay.count <= 0) {
        return;
    }
    ApplyReplayFrameAndTrackPoint(g_Replay.read, player, g_Cars);

    SeedReplayCarTrackState(player);

    if (g_GrandPrixMode != 0) {
        for (i = 0; i < REPLAY_RIVAL_COUNT; i++) {
            if (g_Cars[i].activeFlag != -1 && g_Cars[i].aiEnabled == 1) {
                SeedReplayCarTrackState(&g_Cars[i]);
            }
        }
    }
}

void UpdateReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    s32 i;

    AdvanceReplayCarTrackState(player);

    if (g_GrandPrixMode != 0) {
        for (i = 0; i < REPLAY_RIVAL_COUNT; i++) {
            if (g_Cars[i].activeFlag != -1 && g_Cars[i].aiEnabled == 1) {
                AdvanceReplayCarTrackState(&g_Cars[i]);
            }
        }
    }

    RequestTrackTexturePage(player->trackSection);
}
