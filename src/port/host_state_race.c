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
#include "game/result_screen_types.h"
#include "game/save_types.h"

s32 g_PromotionBonusTable[5] = {
    500, 4800, 20000, 100000, 500000
};
ResultPlaceSpriteTable g_ResultPlaceSprites = {
    .places = {
        {0x24, 0x30, 0x00},
        {0x1a, 0x40, 0x30},
        {0x1c, 0x38, 0x70},
    },
};
u16 g_ResultPlaceCluts[4] = {
    0, 30739, 30926, 30925
};
ResultPanelClutTable g_ResultPanelCluts = {
    .byPlace = {0, 30803, 30795, 30859},
};
ResultPlaceBarTable g_ClassPlaceBarSizes = {
    .places = {
        {0xb8, 0x18},
        {0xb0, 0x1c},
        {0xa8, 0x24},
    },
};
s16 g_AttractTitleDelays[4] = {
    15, 256, 0, 0
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
u8 g_TachoFaceR = 128;
u8 g_TachoFaceG = 128;
u8 g_TachoFaceB = 128;
s32 g_LastSectorTime;
s32 g_SplitDelta;
s32 g_SectorTimes[3] = {
    0, 0, 0
};
s32 g_RefLapTime;
SectorReferenceTimes g_RefSectorTimes;
s32 g_RaceTimeRemaining;
s16 g_SplitSector;
s16 g_SplitTimer;
s16 g_SplitSign;
s32 g_SplitTargetTime;
s32 g_CameraCarIndex;
CourseProgressState *g_CourseProgress;
s32 g_BestTotalTimes[2][4][2];
s16 g_PauseDebounce;
s32 g_FrameSyncThreshold;
s16 g_ReverbZoneDepth;
s32 g_CdTrackEnded;
s32 g_ClassResultPlace;
s32 g_SeriesCleared;
ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];
s16 g_TrackZoneCode;
s32 g_BgmTrackCount;
s32 g_SectorIndex;
s16 g_RaceOptionCursor;
unsigned char g_RankingNameCodes[8];
s32 g_ClassPromoted;
/* [series][course][sector], 2 * 4 * 3 signed 32-bit times.  The following
 * retail labels are interior aliases, not the bounds of this object. */
s32 g_BestSectorTimes[2][4][3];
s32 g_FadeStep;
s32 g_FadeLevel;
s32 g_LapCount;
s16 g_RaceFadeTimer;
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
s32 g_EndingWashLevel;
s16 g_RivalCueCooldowns[4];
unsigned char g_BgmShuffleOrder[12];
s16 g_WrongWayTimer;
Replay g_Replay;
unsigned char g_TimeRecordNameCodes[8];
