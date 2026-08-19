#include "game/menu_dialog_controller.h"
#include "game/pad.h"

static s32 StepRange(s32 value, s32 minimum, s32 maximum, s32 step,
                     s32 wrap, u8 *moveCount) {
    s32 next = value + step;

    if (next < minimum) {
        if (!wrap) return value;
        next = maximum;
    } else if (next > maximum) {
        if (!wrap) return value;
        next = minimum;
    }
    if (next != value) {
        value = next;
        (*moveCount)++;
    }
    return value;
}

MenuDialogInputResult MenuDialogHandleBinary(s32 value, s32 leftValue,
                                             s32 rightValue, u16 pressed) {
    MenuDialogInputResult result;

    result.value = value;
    result.moveCount = 0;
    result.confirmed = (pressed & PAD_CONFIRM) != 0;
    result.cancelled = (pressed & PAD_CANCEL) != 0;
    if ((pressed & PAD_LEFT) != 0 && result.value != leftValue) {
        result.value = leftValue;
        result.moveCount++;
    }
    if ((pressed & PAD_RIGHT) != 0 && result.value != rightValue) {
        result.value = rightValue;
        result.moveCount++;
    }
    return result;
}

MenuDialogInputResult MenuDialogHandleRange(s32 value, s32 minimum,
                                            s32 maximum, s32 leftStep,
                                            s32 wrap, u16 buttons,
                                            u16 pressed) {
    MenuDialogInputResult result;

    result.value = value;
    result.moveCount = 0;
    result.confirmed = (pressed & PAD_CONFIRM) != 0;
    result.cancelled = (pressed & PAD_CANCEL) != 0;
    if ((buttons & PAD_LEFT) != 0) {
        result.value = StepRange(result.value, minimum, maximum, leftStep,
                                 wrap, &result.moveCount);
    }
    if ((buttons & PAD_RIGHT) != 0) {
        result.value = StepRange(result.value, minimum, maximum, -leftStep,
                                 wrap, &result.moveCount);
    }
    return result;
}
