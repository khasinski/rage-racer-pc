/*
 * Lap counting and the end of a race, swept.
 *
 * UpdateLapAndFinish is where a lap ticks over, where the lap and sector times
 * are written into the records the game keeps, where the race is declared
 * finished or retired, and where the finish fade counts down to the replay.
 * Two hundred and thirty lines nested eight deep, and nothing tested any of
 * it.
 *
 * So this walks the states that decide which branch runs, folds everything it
 * writes and every call it makes into one number, and pins the value it
 * returns, which is what the race scene reads to know what happened.
 */

#include "common.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

s32 g_BestLapThisRace;
s32 g_BestLapTimes[2][4][2];
s32 g_BestSectorTimes[2][4][3];
s32 g_BestTotalTimes[2][4][2];
CourseProgressState *g_CourseProgress;
s32 g_CourseIndex;
s16 g_GrandPrixMode;
s32 g_LapCount;
s32 g_LapTimeMs;
s32 g_LapTimeSaturated;
s16 g_MirrorViewEnabled;
PlayerCarRuntime g_PlayerCar;
s32 g_RaceSeries;
s16 g_RaceCueDelay;
s32 g_RaceCueFlags;
s16 g_RaceFadeTimer;
s16 g_RacePhase;
s32 g_RaceTotalTime;
RaceRecord g_RankingRecords[2][4][5];
s32 g_RefSectorTime1;
s32 g_RefSectorTime2;
SectorReferenceTimes g_RefSectorTimes;
s16 g_RivalCueEnabled;
s32 g_SectorTimes[3];
s32 g_SeriesCleared;
s32 g_TrackLength;
s16 g_WrongWayTimer;

static unsigned long s_digest = 2166136261UL;
static FILE *s_out;
static int s_calls;

static void Fold(unsigned char byte) {
    s_digest = ((s_digest ^ byte) * 16777619UL) & 0xFFFFFFFFUL;
}

/* The digest folds raw values rather than their text; the readable form is
 * only produced when a file was asked for. */
static void Record(const char *name, const s32 *values, int count) {
    const char *p;
    int i;

    for (p = name; *p != '\0'; p++) {
        Fold((unsigned char)*p);
    }
    for (i = 0; i < count; i++) {
        u32 value = (u32)values[i];

        Fold((unsigned char)value);
        Fold((unsigned char)(value >> 8));
        Fold((unsigned char)(value >> 16));
        Fold((unsigned char)(value >> 24));
    }
    if (s_out != NULL) {
        fputs(name, s_out);
        for (i = 0; i < count; i++) {
            fprintf(s_out, " %d", values[i]);
        }
        fputc('\n', s_out);
    }
    s_calls++;
}

#define RECORD(name, ...)                                                      \
    do {                                                                       \
        s32 v[] = {__VA_ARGS__};                                               \
        Record(name, v, (int)(sizeof(v) / sizeof(v[0])));                       \
    } while (0)

/*
 * The millisecond time a lap is recorded at is the frame count converted with
 * a few milliseconds of jitter taken from the random generator. The sweep sets
 * the jitter instead of rolling it, because the saturation limit is only
 * reachable at particular values of it.
 */
static s32 s_jitter = 7;

s32 Random15(void) { return s_jitter; }
void PlaySoundCue(s32 cue) { RECORD("cue", cue); }
void QueueFinishFollowupCue(s32 cue) { RECORD("followup", cue); }
void SeedFinishCamera(PlayerCarRuntime *car) {
    RECORD("finishcamera", car == &g_PlayerCar);
}
void StartCdVolumeFade(s32 frames) { RECORD("cdfade", frames); }
void ForceAllEffectVoicesEnabled(s32 enabled) { RECORD("voices", enabled); }
void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    RECORD("fadetile", color, tpage);
}
void CommitClassProgress(void) { RECORD("commitclass", 0); }
void RequestCdTrack(s32 track) { RECORD("cdtrack", track); }
void BeginReplay(void) { RECORD("replay", 0); }
void ExitRaceScene(s32 sceneId) { RECORD("exit", sceneId); }
void StartCdAudio(void) { RECORD("cdaudio", 0); }
void UpdateRivalCueGate(void) { RECORD("rivalgate", 0); }

static CourseProgressState s_course;

int main(int argc, char **argv) {
    /*
     * What the lap update did before it was taken apart. Run the test with a
     * file name to write the sweep out and diff two runs.
     */
    /* Digest produced with the real PAL 25 Hz frame-to-millisecond
     * conversion, not the former 20 ms test double. */
    static const unsigned long expected = 3315525677UL;
    static const s32 laps[] = {0, 1, 2, 3};
    static const s32 lapCounts[] = {2, 3};
    /* Where the car is against the distance the current lap needs: short of
     * it, exactly on it, past it, and a whole lap the wrong way. */
    static const s32 progressCases[] = {0, 1, 2, 3};
    static const s32 fadeTimers[] = {0, 1, 2, 0x3E, 0x82};
    static const s32 cueDelays[] = {0, 1, 2, 3};
    int li, lc, pc, gp, pos, phase, ft, cd, best, retries, cleared, gpGlobal,
        wrongWay, saturating;
    int steps = 0;

    if (argc > 1) {
        s_out = fopen(argv[1], "w");
        if (s_out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    g_CourseProgress = &s_course;
    for (li = 0; li < 4; li++)
    for (lc = 0; lc < 2; lc++)
    for (pc = 0; pc < 4; pc++)
    for (gp = 0; gp < 2; gp++)
    for (pos = 0; pos < 2; pos++)
    for (phase = 0; phase < 2; phase++)
    for (ft = 0; ft < 5; ft++)
    for (cd = 0; cd < 4; cd++)
    for (best = 0; best < 2; best++)
    for (retries = 0; retries < 2; retries++)
    for (cleared = 0; cleared < 2; cleared++)
    for (gpGlobal = 0; gpGlobal < 2; gpGlobal++)
    for (wrongWay = 0; wrongWay < 2; wrongWay++)
    for (saturating = 0; saturating < 2; saturating++) {
        char label[224];
        s32 result;
        s32 wanted;
        int i;

        memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
        memset(&s_course, 0, sizeof(s_course));
        memset(g_BestLapTimes, 0, sizeof(g_BestLapTimes));
        memset(g_BestSectorTimes, 0, sizeof(g_BestSectorTimes));
        memset(g_BestTotalTimes, 0, sizeof(g_BestTotalTimes));
        memset(g_RankingRecords, 0, sizeof(g_RankingRecords));
        memset(g_SectorTimes, 0, sizeof(g_SectorTimes));

        g_TrackLength = 0x10000;
        g_RaceSeries = 1;
        g_CourseIndex = 2;
        for (i = 0; i < 2; i++) {
            int j, k;

            for (j = 0; j < 4; j++) {
                for (k = 0; k < 3; k++) {
                    g_BestSectorTimes[i][j][k] = 90000 + k;
                }
                for (k = 0; k < 2; k++) {
                    g_BestLapTimes[i][j][k] = 70000;
                    g_BestTotalTimes[i][j][k] = 200000;
                }
                for (k = 0; k < 5; k++) {
                    g_RankingRecords[i][j][k].raceTime = 123456 + k;
                }
            }
        }

        g_PlayerCar.lap = (s16)laps[li];
        /* A lap either well inside the counters or right on the edge of them:
         * the frame count saturates at 0xFFFF and the millisecond time at
         * 0x927BE, and both limits are only reachable from close by. */
        for (i = 0; i < 6; i++) {
            g_PlayerCar.lapTimes.table.frameCounts[i] =
                saturating ? 0xFFFF : (500 + i * 100);
            g_PlayerCar.lapTimes.table.milliseconds[i] =
                saturating ? (0x927BE - 1 + i) : (60000 + i * 1000);
        }
        g_PlayerCar.drive.racePosition = (s16)(pos ? 5 : 3);

        /* The lap ticks over when the distance covered reaches lap * length. */
        wanted = laps[li] * g_TrackLength;
        switch (progressCases[pc]) {
        case 0:
            g_PlayerCar.progressA = wanted - 1;
            break;
        case 1:
            g_PlayerCar.progressA = wanted;
            break;
        case 2:
            g_PlayerCar.progressA = wanted + 0x8000;
            break;
        default:
            g_PlayerCar.progressA = -g_TrackLength;
            break;
        }
        g_PlayerCar.progressB = 0;
        /* The lap the sweep set above IS g_PlayerCar.lap: the race state
         * union lands on it at offset 0x168. Retiring the wrong way needs
         * that lap to be zero, which the lap axis supplies. */
        g_WrongWayTimer = (s16)(wrongWay ? 0x3C : 0);

        g_LapCount = lapCounts[lc];
        g_RacePhase = (s16)(phase ? 4 : 0);
        g_RaceFadeTimer = (s16)fadeTimers[ft];
        g_RaceCueDelay = (s16)cueDelays[cd];
        g_RaceCueFlags = 0xFF;
        g_BestLapThisRace = best ? 0x7FFFFFFF : 100;
        g_RaceTotalTime = 150000;
        g_LapTimeMs = 0;
        g_LapTimeSaturated = 0;
        g_MirrorViewEnabled = 1;
        g_RivalCueEnabled = 0;
        g_SeriesCleared = cleared;
        g_GrandPrixMode = (s16)gpGlobal;
        g_RefSectorTime1 = 0;
        g_RefSectorTime2 = 0;
        g_RefSectorTimes.fields.first = 0;
        s_course.retriesRemaining = (s16)retries;

        sprintf(label,
                "== lap%d/count%d/progress%d/gp%d/pos%d/phase%d/fade%d/cue%d/"
                "best%d/retries%d/cleared%d/gpglobal%d/wrong%d/sat%d",
                laps[li], lapCounts[lc], progressCases[pc], gp, pos, phase,
                fadeTimers[ft], cueDelays[cd], best, retries, cleared,
                gpGlobal, wrongWay, saturating);
        Record(label, NULL, 0);

        result = UpdateLapAndFinish(&g_PlayerCar, gp);

        {
            s32 after[20];

            after[0] = result;
            after[1] = g_PlayerCar.lap;
            after[2] = g_PlayerCar.drive.hudLapHighlightRow;
            after[3] = g_RacePhase;
            after[4] = g_RaceFadeTimer;
            after[5] = g_RaceCueDelay;
            after[6] = g_RaceCueFlags;
            after[7] = g_RaceTotalTime;
            after[8] = g_BestLapThisRace;
            after[9] = g_LapTimeMs;
            after[10] = g_LapTimeSaturated;
            after[11] = g_MirrorViewEnabled;
            after[12] = g_RivalCueEnabled;
            after[13] = g_SeriesCleared;
            after[14] = g_SectorTimes[0];
            after[15] = g_SectorTimes[1];
            after[16] = g_SectorTimes[2];
            after[17] = g_RefSectorTime1;
            after[18] = g_RefSectorTime2;
            after[19] = g_RefSectorTimes.fields.first;
            Record("state", after, 20);
            Record("laptimes", g_PlayerCar.lapTimes.words, 12);
            Record("bestlap", &g_BestLapTimes[0][0][0], 2 * 4 * 2);
            Record("besttotal", &g_BestTotalTimes[0][0][0], 2 * 4 * 2);
            Record("bestsector", &g_BestSectorTimes[0][0][0], 2 * 4 * 3);
        }
        steps++;
    }

    /*
     * Two limits the sweep above cannot land on by multiplying out: a lap
     * whose frame count reaches 0xFFFF, and one whose converted time reaches
     * 0x927BE. Both are one step wide, so they get a narrow pass with the
     * exact frame counts and the exact jitter that put a lap on either side.
     */
    {
        static const s32 frameCounts[] = {0xFFFD, 0xFFFE, 29997, 29998};
        /* 40 and 41 are past the range the conversion keeps, so they pin
         * how wide that range is. */
        static const s32 jitters[] = {17, 18, 19, 40, 41};
        int fi, ji;

        for (fi = 0; fi < 4; fi++)
        for (ji = 0; ji < 5; ji++) {
            char label[96];

            memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
            g_TrackLength = 0x10000;
            g_RaceSeries = 1;
            g_CourseIndex = 2;
            g_LapCount = 3;
            g_RacePhase = 0;
            g_RaceFadeTimer = 0;
            g_RaceCueDelay = 0;
            g_RaceTotalTime = 0;
            g_BestLapThisRace = 0x7FFFFFFF;
            g_LapTimeMs = 0;
            g_LapTimeSaturated = 0;
            s_jitter = jitters[ji];
            g_PlayerCar.lap = 1;
            g_PlayerCar.lapTimes.table.frameCounts[0] =
                frameCounts[fi];
            /* Short of the next lap, so only the timing half runs. */
            g_PlayerCar.progressA = -1;

            sprintf(label, "== frames %d jitter %d", frameCounts[fi],
                    jitters[ji]);
            Record(label, NULL, 0);
            UpdateLapAndFinish(&g_PlayerCar, 0);
            RECORD("saturation",
                   g_PlayerCar.lapTimes.table.frameCounts[0],
                   g_PlayerCar.lapTimes.table.milliseconds[0],
                   g_LapTimeMs, g_LapTimeSaturated);
            steps++;
        }
    }

    if (s_out != NULL) {
        fclose(s_out);
    }
    if (s_digest != expected) {
        printf("FAIL laps and finishing behave differently: %d states making "
               "%d calls digest to %lu, expected %lu\n", steps, s_calls,
               s_digest, expected);
        return 1;
    }
    printf("laps and finishing take the same %d states they always did\n",
           steps);
    return 0;
}
