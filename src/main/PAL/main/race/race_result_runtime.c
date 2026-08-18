#include "game/audio.h"
#include "game/cd.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_clock.h"
#include "game/race_internal.h"
#include "game/race_result_runtime.h"
#include "game/records_internal.h"
#include "game/save_internal.h"
#include "game/render.h"
#include "game/state.h"

static void StoreWinningRaceRecords(PlayerCarRaceState *raceState,
                                    s32 lapCount,
                                    s32 grandPrixMode) {
    s32 index = 0;
    s32 tableOffset;
    s32 *cursor;
    PlayerCarRaceStateAddress address;
    SectorTimeTableAddress sectorAddress;

    if (lapCount > 0) {
        address.state = raceState;
        cursor = address.words;
        do {
            address.words = cursor;
            g_RaceTotalTime +=
                address.state->timing.fields.lapTimes.table.milliseconds[0];
            cursor++;
        } while (++index < lapCount);
    }
    g_RaceTotalTime = RaceClockSaturateMilliseconds(g_RaceTotalTime);
    if (g_BestLapTimes[ReadStableRaceSeries()][RageSeriesCourseIndex()]
                      [grandPrixMode] > g_BestLapThisRace) {
        g_BestLapTimes[ReadStableRaceSeries()][RageSeriesCourseIndex()]
                      [grandPrixMode] = g_BestLapThisRace;
    }
    if (grandPrixMode == 0) {
        tableOffset = RageSeriesCourseIndex() * 12 +
                      ReadStableRaceSeries() * 48;
        sectorAddress.table = g_BestSectorTimes;
        sectorAddress.bytes += tableOffset;
        sectorAddress.pointer[0] = g_RefSectorTimes.fields.first;
        sectorAddress.pointer = &g_BestSectorTimes[0][0][1];
        sectorAddress.bytes += tableOffset;
        sectorAddress.pointer[0] = g_RefSectorTime1;
        sectorAddress.pointer = &g_BestSectorTimes[0][0][2];
        sectorAddress.bytes += tableOffset;
        sectorAddress.pointer[0] = g_RefSectorTime2;
    }
}

void ApplyRaceFinishResult(PlayerCarRaceState *raceState,
                           s32 lapCount,
                           s32 grandPrixMode,
                           RaceResult result) {
    if (result == RACE_RESULT_WON) {
        StoreWinningRaceRecords(raceState, lapCount, grandPrixMode);
        g_RacePhase = 4;
        StartCdVolumeFade(8);
        PlaySoundCue(0x2B);
    } else if (result == RACE_RESULT_LOST) {
        g_RacePhase = 5;
        SeedFinishCamera(&g_PlayerCar);
        StartCdVolumeFade(0x3C);
        if (g_CourseProgress->retriesRemaining != 0)
            PlaySoundCue(0x3D);
    } else {
        return;
    }

    ForceAllEffectVoicesEnabled(0);
    g_RaceFadeTimer = 0;
    g_MirrorViewEnabled = 0;
}
