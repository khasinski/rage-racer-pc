#ifndef GAME_RECORD_ENTRY_STATE_H
#define GAME_RECORD_ENTRY_STATE_H

#include "common.h"

typedef enum RecordEntryStep {
    RECORD_ENTRY_STATE_INVALID = -1,
    RECORD_ENTRY_STATE_FADE_IN,
    RECORD_ENTRY_STATE_EDIT_LAP_NAME,
    RECORD_ENTRY_STATE_WAIT_AFTER_LAP_NAME,
    RECORD_ENTRY_STATE_SWITCH_TO_RACE_RECORD,
    RECORD_ENTRY_STATE_EDIT_RACE_NAME,
    RECORD_ENTRY_STATE_WAIT_TO_FINISH,
    RECORD_ENTRY_STATE_FADE_OUT
} RecordEntryStep;

typedef struct RecordEntry {
    RecordEntryStep step;
    s32 nameCharacter;
    s32 nameCursor;
    s32 panelSlide;
    s32 bestLap;
    s32 rankingRow;
    s32 timeRow;
} RecordEntry;

#endif
