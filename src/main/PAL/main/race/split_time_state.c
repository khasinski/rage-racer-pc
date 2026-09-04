#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_hud_internal.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/track.h"

#include <limits.h>
#include <stdint.h>

enum {
    SPLIT_STATE_WAITING_FOR_LAP = -2,
    SPLIT_AHEAD_CUE = 0x3E,
    SPLIT_BEHIND_CUE = 0x3F,
};

static void ResetSplitDisplay(void) {
    g_SectorIndex = 0;
    g_SplitSector = 0;
    g_SplitTimer = 0;
    g_SplitSign = 0;
    g_SplitTargetTime = g_RefSectorTimes.values[0];
}

void UpdateSplitTimes(PlayerCarRuntime *car, s32 grandPrixMode, s32 lapEvent) {
    s32 slot;
    s32 nextSlot;
    int64_t delta;

    if (car == NULL || lapEvent == 2 || grandPrixMode != 0) {
        return;
    }

    slot = g_SectorIndex;
    if (slot >= SPLIT_SECTOR_COUNT) {
        ResetSplitDisplay();
        return;
    }
    if (slot >= 0 &&
        (((int64_t)car->lap - 1) * g_TrackLength +
                 g_SectorEndDistance[slot] <=
             (int64_t)car->progressB + car->progressA ||
         lapEvent != 0)) {
        g_SectorTimes[slot] = g_LapTimeMs;
        if (g_LapTimeMs >= 0 && g_LapTimeMs <= SPLIT_TIME_MAX_MS) {
            delta = lapEvent != 0
                        ? (int64_t)g_RefLapTime - g_LapTimeMs
                        : (int64_t)g_RefSectorTimes.values[slot] -
                              g_LapTimeMs;

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
            g_SplitDelta = delta < INT_MAX ? (s32)delta : INT_MAX;
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
            g_BestSectorTimes[RaceSeriesIndex(g_RaceSeries)]
                             [SeriesCourseIndex()][0];
        g_SplitTimer = SPLIT_DISPLAY_FRAMES;
        g_SplitSector = 0;
    } else if (g_SectorIndex >= 0 && g_LapCount >= car->lap) {
        if (g_SplitTimer < 0) {
            g_SplitTimer = 0;
        }
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
        ResetSplitDisplay();
    }
}
