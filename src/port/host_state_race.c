/*
 * Retail state a race in progress reads: the clock, the lap and position
 * bookkeeping, the standings, the tachometer face, the replay and the
 * in-race music selector.
 *
 * The line between this and the front end is which code reads it, not which
 * screen shows it: the results screen totals are the front end's because the
 * front end computes them, while the times they are computed from are here.
 * Order is retail's address order.
 */

#include <stddef.h>

#include "common.h"
#include "game/menu_types.h"
#include "game/race_hud_internal.h"
#include "game/race_time_types.h"
#include "game/replay_internal.h"
#include "game/save_types.h"

s32 g_PromotionBonusTable[5] = {
    500, 4800, 20000, 100000, 500000
};
StartCountdownPattern
    g_CountdownGlyphTable[START_COUNTDOWN_GLYPH_PATTERN_COUNT] = {
    {268435456, 403570816, 406790336, 1004937344,
     805316856, 872820732, 1728013888, 3424632896,
     2550677568, 805847104, 2013806784, 1208772800,
     2282516608, 203174016, 209462016, 134742016},
    {0, 4229953599, 4232051775, 4232051712,
     4231543871, 15423, 4228381696, 4228380735,
     4228380735, 4227873792, 15415, 4231543857,
     4232051767, 4232051761, 4229953591, 0},
    {0, 4229953599, 4232051775, 4232051775,
     4231543808, 4227923007, 4227987519, 520255,
     4228890687, 4229922816, 4231921719, 4231790641,
     4232051767, 4232051764, 4232051767, 0},
    {0, 4228112447, 4228374591, 4229947455,
     4229947455, 4228112447, 4228112447, 4228112447,
     4228112447, 4228112384, 4228112434, 4228112438,
     4229954610, 4229954610, 4229954610, 0},
};
s32 g_CameraCarIndex;
CourseProgressState *g_CourseProgress;
s32 g_BestTotalTimes[2][4][2];
s32 g_FrameSyncThreshold;
s16 g_ReverbZoneDepth;
s32 g_CdTrackEnded;
s32 g_ClassResultPlace;
s32 g_SeriesCleared;
ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];
s16 g_TrackZoneCode;
s32 g_BgmTrackCount;
unsigned char g_RankingNameCodes[8];
s32 g_ClassPromoted;
/* [series][course][sector], 2 * 4 * 3 signed 32-bit times.  The following
 * retail labels are interior aliases, not the bounds of this object. */
s32 g_BestSectorTimes[2][4][3];
s32 g_FadeStep;
s32 g_FadeLevel;
s32 g_LapCount;
s32 g_BestLapTimes[2][4][2];
s32 g_FrameParity;
s32 g_ClassCompleted;
s32 g_RaceTotalTime;
s32 g_RacePaused;
s32 g_BestLapThisRace;
s32 g_LapTimeMs;
s32 g_SectorEndDistance[3];
s32 g_ClassWinCount;
s16 g_GrandPrixMode;
s16 g_RaceCueDelay;
s16 g_RacePhase;
s16 g_RivalCueCooldowns[4];
unsigned char g_BgmShuffleOrder[12];
s16 g_WrongWayTimer;
Replay g_Replay;
unsigned char g_TimeRecordNameCodes[8];
