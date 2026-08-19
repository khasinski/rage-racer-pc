#ifndef GAME_OPTION_CONTROLLER_H
#define GAME_OPTION_CONTROLLER_H

#include "common.h"

typedef enum OptionDecision {
    OPTION_DECISION_NONE,
    OPTION_DECISION_ACCEPT,
    OPTION_DECISION_CANCEL
} OptionDecision;

typedef struct ScreenAdjustState {
    s32 x;
    s32 y;
} ScreenAdjustState;

typedef struct ScreenAdjustResult {
    OptionDecision decision;
    u8 moved;
} ScreenAdjustResult;

ScreenAdjustResult ScreenAdjustReduce(
    ScreenAdjustState *state, u16 pressed, u16 pressedRepeat);

typedef struct ClassRecordBrowseState {
    s32 column;
    s32 row;
} ClassRecordBrowseState;

typedef struct ClassRecordBrowseResult {
    u8 moved;
    u8 close;
} ClassRecordBrowseResult;

ClassRecordBrowseResult ClassRecordBrowseReduce(
    ClassRecordBrowseState *state, u16 pressed);

typedef struct NegconCalibrationState {
    s32 value;
    s32 nextMode;
    u8 restoreSettings;
} NegconCalibrationState;

typedef struct NegconCalibrationInput {
    u16 pressed;
    u8 connected;
    s32 confirmMode;
} NegconCalibrationInput;

typedef struct NegconCalibrationResult {
    u8 moved;
    OptionDecision decision;
} NegconCalibrationResult;

NegconCalibrationResult NegconCalibrationReduce(
    NegconCalibrationState *state, const NegconCalibrationInput *input);

#endif
