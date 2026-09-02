#include <stdio.h>
#include "game/asset.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "game/audio.h"
#include "game/save_internal.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/player_car_internal.h"

#include "rage/hud_config.h"


void SeedReplayCars(void) {
    GameCarRuntime *player = AsRivalCar(&g_PlayerCar);
    GameCarRuntime *rival = &g_Cars[0];

    InitShuttleScenery();

    ApplyReplayFrameAndTilt(g_ReplayReadCursor, player, rival);

    g_PlayerCar.trackPointIndex =
        FindTrackSegment(player, g_PlayerCar.trackPointIndex);
    SeedCarLapProgress(player, 1);
    AccumulateLapProgress(player);
    ResetCarTrackState(player);

    if (g_GrandPrixMode == 1) {
        g_Cars[0].trackPointIndex =
            FindTrackSegment(rival, g_Cars[0].trackPointIndex);
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

    RequestTrackTexturePage(g_PlayerCar.trackSection);
}

void ExitRaceScene(s32 sceneId) {
    g_SceneId = sceneId;
    ForceAllEffectVoicesEnabled(0);
    SetReverbDepth(0, 0);
    if (g_SceneId == 6) {
        RequestSelectBgmAssets();
    }
    printf("%s", &g_MsgGameExit);
}

void UpdateSplitTimes(PlayerCarRuntime *car, s32 grandPrixMode, s32 lapEvent) {
    s32 slot;
    s32 nextSlot;
    s32 delta;
    s32 sectorClosed;

    if (lapEvent == 2 || grandPrixMode != 0) {
        return;
    }

    sectorClosed = 0;
    slot = g_SectorIndex;
    if (slot >= 0) {
        if ((car->lap - 1) * g_TrackLength + g_SectorEndDistance[slot] <=
                (car->progressB + car->progressA) ||
            lapEvent != 0) {
            g_SectorTimes[slot] = g_LapTimeMs;
            if (g_LapTimeMs <= 0x927BE) {
                if (lapEvent != 0) {
                    delta = g_RefLapTime - g_LapTimeMs;
                } else {
                    delta = g_RefSectorTimes.values[slot] - g_LapTimeMs;
                }

                g_SplitSign = 1;
                if (delta < 0) {
                    g_SplitSign = -1;
                    delta = -delta;
                    if (lapEvent == 0) {
                        PlaySoundCue(0x3F);
                    }
                } else if (delta > 0 && lapEvent == 0) {
                    PlaySoundCue(0x3E);
                }
                g_SplitDelta = delta;
            } else {
                g_SplitSign = 0;
            }

            g_SplitTimer = 0;
            nextSlot = (g_SectorIndex + 1) % 3;
            g_SectorIndex = nextSlot;

            if (lapEvent != 0) {
                g_SplitSector = 2;
                g_SplitTargetTime = g_RefLapTime;
                g_RefLapTime = g_BestLapThisRace;
            } else {
                nextSlot = (nextSlot + 2) % 3;
                g_SplitSector = nextSlot;
                g_SplitTargetTime = g_RefSectorTimes.values[nextSlot];
            }

            nextSlot = (g_SectorIndex + 2) % 3;
            g_LastSectorTime = g_SectorTimes[nextSlot];
            sectorClosed = 1;
        }
    }

    /* Closing a sector has already set the next target, so the rest is only
     * for a frame that did not close one. */
    if (!sectorClosed) {
        if (g_SectorIndex == -2 && lapEvent != 0) {
            g_SectorIndex = 0;
            g_SplitSign = 0;
            g_SplitTargetTime =
                g_BestSectorTimes[g_RaceSeries][SeriesCourseIndex()][0];
            g_SplitTimer = 0x3C;
            g_SplitSector = (u16)g_SectorIndex;
        } else if (g_SectorIndex >= 0 &&
                   g_LapCount >= car->lap) {
            /* Hold the split on screen for a second before showing the next
             * sector's target. */
            if (g_SplitTimer < 0x3C) {
                g_SplitTimer++;
                if (g_SplitTimer == 0x3C) {
                    g_SplitTargetTime = g_RefSectorTimes.values[g_SectorIndex];
                    g_SplitSign = 0;
                    g_SplitSector = (u16)g_SectorIndex;
                }
            }
        } else {
            g_SplitSector = 0;
            g_SplitTimer = 0;
            g_SplitSign = 0;
            g_SplitTargetTime = g_RefSectorTimes.values[0];
        }
    }

    /*
     * The same drawing DrawSplitTimes does during a race. Keeping a second
     * copy here is what let the two drift: this one kept the coordinates a
     * 4:3 screen was authored with long after the other was anchored, and it
     * never learned that lap times can be turned off.
     */
    DrawSplitTimes();
}
