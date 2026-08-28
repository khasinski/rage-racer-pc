#ifndef GAME_RECORDS_INTERNAL_H
#define GAME_RECORDS_INTERNAL_H

#include "common.h"
#include "game/menu_types.h"

extern RaceRecord g_RankingRecords[2][4][5];
extern RaceRecord g_TimeRecords[2][4][5];

s32 InsertRaceRecord(RaceRecord records[5], s32 raceTime, s16 carIndex,
                     u8 nameCodes[6]);

/* Restores the authored references for uninitialised record fields in an
 * otherwise valid memory-card save.  Early host builds could write zeroes
 * here; zero is never a valid completed lap, total, or sector time. */
void RepairRecordTimes(void);

#endif
