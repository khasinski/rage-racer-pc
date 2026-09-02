#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/track.h"

enum {
    SPLIT_SECTOR_COUNT = 3,
    SPLIT_STATE_WAITING_FOR_LAP = -2,
    SPLIT_DISPLAY_FRAMES = 60,
    MAX_COMPARABLE_TIME_MS = 599998,
    SPLIT_AHEAD_CUE = 0x3E,
    SPLIT_BEHIND_CUE = 0x3F,
};

void UpdateSplitTimes(PlayerCarRuntime *car, s32 grandPrixMode, s32 lapEvent) {
    s32 slot;
    s32 nextSlot;
    s32 delta;

    if (lapEvent == 2 || grandPrixMode != 0) {
        return;
    }

    slot = g_SectorIndex;
    if (slot >= 0 &&
        ((car->lap - 1) * g_TrackLength + g_SectorEndDistance[slot] <=
             car->progressB + car->progressA ||
         lapEvent != 0)) {
        g_SectorTimes[slot] = g_LapTimeMs;
        if (g_LapTimeMs <= MAX_COMPARABLE_TIME_MS) {
            delta = lapEvent != 0
                        ? g_RefLapTime - g_LapTimeMs
                        : g_RefSectorTimes.values[slot] - g_LapTimeMs;

            g_SplitSign = 1;
            if (delta < 0) {
                g_SplitSign = -1;
                delta = -delta;
                if (lapEvent == 0) {
                    PlaySoundCue(SPLIT_BEHIND_CUE);
                }
            } else if (delta > 0 && lapEvent == 0) {
                PlaySoundCue(SPLIT_AHEAD_CUE);
            }
            g_SplitDelta = delta;
        } else {
            g_SplitSign = 0;
        }

        g_SplitTimer = 0;
        nextSlot = (slot + 1) % SPLIT_SECTOR_COUNT;
        g_SectorIndex = nextSlot;

        if (lapEvent != 0) {
            g_SplitSector = 2;
            g_SplitTargetTime = g_RefLapTime;
            g_RefLapTime = g_BestLapThisRace;
        } else {
            const s32 closedSlot =
                (nextSlot + SPLIT_SECTOR_COUNT - 1) % SPLIT_SECTOR_COUNT;

            g_SplitSector = closedSlot;
            g_SplitTargetTime = g_RefSectorTimes.values[closedSlot];
        }

        g_LastSectorTime = g_SectorTimes[
            (g_SectorIndex + SPLIT_SECTOR_COUNT - 1) % SPLIT_SECTOR_COUNT];
        return;
    }

    if (g_SectorIndex == SPLIT_STATE_WAITING_FOR_LAP && lapEvent != 0) {
        g_SectorIndex = 0;
        g_SplitSign = 0;
        g_SplitTargetTime =
            g_BestSectorTimes[g_RaceSeries][SeriesCourseIndex()][0];
        g_SplitTimer = SPLIT_DISPLAY_FRAMES;
        g_SplitSector = 0;
    } else if (g_SectorIndex >= 0 && g_LapCount >= car->lap) {
        if (g_SplitTimer < SPLIT_DISPLAY_FRAMES) {
            g_SplitTimer++;
            if (g_SplitTimer == SPLIT_DISPLAY_FRAMES) {
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
