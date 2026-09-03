/*
 * Lap counting and the end of a race.
 *
 * Every frame of a race this ticks the running lap's clock, and when the car
 * crosses the line it writes the lap into the times the game keeps, then
 * either starts the next lap, finishes the race, or retires it. What it
 * returns is what the race scene reads to know which of those happened: zero
 * for nothing, one for a lap, two once the finish fade has begun.
 */

#include "game/audio.h"
#include "game/player_car_internal.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/replay_internal.h"
#include "game/random.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"
#include "game/track.h"

#include <stdint.h>

enum { LAP_FRAME_COUNT_MAX = 0x10000 };
/* A lap's clock counts frames, and the frame count is converted to a time as
 * it goes. Both saturate: 0x10000 frames and just under ten minutes. */
static void TickRunningLapTime(PlayerCarRuntime *car) {
    PlayerLapTimes *times = &car->lapTimes;
    s32 slot = car->lap - 1;

    if ((u32)slot >= PLAYER_LAP_TIME_CAPACITY) {
        return;
    }
    if (times->table.frameCounts[slot] < 0) {
        times->table.frameCounts[slot] = 0;
    } else if (times->table.frameCounts[slot] < LAP_FRAME_COUNT_MAX) {
        times->table.frameCounts[slot]++;
    } else {
        times->table.frameCounts[slot] = LAP_FRAME_COUNT_MAX;
    }
    times->table.milliseconds[slot] =
        FramesToMilliseconds(times->table.frameCounts[slot], Random15() % 40);
    if (times->table.milliseconds[slot] >= RACE_TIME_MAX_MS) {
        times->table.milliseconds[slot] = RACE_TIME_MAX_MS;
    }
    g_LapTimeMs = times->table.milliseconds[slot];
}

/*
 * The lap just completed, if it beats the best of this race, becomes the new
 * best and its sector times become the ones the next lap is measured against.
 */
static void RecordBestLap(PlayerCarRuntime *car, s32 recordMode) {
    s32 lap = car->lap;
    s32 lapTime;

    if (lap < 2 || lap > PLAYER_LAP_TIME_CAPACITY + 1) {
        return;
    }
    lapTime = car->lapTimes.table.milliseconds[lap - 2];
    if (lapTime >= g_BestLapThisRace) {
        return;
    }
    car->drive.hudLapHighlightRow = (s16)((u16)lap - 2);
    g_BestLapThisRace = lapTime;
    g_SectorTimes[2] = lapTime;
    if (recordMode == 0) {
        g_RefSectorTimes.fields.first = g_SectorTimes[0];
        g_RefSectorTimes.fields.second = g_SectorTimes[1];
        g_RefSectorTimes.fields.third = lapTime;
    }
    /* Announced only while there are still laps left to run. */
    if (g_LapCount >= lap) {
        PlaySoundCue(0x26);
        g_RaceCueDelay = 0x96;
    }
}

/* The race is over and the player finished it: add up the laps, keep whatever
 * beats the records, and hand over to the finish sequence. */
static void FinishRace(PlayerCarRuntime *car, s32 recordMode,
                       s32 lapsRun) {
    s32 series = RaceSeriesIndex(g_RaceSeries);
    s32 course = SeriesCourseIndex();
    int64_t totalTime = g_RaceTotalTime;
    s32 i;

    if (lapsRun < 0) {
        lapsRun = 0;
    } else if (lapsRun > PLAYER_LAP_TIME_CAPACITY) {
        lapsRun = PLAYER_LAP_TIME_CAPACITY;
    }
    if (totalTime < 0) {
        totalTime = 0;
    }
    for (i = 0; i < lapsRun; i++) {
        s32 lapTime = car->lapTimes.table.milliseconds[i];

        if (lapTime > 0) {
            totalTime += lapTime;
        }
    }
    g_RaceTotalTime = totalTime < RACE_TIME_MAX_MS
                          ? (s32)totalTime
                          : RACE_TIME_MAX_MS;
    if (g_BestLapTimes[series][course][recordMode] > g_BestLapThisRace) {
        g_BestLapTimes[series][course][recordMode] = g_BestLapThisRace;
    }
    if (recordMode == 0) {
        g_BestSectorTimes[series][course][0] = g_RefSectorTimes.fields.first;
        g_BestSectorTimes[series][course][1] = g_RefSectorTimes.fields.second;
        g_BestSectorTimes[series][course][2] = g_RefSectorTimes.fields.third;
    }
    g_RacePhase = 4;
    StartCdVolumeFade(8);
    /* TriggerRaceCues runs later in the frame, but phase 4 skips that whole
     * block. Guarantee the spoken FINISHED cue at the state transition itself
     * instead of depending on the car remaining in one authored finish-line
     * track section. */
    g_RaceCueFlags |= 8;
    PlaySoundCue(0x2A);
    QueueFinishFollowupCue(0x2B);
}

/* The race is over and the player did not finish it: no records, just the
 * camera pulling away. */
static void RetireAtLastLap(void) {
    g_RacePhase = 5;
    SeedFinishCamera(&g_PlayerCar);
    StartCdVolumeFade(0x3C);
    if (g_CourseProgress != NULL &&
        g_CourseProgress->retriesRemaining != 0) {
        PlaySoundCue(0x3D);
    }
}

/* The car has covered the distance the current lap needs. */
static s32 CrossTheLine(PlayerCarRuntime *car, s32 recordMode) {
    s32 lapsRun;

    car->lap += 1;
    g_RaceCueFlags &= 0xF;
    if (g_RaceCueDelay == 0) {
        g_RaceCueDelay = 2;
    }
    RecordBestLap(car, recordMode);

    lapsRun = g_LapCount;
    if (car->lap == lapsRun + 1) {
        /* Anything below fourth is not a finish; the race is retired. */
        if (car->drive.racePosition < 4) {
            FinishRace(car, recordMode, lapsRun);
        } else {
            RetireAtLastLap();
        }
        ForceAllEffectVoicesEnabled(0);
        g_RaceFadeTimer = 0;
        g_MirrorViewEnabled = 0;
    }
    return 1;
}

/*
 * After the finish the screen fades out, and the fade doubles as the timer
 * that starts the results music and then the replay.
 */
static s32 AdvanceFinishFade(s32 returnValue) {
    s32 fadeTimer = g_RaceFadeTimer;

    if (fadeTimer < 0) {
        fadeTimer = 0;
    } else if (fadeTimer > 0x83) {
        fadeTimer = 0x83;
    }
    DrawFullscreenFadeTile(fadeTimer * 2, 0x29);
    if (fadeTimer >= 2) {
        returnValue = 2;
    }
    if (fadeTimer < 0x83) {
        fadeTimer++;
    }
    g_RaceFadeTimer = (s16)fadeTimer;
    if (g_RaceFadeTimer == 0x3F) {
        if (g_GrandPrixMode != 0) {
            CommitClassProgress();
            RequestCdTrack((g_SeriesCleared == 1) ? 0x10 : 0xC);
        } else {
            g_SeriesCleared = 0;
            RequestCdTrack(0xD);
        }
    }
    if (g_RaceFadeTimer >= 0x83) {
        BeginReplay();
        ExitRaceScene(0x11);
        StartCdAudio();
    }
    return returnValue;
}

/* Outside a Grand Prix, a car a whole lap behind the line or stuck facing the
 * wrong way for a second is retired where it stands. */
static void RetireWrongWay(void) {
    s32 series = RaceSeriesIndex(g_RaceSeries);
    s32 course = SeriesCourseIndex();

    g_RacePhase = 5;
    g_BestLapTimes[series][course][0] =
        g_RankingRecords[series][course][0].raceTime;
    StartCdVolumeFade(8);
    ForceAllEffectVoicesEnabled(0);
    g_RaceFadeTimer = 0;
    SeedFinishCamera(&g_PlayerCar);
}

/* Two laps out, one lap out and the last lap each get their own call. */
static void CountDownTheLaps(PlayerCarRuntime *car) {
    if (g_RaceCueDelay == 2) {
        switch (g_LapCount - car->lap) {
        case 2:
            PlaySoundCue(0x27);
            break;
        case 1:
            PlaySoundCue(0x28);
            break;
        case 0:
            PlaySoundCue(0x29);
            break;
        }
        g_RaceCueDelay--;
    } else if (g_RaceCueDelay == 1) {
        g_RaceCueDelay = 0;
        g_RivalCueEnabled = 2;
    } else if (g_RaceCueDelay > 0) {
        g_RaceCueDelay--;
    }
}

static int HasCrossedCurrentLapLine(s32 lap, s32 trackLength,
                                    s32 progressA, s32 progressB) {
    return (int64_t)lap * trackLength <=
           (int64_t)progressA + progressB;
}

static int IsWholeLapBehind(s32 trackLength, s32 progressA, s32 progressB) {
    return (int64_t)progressA + progressB <= -(int64_t)trackLength;
}

s32 UpdateLapAndFinish(PlayerCarRuntime *car, s32 grandPrixMode) {
    s32 series = RaceSeriesIndex(g_RaceSeries);
    s32 course = SeriesCourseIndex();
    s32 recordMode = RaceRecordMode(grandPrixMode);
    s16 lapAtEntry;
    u16 returnValue;

    if (car == NULL) {
        return 0;
    }

    if ((car->lap > 0) && (g_LapCount >= car->lap)) {
        TickRunningLapTime(car);
    } else if (g_LapCount < car->lap) {
        /* Past the last lap the clock stops and the total is kept instead. */
        if (g_RaceTotalTime < g_BestTotalTimes[series][course][recordMode]) {
            g_BestTotalTimes[series][course][recordMode] = g_RaceTotalTime;
        }
    }

    lapAtEntry = car->lap;
    if (lapAtEntry > 0 && lapAtEntry <= PLAYER_LAP_TIME_CAPACITY &&
        HasCrossedCurrentLapLine(lapAtEntry, g_TrackLength,
                                car->progressA, car->progressB) &&
        (lapAtEntry <= g_LapCount)) {
        returnValue = (u16)CrossTheLine(car, recordMode);
    } else {
        returnValue = 0;
    }

    if ((g_LapCount < car->lap) && (g_RacePhase == 4)) {
        returnValue = (u16)AdvanceFinishFade(returnValue);
    } else if ((g_GrandPrixMode == 0) &&
               (IsWholeLapBehind(g_TrackLength, car->progressA,
                                 car->progressB) ||
                ((car->lap == 0) && (g_WrongWayTimer >= 0x3C)))) {
        RetireWrongWay();
    }

    CountDownTheLaps(car);
    UpdateRivalCueGate();
    return returnValue;
}
