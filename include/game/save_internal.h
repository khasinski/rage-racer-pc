#ifndef GAME_SAVE_INTERNAL_H
#define GAME_SAVE_INTERNAL_H

#include "common.h"
#include "game/car.h"
#include "game/memcard.h"
#include "game/menu_types.h"
#include "game/team_logo.h"
#include "psyq/gpu.h"

extern CarEntry g_SaveDefaults[13];
extern ScoreRecord g_ClassRecords[11];
typedef struct CourseProgressState {
    u8 bestPlace[4];
    s16 unlockPending;
    s16 retriesRemaining;
} CourseProgressState;
extern s32 g_ClassWinCount;
extern CourseProgressState g_GrandPrixCourseProgress;
extern CourseProgressState g_ExtraGrandPrixCourseProgress;
extern CourseProgressState *g_CourseProgress;
extern s32 g_BgmSelection;
extern TeamLogoCanvas g_TeamLogoCanvas;
extern s32 g_BestLapTimes[2][4][2];
extern s32 g_BestTotalTimes[2][4][2];
extern s32 g_BestSectorTimes[2][4][3];

u32 CalculateSaveBlockChecksum(const GameSaveBlock *block);

#endif
