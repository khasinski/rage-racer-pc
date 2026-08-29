#include <stdio.h>
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"
#include "game/audio.h"
#include "game/save_internal.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/player_car_internal.h"


void SeedReplayCars(void) {
    ReplayCarAddress primary;
    ReplayCarAddress secondary;

    InitShuttleScenery();

    primary.player = &g_PlayerCar;
    secondary.rivals = g_Cars;
    ApplyReplayFrameAndTilt(g_ReplayReadCursor, primary.state, secondary.state);

    g_PlayerCar.trackPointIndex = FindTrackSegment(primary.runtime, g_PlayerCar.trackPointIndex);
    SeedCarLapProgress(primary.runtime, 1);
    AccumulateLapProgress(primary.runtime);
    ResetCarTrackState(primary.runtime);

    if (g_GrandPrixMode == 1) {
        g_Cars[0].trackPointIndex =
            FindTrackSegment(secondary.runtime, g_Cars[0].trackPointIndex);
        SeedCarLapProgress(secondary.runtime, 1);
        AccumulateLapProgress(secondary.runtime);
        ResetCarTrackState(secondary.runtime);
    }
}

void UpdateReplayCars(void) {
    void *ptr = &g_PlayerCar;

    AccumulateLapProgress(ptr);
    ResetCarTrackState(ptr);

    if (g_GrandPrixMode == 1) {
        ptr = g_Cars;
        AccumulateLapProgress(ptr);
        ResetCarTrackState(ptr);
    }

    RequestTrackTexturePage(g_PlayerCar.trackSection);
}

s32 GetTrackZoneBlend(s32 position) {
    TrackEventData *data;
    s32 scene;
    TrackZoneAddress first;
    TrackZoneAddress zone;
    s32 status;
    s32 start;
    s32 finish;
    s32 code;

    data = g_TrackEventData;
    scene = g_RaceSeries;
    first.pointer = data->zones;
    if (scene != 0) {
        position = g_TrackLength - position;
    }

    status = 0;
    zone.pointer = first.pointer;
    g_TrackZoneCode = 0;
    g_ReverbZoneDepth = 0;
    g_TrackZoneDark = 0;

    do {
        start = zone.pointer->start;
        finish = zone.pointer->end;
        if (start == -1) {
            break;
        }

        if ((start < position) && (position < finish)) {
            /* How far into the zone: just inside either end, or well in. */
            if (position < start + 0x100) {
                status = 1;
            } else if (finish - 0x100 < position) {
                status = 2;
            } else {
                status = 3;
            }

            code = (s16)zone.pointer->code;
            g_TrackZoneCode = code;
            if (code == 0) {
                /* A zone with no code of its own only darkens the scene. */
                g_TrackZoneDark = 3;
            } else if (code == 2 || code == -3) {
                /* The two codes that stand for zone 1. Code 2 counts as
                 * being well inside whatever the position said; code -3
                 * only upgrades the far end. */
                if (code == 2) {
                    status = 4;
                } else if (status == 2) {
                    status = 3;
                }
                g_TrackZoneCode = 1;
            } else if (code < 0) {
                /* Any other negative code is the zone number, negated to
                 * mark the whole zone as well inside. */
                g_TrackZoneCode = -code;
                status = 3;
            }
            g_ReverbZoneDepth = zone.pointer->value;
        }

        if (status > 0) {
            break;
        }
        zone.pointer++;
    } while (zone.value < first.value + 0xF0);

    switch (status) {
    case 1:
        return position - start;
    case 2:
        return finish - position;
    case 3:
        return 0x100;
    default:
        return 0;
    }
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
    s32 value;
    s32 tile;
    s32 timeout;
    s32 threshold;
    PlayerCarRaceState *raceState;
    SectorTimeTableAddress sectorAddress;
    s32 sectorClosed;

    raceState = GetPlayerCarRaceState(car);

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
            nextSlot = g_SectorIndex;
            nextSlot++;
            nextSlot %= 3;
            g_SectorIndex = nextSlot;

            if (lapEvent != 0) {
                g_SplitSector = 2;
                g_SplitTargetTime = g_RefLapTime;
                g_RefLapTime = g_BestLapThisRace;
            } else {
                nextSlot += 2;
                nextSlot %= 3;
                g_SplitSector = nextSlot;
                g_SplitTargetTime = g_RefSectorTimes.values[nextSlot];
            }

            nextSlot = g_SectorIndex;
            nextSlot += 2;
            nextSlot %= 3;
            nextSlot <<= 2;
            sectorAddress.pointer = g_SectorTimes;
            sectorAddress.bytes += nextSlot;
            g_LastSectorTime = *sectorAddress.pointer;
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
                   g_LapCount >= raceState->timing.fields.lap) {
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

    if (g_SplitTimer >= 0x3C) {
        threshold = 0x927BE;
        value = g_LapTimeMs;

    } else if (g_SectorIndex >= 0) {
        if (g_SplitSign != 0) {
            if (g_LapCount >= g_PlayerCar.lap) {
                value = g_SplitDelta;
                if (g_SplitSign > 0) {
                    tile = 0x7810;
                } else {
                    tile = 0x780F;
                }
                DrawTimeValue(0x80, 0x50, value, tile, 0x3E8);
            }
        }
        threshold = 0x927BE;
        value = g_LastSectorTime;

        tile = value <= threshold ? 0x78CC : 0x7890;
        DrawTimeValue(0x12, 0x2A, value, tile, 0x3E8);
    }

    timeout = 0x3E8;
    DrawTimeValue(0x12, 0x20, g_SplitTargetTime, 0x78CC, timeout);
    DrawSplitDelta(g_SplitSector, g_SplitSign);

    DrawTimeValue(
        0xFA,
        0x7C,
        g_BestTotalTimes[g_RaceSeries][SeriesCourseIndex()][grandPrixMode],
        0x78CC,
        timeout);
}
