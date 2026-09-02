#ifndef GAME_RECORDS_INTERNAL_H
#define GAME_RECORDS_INTERNAL_H

#include "common.h"
#include "game/menu_types.h"

extern RaceRecord g_RankingRecords[2][4][5];
extern RaceRecord g_TimeRecords[2][4][5];

enum {
    RECORD_NAME_LENGTH = 6,
    RECORD_TABLE_LENGTH = 5,
};

typedef struct FastestLap {
    s32 index;
    s32 time;
} FastestLap;

FastestLap FindFastestLap(const s32 *lapTimes, s32 lapCount);
s32 InsertRaceRecord(RaceRecord records[RECORD_TABLE_LENGTH], s32 raceTime,
                     s16 carIndex, u8 nameCodes[RECORD_NAME_LENGTH]);
void WriteRecordDriverName(RaceRecord *record, const u8 *nameCodes);
void FormatRecordDriverClass(char *dst, s32 dstSize, const char *format,
                             const RaceRecord *record,
                             const char *className);
s32 UpdateRecordNameEntry(u8 *nameCodes);
void EnterRecordEntry(void);
void UpdateRecordEntry(void);
void DrawNameEntryCursor(s32 charIndex, s32 row);
void DrawRankingPanel(s32 slideX);
void DrawTimeRecordPanel(s32 slideX);

/* Restores the authored references for uninitialised record fields in an
 * otherwise valid memory-card save.  Early host builds could write zeroes
 * here; zero is never a valid completed lap, total, or sector time. */
void RepairRecordTimes(void);

#endif
