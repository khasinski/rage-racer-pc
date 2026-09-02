#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/track.h"

void UpdateSplitTimes(PlayerCarRuntime *car, s32 grandPrixMode, s32 lapEvent) {
    s32 slot;
    s32 nextSlot;
    s32 delta;
    s32 sectorClosed = 0;

    if (lapEvent == 2 || grandPrixMode != 0) {
        return;
    }

    slot = g_SectorIndex;
    if (slot >= 0 &&
        ((car->lap - 1) * g_TrackLength + g_SectorEndDistance[slot] <=
             car->progressB + car->progressA ||
         lapEvent != 0)) {
        g_SectorTimes[slot] = g_LapTimeMs;
        if (g_LapTimeMs <= 0x927BE) {
            delta = lapEvent != 0
                        ? g_RefLapTime - g_LapTimeMs
                        : g_RefSectorTimes.values[slot] - g_LapTimeMs;

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
        nextSlot = (slot + 1) % 3;
        g_SectorIndex = nextSlot;

        if (lapEvent != 0) {
            g_SplitSector = 2;
            g_SplitTargetTime = g_RefLapTime;
            g_RefLapTime = g_BestLapThisRace;
        } else {
            const s32 closedSlot = (nextSlot + 2) % 3;

            g_SplitSector = closedSlot;
            g_SplitTargetTime = g_RefSectorTimes.values[closedSlot];
        }

        g_LastSectorTime = g_SectorTimes[(g_SectorIndex + 2) % 3];
        sectorClosed = 1;
    }

    if (sectorClosed == 0) {
        if (g_SectorIndex == -2 && lapEvent != 0) {
            g_SectorIndex = 0;
            g_SplitSign = 0;
            g_SplitTargetTime =
                g_BestSectorTimes[g_RaceSeries][SeriesCourseIndex()][0];
            g_SplitTimer = 0x3C;
            g_SplitSector = 0;
        } else if (g_SectorIndex >= 0 && g_LapCount >= car->lap) {
            if (g_SplitTimer < 0x3C) {
                g_SplitTimer++;
                if (g_SplitTimer == 0x3C) {
                    g_SplitTargetTime =
                        g_RefSectorTimes.values[g_SectorIndex];
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
}
