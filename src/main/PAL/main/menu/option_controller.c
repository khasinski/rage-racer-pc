#include "game/option_controller.h"
#include "game/pad.h"

ScreenAdjustResult ScreenAdjustReduce(
    ScreenAdjustState *state, u16 pressed, u16 pressedRepeat) {
    ScreenAdjustResult result = {OPTION_DECISION_NONE, 0};
    ScreenAdjustState previous = *state;

    if ((pressedRepeat & PAD_UP) != 0 && state->y >= -31) state->y--;
    if ((pressedRepeat & PAD_DOWN) != 0 && state->y < 23) state->y++;
    if ((pressedRepeat & PAD_LEFT) != 0 && state->x >= -10) state->x--;
    if ((pressedRepeat & PAD_RIGHT) != 0 && state->x < 32) state->x++;
    result.moved = state->x != previous.x || state->y != previous.y;
    if ((pressed & PAD_CONFIRM) != 0)
        result.decision = OPTION_DECISION_ACCEPT;
    else if ((pressed & PAD_CANCEL) != 0)
        result.decision = OPTION_DECISION_CANCEL;
    return result;
}

ClassRecordBrowseResult ClassRecordBrowseReduce(
    ClassRecordBrowseState *state, u16 pressed) {
    ClassRecordBrowseResult result = {0, 0};
    ClassRecordBrowseState previous = *state;

    if ((pressed & PAD_UP) != 0 && state->row == 1) state->row = 0;
    if ((pressed & PAD_DOWN) != 0 && state->row == 0) state->row = 1;
    if ((pressed & PAD_LEFT) != 0) state->column--;
    if ((pressed & PAD_RIGHT) != 0) state->column++;
    state->column = (state->column + 6) % 6;
    if (state->column == 5) state->row = 0;
    result.moved =
        state->column != previous.column || state->row != previous.row;
    result.close = (pressed & (PAD_CONFIRM | PAD_CANCEL)) != 0;
    return result;
}

NegconCalibrationResult NegconCalibrationReduce(
    NegconCalibrationState *state, const NegconCalibrationInput *input) {
    NegconCalibrationResult result = {0, OPTION_DECISION_NONE};

    if (!input->connected) {
        state->nextMode = 1;
        state->restoreSettings = 1;
        return result;
    }
    if ((input->pressed & PAD_CANCEL) != 0) {
        state->nextMode = 1;
        state->restoreSettings = 1;
        result.decision = OPTION_DECISION_CANCEL;
    } else if ((input->pressed & PAD_CONFIRM) != 0) {
        state->nextMode = input->confirmMode;
        result.decision = OPTION_DECISION_ACCEPT;
    }
    if ((input->pressed & PAD_LEFT) != 0 && state->value > 0) {
        state->value--;
        result.moved = 1;
    }
    if ((input->pressed & PAD_RIGHT) != 0 && state->value < 3) {
        state->value++;
        result.moved = 1;
    }
    return result;
}
