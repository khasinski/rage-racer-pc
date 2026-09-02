#include "game/screens.h"
#include "game/race.h"
#include "game/render.h"
#include "game/save_internal.h"
#include "game/records_internal.h"

static const RaceRecord s_DefaultRecords[RECORD_TABLE_LENGTH] = {
    {{'R', 'A', 'G', 'E', ' ', ' ', '\0', '\0'}, 0, 0, 0},
    {{'R', 'A', 'C', 'E', 'R', ' ', '\0', '\0'}, 0, 3, 0},
    {{'N', 'A', 'M', 'C', 'O', ' ', '\0', '\0'}, 0, 4, 0},
    {{'R', 'I', 'D', 'G', 'E', ' ', '\0', '\0'}, 0, 7, 0},
    {{'R', 'A', 'C', 'E', 'R', ' ', '\0', '\0'}, 0, 3, 0},
};

FastestLap FindFastestLap(const s32 *lapTimes, s32 lapCount) {
    FastestLap fastest = {-1, 0};
    s32 lap;

    if (lapCount <= 0) {
        return fastest;
    }

    fastest.index = 0;
    fastest.time = lapTimes[0];
    for (lap = 1; lap < lapCount; lap++) {
        if (lapTimes[lap] < fastest.time) {
            fastest.index = lap;
            fastest.time = lapTimes[lap];
        }
    }
    return fastest;
}

s32 InsertRaceRecord(RaceRecord records[RECORD_TABLE_LENGTH], s32 raceTime,
                     s16 carIndex, u8 nameCodes[RECORD_NAME_LENGTH]) {
    s32 row;
    s32 shift;
    s32 character;

    for (row = 0; row < RECORD_TABLE_LENGTH; row++) {
        if (raceTime >= records[row].raceTime) continue;
        for (shift = RECORD_TABLE_LENGTH - 1; shift > row; shift--) {
            records[shift] = records[shift - 1];
        }
        records[row] = (RaceRecord){{0}, raceTime, carIndex, 0};
        for (character = 0; character < RECORD_NAME_LENGTH; character++) {
            records[row].driverName[character] = 'A';
            nameCodes[character] = 0xB;
        }
        return row;
    }
    return RECORD_TABLE_LENGTH;
}

void InitRecordTables(void) {
    s32 series;
    s32 course;
    s32 slot;
    const s32 *defaultLapTimes = g_DefaultLapTimes;
    const s32 *defaultTotalTimes = g_DefaultTotalTimes;

    for (series = 0; series < 2; series++) {
        for (course = 0; course < 4; course++) {
            for (slot = 0; slot < 2; slot++) {
                g_BestLapTimes[series][course][slot] = defaultLapTimes[series * 4 + course];
                g_BestTotalTimes[series][course][slot] = defaultTotalTimes[series * 4 + course];
            }
            for (slot = 0; slot < 3; slot++) {
                /* Retail seeds all three sector references from the course's
                 * default lap time; memory-card data may replace them later. */
                g_BestSectorTimes[series][course][slot] =
                    defaultLapTimes[series * 4 + course];
            }
            for (slot = 0; slot < RECORD_TABLE_LENGTH; slot++) {
                g_RankingRecords[series][course][slot] =
                    s_DefaultRecords[slot];
                g_RankingRecords[series][course][slot].raceTime =
                    defaultLapTimes[series * 4 + course] + slot * 2000;
                g_TimeRecords[series][course][slot] = s_DefaultRecords[slot];
                g_TimeRecords[series][course][slot].raceTime =
                    defaultTotalTimes[series * 4 + course] + slot * 10000;
            }
        }
    }
}

void RepairRecordTimes(void) {
    s32 series;
    s32 course;
    s32 slot;
    const s32 *defaultLapTimes = g_DefaultLapTimes;
    const s32 *defaultTotalTimes = g_DefaultTotalTimes;

    for (series = 0; series < 2; series++) {
        for (course = 0; course < 4; course++) {
            s32 index = series * 4 + course;

            for (slot = 0; slot < 2; slot++) {
                if (g_BestLapTimes[series][course][slot] <= 0) {
                    g_BestLapTimes[series][course][slot] =
                        defaultLapTimes[index];
                }
                if (g_BestTotalTimes[series][course][slot] <= 0) {
                    g_BestTotalTimes[series][course][slot] =
                        defaultTotalTimes[index];
                }
            }
            for (slot = 0; slot < 3; slot++) {
                if (g_BestSectorTimes[series][course][slot] <= 0) {
                    g_BestSectorTimes[series][course][slot] =
                        defaultLapTimes[index];
                }
            }
        }
    }
}

void *FormatLapTime(void *dst, s32 value) {
    s32 minutes = value / 60000;
    s32 ticks = value / 1000;
    s32 seconds = ticks - (minutes * 60);
    s32 fraction = value - (ticks * 1000);

    sprintf(dst, g_FmtLapTime, minutes, seconds, fraction);
    return dst;
}

void DrawCourseIntro(void) {
    DrawProportionalText(0x10, 0x1C, g_TextTimeAttack, 0x7812);
    DrawText8x8Trans(0x10, 0x39, g_TextCourseIn, 0x78CC);
    DrawResultScreen();
}
