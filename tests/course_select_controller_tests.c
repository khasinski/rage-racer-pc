#include "game/course_select_controller.h"
#include "game/pad.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    CourseSelectRules rules = {1, 0, 1, 0, 0, 0};
    CourseSelectInputResult input;
    CourseSelectModalResult modal;
    CourseSelectScreenState screen;
    CourseSelectScreenInput screenInput;
    CourseSelectScreenResult screenResult;

    EXPECT_EQ(2, CourseSelectLastCourse(&rules));
    EXPECT_EQ(0, CourseSelectCanMove(&rules, 0, -1));
    EXPECT_EQ(1, CourseSelectCanMove(&rules, 1, -1));
    rules.grandPrixClass = 2;
    EXPECT_EQ(3, CourseSelectLastCourse(&rules));
    rules.seriesSelection = 1;
    EXPECT_EQ(4, CourseSelectFirstCourse(&rules));
    EXPECT_EQ(7, CourseSelectLastCourse(&rules));
    EXPECT_EQ(0, CourseSelectCanMove(&rules, 4, -1));

    rules = (CourseSelectRules){0, 0, 0, 1, 0, 2};
    EXPECT_EQ(7, CourseSelectLastCourse(&rules));
    rules.extraGrandPrixUnlocked = 0;
    EXPECT_EQ(2, CourseSelectLastCourse(&rules));

    input = CourseSelectHandleMainInput(0, 1, PAD_UP | PAD_DOWN | PAD_CONFIRM);
    EXPECT_EQ(0, input.option);
    EXPECT_EQ(2, input.moveCount);
    EXPECT_EQ(COURSE_SELECT_COMMAND_OPEN_CAR_SELECT, input.command);
    input = CourseSelectHandleMainInput(1, 1, PAD_CONFIRM);
    EXPECT_EQ(COURSE_SELECT_COMMAND_OPEN_CLASS_SELECT, input.command);
    input = CourseSelectHandleMainInput(2, 1, PAD_CONFIRM);
    EXPECT_EQ(COURSE_SELECT_COMMAND_OPEN_SAVE_PROMPT, input.command);
    input = CourseSelectHandleMainInput(1, 0, PAD_CONFIRM);
    EXPECT_EQ(COURSE_SELECT_COMMAND_OPEN_RANKING, input.command);
    input = CourseSelectHandleMainInput(2, 0, PAD_CONFIRM);
    EXPECT_EQ(COURSE_SELECT_COMMAND_START_RACE, input.command);

    modal = CourseSelectHandleSavePrompt(1, PAD_RIGHT | PAD_CONFIRM | PAD_CANCEL);
    EXPECT_EQ(0, modal.cursor);
    EXPECT_EQ(1, modal.confirmed);
    EXPECT_EQ(1, modal.cancelled);
    modal = CourseSelectHandleClassPrompt(2, 3, PAD_UP | PAD_DOWN);
    EXPECT_EQ(2, modal.cursor);
    EXPECT_EQ(2, modal.moveCount);

    screen = (CourseSelectScreenState){COURSE_SELECT_ACTIVE, 1, 0, 0, 0};
    screenInput = (CourseSelectScreenInput){PAD_CONFIRM, 1, 0, 3};
    screenResult = CourseSelectReduceInput(&screen, &screenInput);
    EXPECT_EQ(COURSE_SELECT_COMMAND_OPEN_CLASS_SELECT, screenResult.command);

    screen = (CourseSelectScreenState){COURSE_SELECT_SAVE_PROMPT, 0, 1, 0, 0};
    screenInput.pressed = PAD_RIGHT | PAD_CONFIRM;
    screenResult = CourseSelectReduceInput(&screen, &screenInput);
    EXPECT_EQ(COURSE_SELECT_CONFIRM_SAVE, screenResult.state.phase);
    EXPECT_EQ(0, screenResult.state.modalCursor);
    EXPECT_EQ(0x23, screenResult.state.confirmTimer);
    EXPECT_EQ(COURSE_SELECT_EFFECT_ACCEPT, screenResult.effects);

    screenInput.pressed = PAD_CONFIRM | PAD_CANCEL;
    screenResult = CourseSelectReduceInput(&screen, &screenInput);
    EXPECT_EQ(COURSE_SELECT_CLOSE_SAVE_PROMPT, screenResult.state.phase);
    EXPECT_EQ(COURSE_SELECT_EFFECT_ACCEPT | COURSE_SELECT_EFFECT_CANCEL,
              screenResult.effects);

    screen = (CourseSelectScreenState){COURSE_SELECT_CLASS_PROMPT, 0, 2, 0, 1};
    screenInput = (CourseSelectScreenInput){PAD_CONFIRM, 1, 1, 3};
    screenResult = CourseSelectReduceInput(&screen, &screenInput);
    EXPECT_EQ(COURSE_SELECT_CLASS_CHANGE, screenResult.state.phase);
    EXPECT_EQ(0x23, screenResult.state.confirmTimer);
    EXPECT_EQ(0, screenResult.state.classChangeApplied);
    EXPECT_EQ(COURSE_SELECT_EFFECT_ACCEPT |
                  COURSE_SELECT_EFFECT_BEGIN_CLASS_CHANGE,
              screenResult.effects);

    screen = CourseSelectTickConfirmTimer(&screenResult.state);
    EXPECT_EQ(0x22, screen.confirmTimer);
    screen = (CourseSelectScreenState){COURSE_SELECT_CONFIRM_SAVE, 0, 0, 0, 0};
    screen = CourseSelectFinishModalClose(&screen);
    EXPECT_EQ(COURSE_SELECT_START_RACE, screen.phase);
    screen = (CourseSelectScreenState){COURSE_SELECT_CONFIRM_SAVE, 0, 1, 0, 0};
    screen = CourseSelectFinishModalClose(&screen);
    EXPECT_EQ(COURSE_SELECT_TO_MEMORY_CARD, screen.phase);
    return 0;
}
