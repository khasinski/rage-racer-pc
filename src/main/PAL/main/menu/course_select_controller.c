#include "game/course_select_controller.h"
#include "game/pad.h"

s32 CourseSelectFirstCourse(const CourseSelectRules *rules) {
    return (rules->grandPrixMode != 0 && rules->seriesSelection != 0) ? 4 : 0;
}

s32 CourseSelectLastCourse(const CourseSelectRules *rules) {
    if (rules->grandPrixMode != 0) {
        if (rules->seriesSelection != 0) {
            return (rules->grandPrixClass < 2) ? 6 : 7;
        }
        return (rules->grandPrixClass < 2) ? 2 : 3;
    }

    if (rules->extraGrandPrixUnlocked != 0) {
        return (rules->extraMaxClassReached < 2) ? 6 : 7;
    }
    return (rules->normalMaxClassReached < 2) ? 2 : 3;
}

s32 CourseSelectCanMove(const CourseSelectRules *rules,
                        s32 courseIndex, s32 direction) {
    if (direction < 0) return courseIndex > CourseSelectFirstCourse(rules);
    if (direction > 0) return courseIndex < CourseSelectLastCourse(rules);
    return 0;
}

CourseSelectInputResult CourseSelectHandleMainInput(s32 option,
                                                    s32 grandPrixMode,
                                                    u16 pressed) {
    CourseSelectInputResult result;

    result.option = option;
    result.moveCount = 0;
    result.command = COURSE_SELECT_COMMAND_NONE;

    /* Keep the retail ordering when opposite directions arrive together. */
    if ((pressed & PAD_UP) != 0) {
        result.option = (result.option > 0) ? result.option - 1 : 2;
        result.moveCount++;
    }
    if ((pressed & PAD_DOWN) != 0) {
        result.option = (result.option < 2) ? result.option + 1 : 0;
        result.moveCount++;
    }

    if ((pressed & PAD_CONFIRM) == 0) return result;

    switch (result.option) {
    case 0:
        result.command = COURSE_SELECT_COMMAND_OPEN_CAR_SELECT;
        break;
    case 1:
        result.command = grandPrixMode != 0
            ? COURSE_SELECT_COMMAND_OPEN_CLASS_SELECT
            : COURSE_SELECT_COMMAND_OPEN_RANKING;
        break;
    case 2:
        result.command = grandPrixMode != 0
            ? COURSE_SELECT_COMMAND_OPEN_SAVE_PROMPT
            : COURSE_SELECT_COMMAND_START_RACE;
        break;
    }
    return result;
}

static CourseSelectModalCommand ResolveModalCommand(u16 pressed) {
    CourseSelectModalCommand command = COURSE_SELECT_MODAL_NONE;

    if ((pressed & PAD_CONFIRM) != 0) command = COURSE_SELECT_MODAL_CONFIRM;
    /* Retail evaluates cancel second, so it wins if both are pressed. */
    if ((pressed & PAD_CANCEL) != 0) command = COURSE_SELECT_MODAL_CANCEL;
    return command;
}

CourseSelectModalResult CourseSelectHandleSavePrompt(s32 cursor, u16 pressed) {
    CourseSelectModalResult result;

    result.cursor = cursor;
    result.moveCount = 0;
    result.confirmed = (pressed & PAD_CONFIRM) != 0;
    result.cancelled = (pressed & PAD_CANCEL) != 0;
    result.command = ResolveModalCommand(pressed);
    if ((pressed & PAD_LEFT) != 0) {
        result.cursor = 1;
        result.moveCount++;
    }
    if ((pressed & PAD_RIGHT) != 0) {
        result.cursor = 0;
        result.moveCount++;
    }
    return result;
}

CourseSelectModalResult CourseSelectHandleClassPrompt(s32 cursor,
                                                      s32 maxClass,
                                                      u16 pressed) {
    CourseSelectModalResult result;

    result.cursor = cursor;
    result.moveCount = 0;
    result.confirmed = (pressed & PAD_CONFIRM) != 0;
    result.cancelled = (pressed & PAD_CANCEL) != 0;
    result.command = ResolveModalCommand(pressed);
    if ((pressed & PAD_UP) != 0) {
        result.cursor = result.cursor != 0 ? result.cursor - 1 : maxClass;
        result.moveCount++;
    }
    if ((pressed & PAD_DOWN) != 0) {
        result.cursor = result.cursor < maxClass ? result.cursor + 1 : 0;
        result.moveCount++;
    }
    return result;
}

CourseSelectScreenResult CourseSelectReduceInput(
    const CourseSelectScreenState *state,
    const CourseSelectScreenInput *input) {
    CourseSelectScreenResult result;

    result.state = *state;
    result.command = COURSE_SELECT_COMMAND_NONE;
    result.moveCount = 0;
    result.effects = COURSE_SELECT_EFFECT_NONE;

    if (state->phase == COURSE_SELECT_ACTIVE) {
        CourseSelectInputResult mainInput = CourseSelectHandleMainInput(
            state->option, input->grandPrixMode, input->pressed);
        result.state.option = mainInput.option;
        result.command = mainInput.command;
        result.moveCount = mainInput.moveCount;
        return result;
    }

    if (state->phase == COURSE_SELECT_SAVE_PROMPT) {
        CourseSelectModalResult modal = CourseSelectHandleSavePrompt(
            state->modalCursor, input->pressed);
        result.state.modalCursor = modal.cursor;
        result.moveCount = modal.moveCount;
        if (modal.confirmed) {
            result.state.phase = COURSE_SELECT_CONFIRM_SAVE;
            result.state.confirmTimer = 0x23;
            result.effects |= COURSE_SELECT_EFFECT_ACCEPT;
        }
        if (modal.cancelled) {
            result.state.phase = COURSE_SELECT_CLOSE_SAVE_PROMPT;
            result.effects |= COURSE_SELECT_EFFECT_CANCEL;
        }
        return result;
    }

    if (state->phase == COURSE_SELECT_CLASS_PROMPT) {
        CourseSelectModalResult modal = CourseSelectHandleClassPrompt(
            state->modalCursor, input->maxClass, input->pressed);
        result.state.modalCursor = modal.cursor;
        result.moveCount = modal.moveCount;
        if (modal.confirmed) {
            result.effects |= COURSE_SELECT_EFFECT_ACCEPT;
            if (state->modalCursor == input->currentClass) {
                result.state.phase = COURSE_SELECT_ACTIVE;
            } else {
                result.state.phase = COURSE_SELECT_CLASS_CHANGE;
                result.state.classChangeApplied = 0;
                result.state.confirmTimer = 0x23;
                result.effects |= COURSE_SELECT_EFFECT_BEGIN_CLASS_CHANGE;
            }
        }
        if (modal.cancelled) {
            result.state.phase = COURSE_SELECT_ACTIVE;
            result.effects |= COURSE_SELECT_EFFECT_CANCEL;
        }
    }
    return result;
}

CourseSelectScreenState CourseSelectTickConfirmTimer(
    const CourseSelectScreenState *state) {
    CourseSelectScreenState result = *state;
    if ((result.phase == COURSE_SELECT_CONFIRM_SAVE ||
         result.phase == COURSE_SELECT_CLASS_CHANGE) &&
        result.confirmTimer > 0) {
        result.confirmTimer--;
    }
    return result;
}

CourseSelectScreenState CourseSelectFinishModalClose(
    const CourseSelectScreenState *state) {
    CourseSelectScreenState result = *state;

    if (result.phase == COURSE_SELECT_CONFIRM_SAVE) {
        result.phase = result.modalCursor != 0
            ? COURSE_SELECT_TO_MEMORY_CARD
            : COURSE_SELECT_START_RACE;
    } else if (result.phase == COURSE_SELECT_CLOSE_SAVE_PROMPT) {
        result.phase = COURSE_SELECT_ACTIVE;
    }
    return result;
}
