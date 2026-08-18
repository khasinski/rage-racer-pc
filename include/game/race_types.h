#ifndef GAME_RACE_TYPES_H
#define GAME_RACE_TYPES_H

#include "common.h"
#include "game/replay.h"

struct PlayerCarRuntime;
struct GameCarRuntime;
struct GameRenderSourcePoint;
struct CarEntry;

typedef union ReplayCarAddress {
    struct PlayerCarRuntime *player;
    struct GameCarRuntime *rivals;
    ReplayCarState *state;
    struct GameRenderSourcePoint *source;
    void *runtime;
} ReplayCarAddress;
/* Prize money per [course][class][place], place 0 = 1st. */
typedef struct GrandPrixPrizeTable {
    s32 values[4][6][3];
} GrandPrixPrizeTable;
typedef union RaceSeriesValue {
    s32 series;
    u16 trackDirection;
} RaceSeriesValue;

typedef union RaceSeriesAddress {
    volatile s32 *series;
    const s32 *stableSeries;
    RaceSeriesValue *value;
} RaceSeriesAddress;
typedef union RaceProgressMoney {
    s32 value;
    u16 half[2];
} RaceProgressMoney;

typedef struct GameRaceProgress {
    s32 course;
    s32 carIndex;
    s32 classIndex;
    s32 maxClassReached; /* highest class unlocked in this slot */
    RaceProgressMoney money; /* GP and Extra GP: prize money, capped at 999999999,
                       confirmed against US and JP saves advertising that cap.
                       The Time Attack slot reuses the word for
                       g_GrandPrixSeries, read back as u16. */
} GameRaceProgress;

#ifdef __psyz
_Static_assert(sizeof(GameRaceProgress) == 0x14,
               "race progress must retain its retail size");
_Static_assert(__builtin_offsetof(GameRaceProgress, maxClassReached) == 0x0C,
               "race progress max class must retain its retail offset");
_Static_assert(__builtin_offsetof(GameRaceProgress, money) == 0x10,
               "race progress money/series must retain its retail offset");
#endif
typedef enum AttractDemoStep {
    ATTRACT_DEMO_STEP_INVALID = -1,
    ATTRACT_DEMO_STEP_LOAD,
    ATTRACT_DEMO_STEP_RACE
} AttractDemoStep;
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
typedef struct PrologueLine {
    s16 x;
    s16 y;
    const u8 *text;
} PrologueLine;
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
typedef struct ReverbZone {
    s32 start;
    s32 end;
} ReverbZone;

#endif
