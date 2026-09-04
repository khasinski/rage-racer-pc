#ifndef GAME_SAVE_INTERNAL_H
#define GAME_SAVE_INTERNAL_H

#include "common.h"
#include "game/car.h"
#include "game/memcard.h"
#include "game/menu_types.h"
#include "game/records_internal.h"
#include "game/save_types.h"
#include "game/team_logo.h"
#include "psyq/gpu.h"

extern CarEntry g_SaveDefaults[GAME_CAR_COUNT];
extern ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];
extern s32 g_ClassWinCount;
extern CourseProgressState g_GrandPrixCourseProgress;
extern CourseProgressState g_ExtraGrandPrixCourseProgress;
extern CourseProgressState *g_CourseProgress;
extern s32 g_BgmSelection;
extern TeamLogoCanvas g_TeamLogoCanvas;
extern s32 g_BestLapTimes[RECORD_SERIES_COUNT][RECORD_COURSE_COUNT]
                             [RECORD_REFERENCE_COUNT];
extern s32 g_BestTotalTimes[RECORD_SERIES_COUNT][RECORD_COURSE_COUNT]
                               [RECORD_REFERENCE_COUNT];
extern s32 g_BestSectorTimes[RECORD_SERIES_COUNT][RECORD_COURSE_COUNT]
                                [RECORD_SECTOR_COUNT];

void BuildSaveIconBlock(GameSaveIconBlock *block, const char *title);
void WriteSaveHeaderRow(GameSaveHeaderRow *row);
s32 LoadSaveStateBlock(const GameSaveBlock *block);
void StoreSaveStateBlock(GameSaveBlock *block);

u32 CalculateSaveBlockChecksum(const GameSaveBlock *block);
u32 CalculateSaveHeaderChecksum(const GameSaveHeaderRow *header);

#endif
