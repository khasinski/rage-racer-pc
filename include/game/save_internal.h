#ifndef GAME_SAVE_INTERNAL_H
#define GAME_SAVE_INTERNAL_H

#include "common.h"
#include "game/car.h"
#include "game/memcard.h"
#include "game/menu_types.h"
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
extern s32 g_BestLapTimes[2][4][2];
extern s32 g_BestTotalTimes[2][4][2];
extern s32 g_BestSectorTimes[2][4][3];

void BuildSaveIconBlock(GameSaveIconBlock *block, const char *title);
void WriteSaveHeaderRow(GameSaveHeaderRow *row);
s32 LoadSaveStateBlock(const GameSaveBlock *block);
void StoreSaveStateBlock(GameSaveBlock *block);

u32 CalculateSaveBlockChecksum(const GameSaveBlock *block);
u32 CalculateSaveHeaderChecksum(const GameSaveHeaderRow *header);

#endif
