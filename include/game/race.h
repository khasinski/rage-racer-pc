#ifndef GAME_RACE_H
#define GAME_RACE_H

#include "common.h"
#include "game/prize_money.h"
#include "game/course_index.h"
#include "game/vector.h"
#include "game/replay.h"
#include "game/render_types.h"

struct PlayerCarRuntime;
struct GameCarRuntime;
struct GameRenderSourcePoint;
struct CarEntry;

enum { GRAND_PRIX_FINAL_CLASS_INDEX = 5 };

/* Grand Prix class index, 0-based; displayed as CLASS(n+1). Also the track
 * tier: course asset index = 0x57 + (course << 1) + (class << 3). OVAL is
 * gated to class >= 2. */
extern s32 g_GrandPrixClass;

/* Physical course asset selector. Extra GP uses indices 4..7 for its track
 * variants, while progress/record tables have four slots per series. Never
 * index a four-course table with this value directly. */

/* Which Grand Prix series is played: 0 = first (6 classes), non-zero =
 * Extra GP (5 classes). Outer index of every per-series table. */
extern s16 g_GrandPrixSeries;

/* Display names: [0..5] first-series classes, [6..10] Extra-GP classes,
 * [11..13] course names. */
extern char *g_GrandPrixNames[];

/* Race position, 1 = leading; recomputed each frame from how many cars are
 * further along. At the finish it indexes g_PrizeMoney. */

/* Round number within the current class; drives the "R O U N D %d" overlay. */
extern s32 g_GrandPrixRound;

/* 1 = Grand Prix (championship), 0 = Time Attack. Picks the pre-race panel, the
 * innermost index of the record tables, and the in-race option count
 * (2 - mode). */
extern s16 g_GrandPrixMode;

/* In-race copy of g_GrandPrixSeries, latched when the grid is built. Outer
 * index of the per-series tables and, because the Extra GP runs the
 * courses backwards, also the lap-direction flag. */
extern s32 g_RaceSeries;

static inline u16 ReadRaceTrackDirection(void) {
    return (u16)g_RaceSeries;
}

static inline s32 ReadStableRaceSeries(void) {
    return g_RaceSeries;
}

/* Race phase: 0 pre-start (physics frozen), 1 countdown, 2 racing, 4/5
 * finished, 7 goal/retire, 8 aborted. */
extern s16 g_RacePhase;

/* Series / save file the title menu picked (0 first, 1 Extra GP); also indexes
 * g_MaxClassReached. Final class is 4 for the Grand Prix, 5 for Extra GP. */
extern s16 g_SeriesSelection;

/* Non-zero once the Extra GP is unlocked (Grand Prix' last class
 * cleared). Saved at save+0x4E; gates title-menu entry 1. */
extern s16 g_ExtraGrandPrixUnlocked;

/* Highest class reached per series/save file. Unlocks courses and bounds the
 * attract-demo class roll. Saved at save+0x50. */
extern s32 g_MaxClassReached[2];

/* Mirror mode, armed by holding the 0x80C pad combination as the race starts:
 * swaps left/right in steering, body roll, stereo pan and the sound cue. */
extern s32 g_MirrorMode;

/* One save slot's Grand Prix / Time Attack progress; InitMenuMode copies it
 * straight into the live globals and UpdateCourseSelectScreen writes it back. */
typedef struct GameRaceProgress {
    s32 course;
    s32 carIndex;
    s32 classIndex;
    s32 maxClassReached; /* highest class unlocked in this slot */
    s32 money; /* GP and Extra GP: prize money, capped at 999999999,
                       confirmed against US and JP saves advertising that cap.
                       The Time Attack slot reuses the word for
                       g_GrandPrixSeries, read back as u16. */
} GameRaceProgress;

_Static_assert(sizeof(GameRaceProgress) == 0x14,
               "race progress must retain its retail size");
_Static_assert(__builtin_offsetof(GameRaceProgress, maxClassReached) == 0x0C,
               "race progress max class must retain its retail offset");
_Static_assert(__builtin_offsetof(GameRaceProgress, money) == 0x10,
               "race progress money/series must retain its retail offset");

/* The save slot the front end is editing; repointed at one of the three below,
 * matching the title-menu row that g_CarTable was repointed for. Declared s32
 * because most translation units only touch the first word. */
extern GameRaceProgress *g_RaceProgress;
/* The three slots themselves. Their fields used to be spelled as separate
 * symbols per serialiser (g_GrandPrixSaveCar and friends); they are members. */
extern GameRaceProgress g_GrandPrixSave;
extern GameRaceProgress g_ExtraGrandPrixSave;
extern GameRaceProgress g_TimeAttackSave;

/* Retail 0x801E6E88 is g_ExtraGrandPrixSave + 0x0C. */
#define g_ExtraGrandPrixSaveMaxClass (g_ExtraGrandPrixSave.maxClassReached)

void ResetProgressSlot(struct CarEntry *cars, GameRaceProgress *progress);

extern s32 g_ClosestRivalRank;

/* Course-select gate: `g_CourseIndex < (class < 2 ? 2 : 3)`, or 6 : 7 for the
 * Extra GP. This is the OVAL unlock. */

/* The race-start signal gantry, live for 105 <= g_SceneTimer < 300: the "3" /
 * "2" / "1" / "GO" dot-matrix board from g_CountdownGlyphTable[1..4] plus the six start
 * lamps. */
void DrawRaceEndBanner(s32 level);

/*
 * Per-course records, all in the memory-card save block. Per-file types.
 *   g_BestTotalTimes  g_BestTotalTimes  [series][course][mode] ms
 *   g_BestLapTimes    g_BestLapTimes  same shape, best single lap
 *   g_BestSectorTimes g_BestSectorTimes  [series][course][3] sector splits
 *   g_CourseProgress  g_CourseProgress  -> the running file's course-result record
 *   g_GrandPrixCourseProgress      g_GrandPrixCourseProgress  row 0's record
 *   g_ExtraGrandPrixCourseProgress g_ExtraGrandPrixCourseProgress  row 1's record
 */

/*
 * Live race timing. Times are milliseconds; 0x927BF is the saturation value
 * for anything over 9'59"998, which the HUD prints as dashes.
 */
enum { RACE_TIME_MAX_MS = 0x927BF };

/* Elapsed time of the lap in progress. */
extern s32 g_LapTimeMs;

/* Grand Prix time limit, in frames; counts down while g_RacePhase >= 2 and
 * forces g_RacePhase = 5 when it reaches 0. Seeded to 15000. */
extern s32 g_RaceTimeRemaining;

/* Sector being timed, 0..2; -2 before the first start-line crossing. */
extern s32 g_SectorIndex;

/* This lap's three sector times, filled in as each boundary is crossed. */
extern s32 g_SectorTimes[3];

/* Total of the best lap the split is measured against; seeded from the save
 * records and written back when the race completes. */
extern s32 g_RefLapTime;

/* Two 3-element arrays:
 *   g_SectorEndDistance[3] lap distance ending each sector (L/3, 2L/3, L)
 *   g_RefSectorTimes[3]    the best lap's sector times
 */

/* Split readout: the sector time just recorded, the unsigned difference from
 * the reference, and its sign (+1 ahead, -1 behind, 0 no split). */
extern s32 g_LastSectorTime;
extern s32 g_SplitDelta;
extern s16 g_SplitSign;

/* Which sector's reference is on screen, the reference time itself, and the
 * 0..0x3C frame counter that ends the split display. */
extern s16 g_SplitSector;
extern s32 g_SplitTargetTime;
extern s16 g_SplitTimer;

/* Frames the player has been driving the wrong way. Past 10 the warning shows
 * and rival cues are muted; in Time Attack 60 on lap 0 aborts the run. */
extern s16 g_WrongWayTimer;

/* g_PlayerCar.facingBackwards. Wrong way is `!= g_RaceSeries`, because the
 * Extra GP drives the course in the other direction. */

/* Non-zero while rival proximity / position sound cues may play: set only in
 * the middle of a lap and cleared by the wrong-way warning. */
extern s16 g_RivalCueEnabled;

/* Frame counter of the in-race fade transitions; every use is the brightness
 * argument of DrawFullscreenFadeTile plus a frame threshold. */
extern s16 g_RaceFadeTimer;

/* Cursor of the in-race option overlay, clamped to 2 - g_GrandPrixMode. */
extern s16 g_RaceOptionCursor;

/* Best lap of this race so far (g_BestLapThisRace), seeded from g_BestLapTimes at the
 * grid, and DrawTimeValue, which prints one millisecond
 * time as m'ss"fff. Both are also referenced from render/, so they are
 * declared per file rather than here. */

/* The wrong-way warning: three sprites over a backing panel, drawn once
 * g_WrongWayTimer passes 10. */
void DrawWrongWayWarning(void);

extern s16 g_PlayerAutoSteer;
typedef enum AttractDemoStep {
    ATTRACT_DEMO_STEP_INVALID = -1,
    ATTRACT_DEMO_STEP_LOAD,
    ATTRACT_DEMO_STEP_RACE
} AttractDemoStep;

extern AttractDemoStep g_AttractDemoStep;
extern s32 g_BestLapThisRace;
extern s32 g_BgmTrack;
extern s32 g_BonusCountStep;
enum { CLASS_CLEAR_FANFARE_DURATION_FRAMES = 210 };
extern s32 g_ClassClearFanfareTimer;
extern s32 g_ClassCompleted;
extern s32 g_ClassResultPlace;
extern s32 g_LapCount;
extern s16 g_PauseDebounce;
extern s32 g_PrizeAmount;
extern s32 g_PrizeCountStep;
typedef enum PrizeScreenState {
    PRIZE_SCREEN_STATE_INVALID = -1,
    PRIZE_SCREEN_STATE_INTRO_FADE_IN,
    PRIZE_SCREEN_STATE_WAIT_FOR_INTRO_CONFIRM,
    PRIZE_SCREEN_STATE_HIDE_RACE_TIME,
    PRIZE_SCREEN_STATE_SHOW_PRIZE_PANEL,
    PRIZE_SCREEN_STATE_COUNT_PRIZE,
    PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM,
    PRIZE_SCREEN_STATE_COUNT_BONUS,
    PRIZE_SCREEN_STATE_WAIT_TO_FINISH,
    PRIZE_SCREEN_STATE_FADE_OUT
} PrizeScreenState;

extern PrizeScreenState g_PrizeScreenState;
extern s32 g_PromotionBonus;
extern s32 g_RacePaused;
extern s32 g_ReplayReadCursor;
extern s16 g_ReverbZoneDepth;
extern s32 g_RivalCueFlags;
extern s32 g_SectorEndDistance[];
extern s32 g_SeriesCleared;
extern s16 g_TrackZoneCode;

/*
 * None of the Draw* functions below draw. Each one packs primitives at the
 * render-state cursor and links them into an ordering table; the GPU is not
 * touched until boot/main_loop.c calls DrawOTag once per frame. That holds
 * for the whole family, render.h's DrawSprite / DrawLine / DrawSolidRect
 * included, which is why they are not spelled Queue* - the queueing is the
 * convention, not the exception.
 */
void DrawLostRaceCaption(s32 level);
void DrawRoundScreen(void);
void RefreshClassWinState(void);
void UpdateZoneAmbience(s32 zone);

extern s16 g_AttractTitleDelays[];
extern s32 g_BestLapIndex;
extern s32 g_BgmRandomLabelTimer;
extern s32 g_BgmRandomPlay;
extern s16 g_CameraCarTrackSection;
extern char g_CaptionLapTime[];
extern char g_CaptionLapTime2[];
extern char g_CaptionLostRace[];
extern char g_CaptionPrizeMoney[];
extern char g_CaptionPromotionBonus[];
extern char g_CaptionRanking[];
extern char g_CaptionRanking2[];
extern char g_CaptionTotalMoney[];
extern char g_CaptionTotalTime[];
extern char g_CaptionTotalTime2[];
extern char *g_NativeCarClassNames[];
extern char *g_NativeCarNames[];
#define g_CarClassNames g_NativeCarClassNames
#define g_CarNames g_NativeCarNames
extern char g_ChanceDigits[6][2];
extern s32 g_ClassPromoted;
extern char g_ClockTextCells[8];
extern s32 g_CountdownBoardOffset;
extern u32 g_CountdownGlyphTable[64];
extern u32 g_CountdownDigitPatterns[16];
extern char *g_CourseNames[];
extern s32 g_DefaultLapTimes[8];
extern s32 g_DefaultTotalTimes[8];
extern s32 g_EndingWashLevel;
extern char g_FmtCarName[];
extern char g_FmtClassGrandPrix[];
extern char g_FmtLapTime[];
extern char g_FmtMoney[];
extern char g_FmtRecordName[];
extern char g_FmtRoundIn[];
extern s32 g_LostRaceChoice;
extern char g_MsgGame0Ok[];
extern s32 g_NameEntryChar;
extern u8 g_NameEntryCharset[];
extern s32 g_NameEntryCursor;
extern u8 *g_NativePlaceSuffixNames[];
#define g_PlaceSuffixNames g_NativePlaceSuffixNames
extern s32 g_PrologueCutIndex;
typedef struct PrologueLine {
    s16 x;
    s16 y;
    const char *text;
} PrologueLine;

extern PrologueLine g_PrologueLines[17];
extern s32 g_PrologueLineCount;
enum { PROMOTION_BONUS_COUNT = 5 };
extern s32 g_PromotionBonusTable[PROMOTION_BONUS_COUNT];
extern char g_RaceOptionMarquee[4][40];
extern s32 g_RaceOptionPulseAngle;
extern s16 g_RaceOptionScroll0;
extern s16 g_RaceOptionScroll1;
extern s32 g_RankingInsertRow;
extern u8 g_RankingNameCodes[];
typedef enum RecordEntryState {
    RECORD_ENTRY_STATE_INVALID = -1,
    RECORD_ENTRY_STATE_FADE_IN,
    RECORD_ENTRY_STATE_EDIT_LAP_NAME,
    RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME,
    RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD,
    RECORD_ENTRY_STATE_EDIT_RACE_NAME,
    RECORD_ENTRY_STATE_WAIT_TO_FINISH,
    RECORD_ENTRY_STATE_FADE_OUT
} RecordEntryState;

extern RecordEntryState g_RecordEntryState;
extern s32 g_RecordPanelSlide;
extern u16 g_ResultPanelCluts[];
extern u16 g_ResultPlaceCluts[];
extern u8 g_TachoFaceB;
extern u8 g_TachoFaceG;
extern u8 g_TachoFaceR;
extern char g_TextChance[];
extern char g_TextCourseIn[];
extern char g_TextEndRace[];
extern char g_TextPressStart[];
extern char g_TextResult[];
extern char g_TextTimeAttack[];
extern char g_TextTryAgain[];
extern RenderBufferAddress g_TileStripBuffers[2];
extern u8 g_TileStripStorage[];
extern s32 g_TimeRecordInsertRow;
extern u8 g_TimeRecordNameCodes[];
extern char g_TimeTextBuffer[];

s32 BeginMirrorPass(void);
void BuildRaceHudPrims(s32 grandPrixMode);
void EnterPrizeScreen(void);
void DrawLapTimes(void);
void DrawRaceHudLabels(s32 grandPrixMode);
void DrawRacePosition(void);
void DrawRaceTimePanel(s32 slideY);
void DrawRearViewMirror(s32 mode);
void DrawTimeRemaining(s32 timeMs);
void ResetMirrorState(void);
s32 UpdateLapAndFinish(struct PlayerCarRuntime *car, s32 grandPrixMode);
void ExitRaceScene(s32 sceneId);
void EnterAttractScene(void);
s32 GetTrackZoneBlend(s32 position);
void EnterBgmSelectScreen(void);

extern CVec g_CountdownCellColors[];
#endif
