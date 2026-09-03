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

char g_TextResult[8] __attribute__((aligned(16))) = "RESULT";
char g_FmtClassGrandPrix[24] __attribute__((aligned(16))) = "CLASS%d %s GRANDPRIX";
char g_FmtRoundIn[12] __attribute__((aligned(16))) = "ROUND%d IN";
char g_CaptionRanking[8] __attribute__((aligned(16))) = "hai";
char g_CaptionTotalTime[8] __attribute__((aligned(16))) = "hegi";
char g_CaptionLapTime[8] __attribute__((aligned(16))) = "hfgi";
char g_CaptionPrizeMoney[8] __attribute__((aligned(16))) = "hci";
char g_FmtMoney[8] __attribute__((aligned(16))) = "%dv";
char g_CaptionTotalMoney[8] __attribute__((aligned(16))) = "hebi";
char g_CaptionPromotionBonus[] __attribute__((aligned(16))) = "hji";
char g_CaptionLostRace[24] __attribute__((aligned(16))) = "h L O S T  R A C E i";
char g_TextTryAgain[12] __attribute__((aligned(16))) = "TRY AGAIN";
char g_TextEndRace[12] __attribute__((aligned(16))) = "END RACE";
char g_TextChance[8] __attribute__((aligned(16))) = "CHANCE";
char g_TextPressStart[20] __attribute__((aligned(16))) = "PRESS START BUTTON";
char g_FmtLapTime[16] __attribute__((aligned(16))) = "%1d'%02d\"%03d";
char g_TextTimeAttack[12] __attribute__((aligned(16))) = "TIME ATTACK";
char g_TextCourseIn[] __attribute__((aligned(16))) = "COURSE IN";
char g_CaptionLapTime2[8] __attribute__((aligned(16))) = "hfgi";
char g_CaptionRanking2[8] __attribute__((aligned(16))) = "hai";
char g_FmtRecordName[8] __attribute__((aligned(16))) = "/%s/%s";
char g_FmtCarName[8] __attribute__((aligned(16))) = "/%s";
char g_CaptionTotalTime2[8] __attribute__((aligned(16))) = "hegi";
unsigned char g_NameEntryCharset[96] __attribute__((aligned(16))) = {0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x20,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x2e,0x2d,0x21,0x3f,0x40,0x00,0x00,0xe0,0x27,0x02,0x80,0x98,0x28,0x02,0x80,0x38,0x2b,0x02,0x80,0x70,0x2b,0x02,0x80,0x0c,0x2c,0x02,0x80,0x00,0x2e,0x02,0x80,0x74,0x2e,0x02,0x80,0x4c,0x3e,0x02,0x80,0x78,0x3e,0x02,0x80,0x94,0x3e,0x02,0x80,0xb0,0x3e,0x02,0x80,0x88,0x3f,0x02,0x80,0xcc,0x3f,0x02,0x80};
char g_TextNowLoading[] __attribute__((aligned(16))) = "NOW LOADING";
char g_MsgGameExit[12] __attribute__((aligned(16))) = "game_exit\n";
char g_MsgGame0Ok[12] __attribute__((aligned(16))) = "game0 ok\n";
s32 g_PromotionBonusTable[5] __attribute__((aligned(16))) = {
    500, 4800, 20000, 100000, 500000
};
unsigned char g_ResultPlaceSprites[10] __attribute__((aligned(16))) = {0x24,0x30,0x00,0x1a,0x40,0x30,0x1c,0x38,0x70,0x00};
u16 g_ResultPlaceCluts[4] __attribute__((aligned(16))) = {
    0, 30739, 30926, 30925
};
u16 g_ResultPanelCluts[5] __attribute__((aligned(16))) = {
    0, 30803, 30795, 30859, 0
};
unsigned char g_ClassPlaceBarSizes[8] __attribute__((aligned(16))) = {0xb8,0x18,0xb0,0x1c,0xa8,0x24,0x00,0x00};
char g_ChanceDigits[6][2] __attribute__((aligned(16))) = {
    {'0', '\0'}, {'1', '\0'}, {'2', '\0'},
    {'3', '\0'}, {'4', '\0'}, {'5', '\0'},
};
unsigned char g_PlaceSuffixNames[20] __attribute__((aligned(16))) = {0xec,0x0e,0x01,0x80,0xe8,0x0e,0x01,0x80,0xe4,0x0e,0x01,0x80,0xe0,0x0e,0x01,0x80,0xdc,0x0e,0x01,0x80};
unsigned char g_CarNames[52] __attribute__((aligned(16))) = {0x68,0x0f,0x01,0x80,0x60,0x0f,0x01,0x80,0x58,0x0f,0x01,0x80,0x4c,0x0f,0x01,0x80,0x44,0x0f,0x01,0x80,0x3c,0x0f,0x01,0x80,0x34,0x0f,0x01,0x80,0x28,0x0f,0x01,0x80,0x20,0x0f,0x01,0x80,0x14,0x0f,0x01,0x80,0x08,0x0f,0x01,0x80,0xfc,0x0e,0x01,0x80,0xf0,0x0e,0x01,0x80};
unsigned char g_CarClassNames[52] __attribute__((aligned(16))) = {0x8c,0x0f,0x01,0x80,0x8c,0x0f,0x01,0x80,0x8c,0x0f,0x01,0x80,0x84,0x0f,0x01,0x80,0x7c,0x0f,0x01,0x80,0x7c,0x0f,0x01,0x80,0x7c,0x0f,0x01,0x80,0x70,0x0f,0x01,0x80,0x70,0x0f,0x01,0x80,0x70,0x0f,0x01,0x80,0x8c,0x0f,0x01,0x80,0x7c,0x0f,0x01,0x80,0x70,0x0f,0x01,0x80};
s32 g_BgmRandomLabelTimer;
s32 g_BgmRandomPlay;
unsigned char g_BgmSelectSteps[20] __attribute__((aligned(16))) = {0xd8,0x5b,0x02,0x80,0x20,0x5c,0x02,0x80,0xd8,0x5e,0x02,0x80,0x84,0x64,0x02,0x80,0x00,0x00,0x00,0x00};
s16 g_AttractTitleDelays[4] __attribute__((aligned(16))) = {
    15, 256, 0, 0
};
u32 g_CountdownGlyphTable[64] __attribute__((aligned(16))) = {
    268435456, 403570816, 406790336, 1004937344, 805316856, 872820732,
    1728013888, 3424632896, 2550677568, 805847104, 2013806784, 1208772800,
    2282516608, 203174016, 209462016, 134742016, 0, 4229953599, 4232051775,
    4232051712, 4231543871, 15423, 4228381696, 4228380735, 4228380735,
    4227873792, 15415, 4231543857, 4232051767, 4232051761, 4229953591, 0,
    0, 4229953599, 4232051775, 4232051775, 4231543808, 4227923007,
    4227987519, 520255, 4228890687, 4229922816, 4231921719, 4231790641,
    4232051767, 4232051764, 4232051767, 0, 0, 4228112447, 4228374591,
    4229947455, 4229947455, 4228112447, 4228112447, 4228112447, 4228112447,
    4228112384, 4228112434, 4228112438, 4229954610, 4229954610, 4229954610,
    0
};
u8 g_TachoFaceR = 128;
u8 g_TachoFaceG = 128;
u8 g_TachoFaceB = 128;
s32 g_CountdownBoardOffset;
s32 g_RaceOptionPulseAngle;
s16 g_RaceOptionScroll0 = -240;
s16 g_RaceOptionScroll1 = 240;
char g_RaceOptionMarquee[4][40] __attribute__((aligned(16))) = {
    "  RAGE RACER GE\0\0\0\0\0TS YOU GOING!  \0\0\0\0\0",
    "  RAGE RACER GE\0\0\0\0\0TS YOU GOING!  \0\0\0\0\0",
    "   KICK BACK AN\0\0\0\0\0D CHILL OUT!   \0\0\0\0\0",
    "     SLASH THOS\0\0\0\0\0E RECORDS!     ",
};
s32 g_LastSectorTime;
s32 g_SplitDelta;
s32 g_SectorTimes[3] __attribute__((aligned(16))) = {
    0, 0, 0
};
s32 g_RefLapTime;
/* SectorReferenceTimes storage; kept untyped here because this translation
 * unit reproduces the retail state layout without importing subsystem headers. */
unsigned char g_RefSectorTimes[12] __attribute__((aligned(16)));
s32 g_RaceTimeRemaining;
s16 g_SplitSector;
s16 g_SplitTimer;
s16 g_SplitSign;
s32 g_SplitTargetTime;
s32 g_CameraCarIndex;
unsigned char g_CourseProgress[8] __attribute__((aligned(16)));
s32 g_CameraViewMode;
s32 g_ReplayBufferWrapped;
s32 g_BestTotalTimes[2][4][2];
s16 g_PauseDebounce;
s32 g_FrameSyncThreshold;
s16 g_ReverbZoneDepth;
s32 g_CdTrackEnded;
s32 g_ClassResultPlace;
s32 g_SeriesCleared;
s32 g_NameEntryCursor;
s16 g_MirrorViewEnabled;
s32 g_RecordPanelSlide;
s32 g_BestLapIndex;
s32 g_BgmChangeDelay;
unsigned char g_ClassRecords[44] __attribute__((aligned(16)));
s32 g_ReplayFrameCount;
s32 g_BgmSelectCdTrack;
s32 g_TimeRecordInsertRow;
s16 g_TrackZoneCode;
s32 g_LostRaceChoice;
s16 g_CameraCarTrackSection;
s32 g_BgmTrackCount;
s32 g_BgmSelectShowUi;
s32 g_SectorIndex;
s16 g_RaceOptionCursor;
s32 g_PrologueStep;
unsigned char g_RankingNameCodes[8] __attribute__((aligned(16)));
s32 g_ClassPromoted;
/* [series][course][sector], 2 * 4 * 3 signed 32-bit times.  The following
 * retail labels are interior aliases, not the bounds of this object. */
s32 g_BestSectorTimes[2][4][3];
s32 g_FadeStep;
s32 g_FadeLevel;
s32 g_LapCount;
s16 g_RaceFadeTimer;
s32 g_BgmSelectTrack;
s32 g_BestLapTimes[2][4][2];
s32 g_FrameParity;
s32 g_BgmSelectCursor;
s32 g_ClassCompleted;
s32 g_RaceTotalTime;
s32 g_RacePaused;
s32 g_ReplayWriteCursor;
s16 g_ReplayRivalModelIndex;
s32 g_BestLapThisRace;
s32 g_ClassClearFanfareTimer;
s32 g_LapTimeMs;
s16 g_ReplayPlayerModelIndex;
s32 g_SectorEndDistance[3];
s32 g_ClassWinCount;
s16 g_GrandPrixMode;
s32 g_PrologueCutIndex;
s32 g_AttractDemoStep;
s32 g_NameEntryChar;
s32 g_BonusCountStep;
s32 g_RecordEntryState;
s16 g_RaceCueDelay;
s32 g_PrizeCountStep;
s16 g_RacePhase;
s32 g_EndingWashLevel;
s16 g_RivalCueCooldowns[4];
unsigned char g_BgmShuffleOrder[12] __attribute__((aligned(16)));
s32 g_RankingInsertRow;
s16 g_WrongWayTimer;
s32 g_ReplayReadCursor;
unsigned char g_TimeRecordNameCodes[8] __attribute__((aligned(16)));
