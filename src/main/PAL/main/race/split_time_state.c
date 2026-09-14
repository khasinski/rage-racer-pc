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

static void ResetSplitDisplay(RaceTiming *timing) {
    timing->sectorIndex = 0;
    timing->splitSector = 0;
    timing->splitTimer = 0;
    timing->splitSign = 0;
    timing->splitTargetTime = timing->refSectorTimes.values[0];
}

void UpdateSplitTimes(RaceTiming *timing, PlayerCarRuntime *car,
                      s32 grandPrixMode, s32 lapEvent) {
    s32 slot;
    s32 nextSlot;
    s32 targetTime;
    int64_t delta;

    if (timing == NULL || car == NULL || lapEvent == 2 || grandPrixMode != 0) {
        return;
    }

    if (timing->sectorIndex == SPLIT_STATE_WAITING_FOR_LAP) {
        if (lapEvent == 0) {
            return;
        }
        timing->sectorIndex = 0;
        timing->splitSign = 0;
        timing->splitTargetTime =
            g_BestSectorTimes[RaceSeriesIndex(g_RaceSeries)]
                             [SeriesCourseIndex()][0];
        timing->splitTimer = SPLIT_DISPLAY_FRAMES;
        timing->splitSector = 0;
        return;
    }

    slot = timing->sectorIndex;
    if (slot >= SPLIT_SECTOR_COUNT) {
        ResetSplitDisplay(timing);
        return;
    }
    if (slot >= 0 &&
        (((int64_t)car->lap - 1) * g_TrackLength +
                 timing->sectorEnds[slot] <=
             (int64_t)car->progressB + car->progressA ||
         lapEvent != 0)) {
        timing->sectorTimes[slot] = timing->lapTime;
        targetTime = lapEvent != 0 ? timing->refLapTime
                                   : timing->refSectorTimes.values[slot];
        if (timing->lapTime >= 0 && timing->lapTime <= SPLIT_TIME_MAX_MS &&
            targetTime > 0 && targetTime <= SPLIT_TIME_MAX_MS) {
            delta = (int64_t)targetTime - timing->lapTime;

            timing->splitSign = 1;
            if (delta < 0) {
                timing->splitSign = -1;
                delta = -delta;
                if (lapEvent == 0) {
                    PlaySoundCue(SPLIT_BEHIND_CUE);
                }
            } else if (delta > 0 && lapEvent == 0) {
                PlaySoundCue(SPLIT_AHEAD_CUE);
            }
            timing->splitDelta = delta < INT_MAX ? (s32)delta : INT_MAX;
        } else {
            timing->splitSign = 0;
        }

        timing->splitTimer = 0;
        nextSlot = (slot + 1) % SPLIT_SECTOR_COUNT;
        timing->sectorIndex = nextSlot;

        if (lapEvent != 0) {
            timing->splitSector = 2;
            timing->splitTargetTime = timing->refLapTime;
            timing->refLapTime = timing->bestLap;
        } else {
            const s32 closedSlot =
                (nextSlot + SPLIT_SECTOR_COUNT - 1) % SPLIT_SECTOR_COUNT;

            timing->splitSector = closedSlot;
            timing->splitTargetTime = timing->refSectorTimes.values[closedSlot];
        }

        timing->lastSectorTime = timing->sectorTimes[
            (timing->sectorIndex + SPLIT_SECTOR_COUNT - 1) % SPLIT_SECTOR_COUNT];
        return;
    }

    if (timing->sectorIndex >= 0 && g_LapCount >= car->lap) {
        if (timing->splitTimer < 0) {
            timing->splitTimer = 0;
        }
        if (timing->splitTimer < SPLIT_DISPLAY_FRAMES) {
            timing->splitTimer++;
            if (timing->splitTimer == SPLIT_DISPLAY_FRAMES) {
                timing->splitTargetTime =
                    timing->refSectorTimes.values[timing->sectorIndex];
                timing->splitSign = 0;
                timing->splitSector = (u16)timing->sectorIndex;
            }
        }
    } else {
        ResetSplitDisplay(timing);
    }
}
