#ifndef GAME_COURSE_SELECT_CONTROLLER_H
#define GAME_COURSE_SELECT_CONTROLLER_H

#include "common.h"

typedef struct CourseSelectRules {
    s32 grandPrixMode;
    s32 seriesSelection;
    s32 grandPrixClass;
    s32 extraGrandPrixUnlocked;
    s32 normalMaxClassReached;
    s32 extraMaxClassReached;
} CourseSelectRules;

typedef enum CourseSelectState {
    COURSE_SELECT_CLASS_CHANGE = -5,
    COURSE_SELECT_CLOSE_SAVE_PROMPT = -4,
    COURSE_SELECT_CONFIRM_SAVE = -3,
    COURSE_SELECT_CLASS_PROMPT = -2,
    COURSE_SELECT_SAVE_PROMPT = -1,
    COURSE_SELECT_ACTIVE = 0,
    COURSE_SELECT_TO_CAR_SELECT = 1,
    COURSE_SELECT_START_RACE = 2,
    COURSE_SELECT_TO_RANKING = 3,
    COURSE_SELECT_TO_MEMORY_CARD = 4
} CourseSelectState;

typedef enum CourseSelectCommand {
    COURSE_SELECT_COMMAND_NONE,
    COURSE_SELECT_COMMAND_OPEN_CAR_SELECT,
    COURSE_SELECT_COMMAND_OPEN_CLASS_SELECT,
    COURSE_SELECT_COMMAND_OPEN_RANKING,
    COURSE_SELECT_COMMAND_START_RACE,
    COURSE_SELECT_COMMAND_OPEN_SAVE_PROMPT
} CourseSelectCommand;

typedef struct CourseSelectInputResult {
    s32 option;
    u8 moveCount;
    CourseSelectCommand command;
} CourseSelectInputResult;

typedef enum CourseSelectModalCommand {
    COURSE_SELECT_MODAL_NONE,
    COURSE_SELECT_MODAL_CONFIRM,
    COURSE_SELECT_MODAL_CANCEL
} CourseSelectModalCommand;

typedef struct CourseSelectModalResult {
    s32 cursor;
    u8 moveCount;
    u8 confirmed;
    u8 cancelled;
    CourseSelectModalCommand command;
} CourseSelectModalResult;

typedef enum CourseSelectEffect {
    COURSE_SELECT_EFFECT_NONE = 0,
    COURSE_SELECT_EFFECT_ACCEPT = 1 << 0,
    COURSE_SELECT_EFFECT_CANCEL = 1 << 1,
    COURSE_SELECT_EFFECT_BEGIN_CLASS_CHANGE = 1 << 2
} CourseSelectEffect;

typedef struct CourseSelectScreenState {
    CourseSelectState phase;
    s32 option;
    s32 modalCursor;
    s32 confirmTimer;
    u8 classChangeApplied;
} CourseSelectScreenState;

typedef struct CourseSelectScreenInput {
    u16 pressed;
    s32 grandPrixMode;
    s32 currentClass;
    s32 maxClass;
} CourseSelectScreenInput;

typedef struct CourseSelectScreenResult {
    CourseSelectScreenState state;
    CourseSelectCommand command;
    u8 moveCount;
    u8 effects;
} CourseSelectScreenResult;

s32 CourseSelectFirstCourse(const CourseSelectRules *rules);
s32 CourseSelectLastCourse(const CourseSelectRules *rules);
s32 CourseSelectCanMove(const CourseSelectRules *rules,
                        s32 courseIndex, s32 direction);
CourseSelectInputResult CourseSelectHandleMainInput(s32 option,
                                                    s32 grandPrixMode,
                                                    u16 pressed);
CourseSelectModalResult CourseSelectHandleSavePrompt(s32 cursor, u16 pressed);
CourseSelectModalResult CourseSelectHandleClassPrompt(s32 cursor,
                                                      s32 maxClass,
                                                      u16 pressed);
CourseSelectScreenResult CourseSelectReduceInput(
    const CourseSelectScreenState *state,
    const CourseSelectScreenInput *input);
CourseSelectScreenState CourseSelectTickConfirmTimer(
    const CourseSelectScreenState *state);
CourseSelectScreenState CourseSelectFinishModalClose(
    const CourseSelectScreenState *state);

#endif
