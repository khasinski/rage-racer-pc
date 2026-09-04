#include <stdio.h>

#include "game/race.h"
#include "game/records_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"

enum {
    DEFAULT_RECORD_NAME_CODE = 0xB,
    DEFAULT_RECORD_NAME_CHARACTER = 'A',
};

static const RaceRecord s_DefaultRecords[RECORD_TABLE_LENGTH] = {
    {{'R', 'A', 'G', 'E', ' ', ' ', '\0', '\0'}, 0, 0, 0},
    {{'R', 'A', 'C', 'E', 'R', ' ', '\0', '\0'}, 0, 3, 0},
    {{'N', 'A', 'M', 'C', 'O', ' ', '\0', '\0'}, 0, 4, 0},
    {{'R', 'I', 'D', 'G', 'E', ' ', '\0', '\0'}, 0, 7, 0},
    {{'R', 'A', 'C', 'E', 'R', ' ', '\0', '\0'}, 0, 3, 0},
};

static s32 ClampRaceTime(int64_t time) {
    if (time < 0) {
        return 0;
    }
    if (time > RACE_TIME_MAX_MS) {
        return RACE_TIME_MAX_MS;
    }
    return (s32)time;
}

FastestLap FindFastestLap(const s32 *lapTimes, s32 lapCount) {
    FastestLap fastest = {
        .index = -1,
        .time = 0,
    };
    s32 lap;

    if (lapTimes == NULL || lapCount <= 0) {
        return fastest;
    }
    if (lapCount > COURSE_LONG_LAPS) {
        lapCount = COURSE_LONG_LAPS;
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

    if (records == NULL || nameCodes == NULL || raceTime < 0 ||
        raceTime > RACE_TIME_MAX_MS || (u32)carIndex >= GAME_CAR_COUNT) {
        return RECORD_TABLE_LENGTH;
    }

    for (row = 0; row < RECORD_TABLE_LENGTH; row++) {
        if (raceTime >= records[row].raceTime) continue;
        for (shift = RECORD_TABLE_LENGTH - 1; shift > row; shift--) {
            records[shift] = records[shift - 1];
        }
        records[row] = (RaceRecord){{0}, raceTime, carIndex, 0};
        for (character = 0; character < RECORD_NAME_LENGTH; character++) {
            records[row].driverName[character] =
                DEFAULT_RECORD_NAME_CHARACTER;
            nameCodes[character] = DEFAULT_RECORD_NAME_CODE;
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

    for (series = 0; series < RECORD_SERIES_COUNT; series++) {
        for (course = 0; course < RECORD_COURSE_COUNT; course++) {
            s32 index = series * RECORD_COURSE_COUNT + course;

            for (slot = 0; slot < RECORD_REFERENCE_COUNT; slot++) {
                g_BestLapTimes[series][course][slot] = defaultLapTimes[index];
                g_BestTotalTimes[series][course][slot] =
                    defaultTotalTimes[index];
            }
            for (slot = 0; slot < RECORD_SECTOR_COUNT; slot++) {
                /* Retail seeds all three sector references from the course's
                 * default lap time; memory-card data may replace them later. */
                g_BestSectorTimes[series][course][slot] =
                    defaultLapTimes[index];
            }
            for (slot = 0; slot < RECORD_TABLE_LENGTH; slot++) {
                g_RankingRecords[series][course][slot] =
                    s_DefaultRecords[slot];
                g_RankingRecords[series][course][slot].raceTime =
                    ClampRaceTime((int64_t)defaultLapTimes[index] +
                                  slot * 2000);
                g_TimeRecords[series][course][slot] = s_DefaultRecords[slot];
                g_TimeRecords[series][course][slot].raceTime =
                    ClampRaceTime((int64_t)defaultTotalTimes[index] +
                                  slot * 10000);
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

    for (series = 0; series < RECORD_SERIES_COUNT; series++) {
        for (course = 0; course < RECORD_COURSE_COUNT; course++) {
            s32 index = series * RECORD_COURSE_COUNT + course;

            for (slot = 0; slot < RECORD_REFERENCE_COUNT; slot++) {
                if (g_BestLapTimes[series][course][slot] <= 0 ||
                    g_BestLapTimes[series][course][slot] >
                        RACE_TIME_MAX_MS) {
                    g_BestLapTimes[series][course][slot] =
                        ClampRaceTime(defaultLapTimes[index]);
                }
                if (g_BestTotalTimes[series][course][slot] <= 0 ||
                    g_BestTotalTimes[series][course][slot] >
                        RACE_TIME_MAX_MS) {
                    g_BestTotalTimes[series][course][slot] =
                        ClampRaceTime(defaultTotalTimes[index]);
                }
            }
            for (slot = 0; slot < RECORD_SECTOR_COUNT; slot++) {
                if (g_BestSectorTimes[series][course][slot] <= 0 ||
                    g_BestSectorTimes[series][course][slot] >
                        RACE_TIME_MAX_MS) {
                    g_BestSectorTimes[series][course][slot] =
                        ClampRaceTime(defaultLapTimes[index]);
                }
            }
        }
    }
}

void FormatLapTime(char dst[LAP_TIME_TEXT_CAPACITY], s32 value) {
    s32 minutes;
    s32 seconds;
    s32 fraction;

    if (dst == NULL) {
        return;
    }
    value = ClampRaceTime(value);
    minutes = value / 60000;
    seconds = value / 1000 % 60;
    fraction = value % 1000;

    snprintf(dst, LAP_TIME_TEXT_CAPACITY, g_FmtLapTime, minutes, seconds,
             fraction);
}
