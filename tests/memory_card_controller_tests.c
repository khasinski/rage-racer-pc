#include "game/memory_card_controller.h"
#include "game/memory_card_types.h"
#include "game/pad.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

static MemoryCardActionResult Reduce(
    MemoryCardActionSession *state, MemoryCardActionEventType type,
    s32 value, s32 saveOperation, s32 finalRowCursor) {
    MemoryCardActionEvent event = {
        type, value, saveOperation, finalRowCursor};
    return MemoryCardActionReduce(state, &event);
}

static MemoryCardReadResult ReduceRead(
    MemoryCardReadSession *state, MemoryCardReadEventType type,
    s32 sceneTimer, s32 cardStatus, s32 refreshResult, u16 pressed,
    u8 fadeBusy) {
    MemoryCardReadEvent event = {
        type, sceneTimer, cardStatus, refreshResult, pressed, fadeBusy};
    return MemoryCardReadReduce(state, &event);
}

static MemoryCardFormatResult ReduceFormat(
    MemoryCardFormatSession *state, MemoryCardFormatEventType type,
    s32 ioResult, s32 menuRowCount, u16 pressed, u16 pressedRepeat,
    u8 fadeBusy) {
    MemoryCardFormatEvent event = {
        type, ioResult, menuRowCount, pressed, pressedRepeat, fadeBusy};
    return MemoryCardFormatReduce(state, &event);
}

int main(void) {
    SaveSession state = {3, 0, 0, 0, 0, 0, -1, 0, 3};
    int index;
    MemoryCardActionSession action;
    MemoryCardReadSession read;
    MemoryCardReadResult readResult;
    MemoryCardFormatSession format;
    MemoryCardFormatResult formatResult;
    MemoryCardNoCardSession noCard;
    MemoryCardNoCardInput noCardInput;
    MemoryCardNoCardResult noCardResult;
    static const struct {
        MemoryCardReadySession initial;
        MemoryCardReadyInput input;
        MemoryCardPage expectedPage;
        MemoryCardActionState expectedAction;
        s32 expectedPrompt;
        u32 expectedEffects;
    } readyCases[] = {
        {{MC_PAGE_MODE_SELECT, MC_ACTION_IDLE, 0, 0, 0, 0, 0, 0},
         {PAD_CONFIRM, 0, 3, 0, 1, 2, 0},
         MC_PAGE_SLOT_ACTION, MC_ACTION_IDLE, MC_PROMPT_NONE,
         MC_READY_EFFECT_ACCEPT},
        {{MC_PAGE_MODE_SELECT, MC_ACTION_IDLE, 2, 0, 0, 0, 0, 0},
         {PAD_CANCEL, 0, 3, 0, 1, 0, 0},
         MC_PAGE_MODE_SELECT, MC_ACTION_IDLE, MC_PROMPT_NONE,
         MC_READY_EFFECT_BACK | MC_READY_EFFECT_EXIT},
        {{MC_PAGE_SLOT_ACTION, MC_ACTION_IDLE, 0, 1, 1, 0, 0, 0},
         {PAD_CONFIRM, 0, 3, 2, 1, 0, 0},
         MC_PAGE_SLOT_ACTION, MC_ACTION_LOAD_PREPARE,
         MC_PROMPT_SELECT_LOAD, MC_READY_EFFECT_ACCEPT},
        {{MC_PAGE_SLOT_ACTION, MC_ACTION_IDLE, 0, 0, 1, 0, 0, 0},
         {PAD_CONFIRM, 0, 3, 2, 1, 0, 0},
         MC_PAGE_SLOT_ACTION, MC_ACTION_NO_FILE,
         MC_PROMPT_SELECT_LOAD, MC_READY_EFFECT_INVALID},
        {{MC_PAGE_SLOT_ACTION, MC_ACTION_IDLE, 0, 2, 0, 0, 0, 0},
         {PAD_CONFIRM, 0, 3, 0, 1, 0, 0},
         MC_PAGE_SLOT_ACTION, MC_ACTION_SAVE_PREPARE,
         MC_PROMPT_SELECT_SAVE, MC_READY_EFFECT_ACCEPT},
        {{MC_PAGE_SLOT_ACTION, MC_ACTION_IDLE, 0, 1, 0, 0, 0, 0},
         {PAD_CONFIRM, 0, 3, 2, 1, 0, 0},
         MC_PAGE_SLOT_ACTION, MC_ACTION_CONFIRM_OVERWRITE,
         MC_PROMPT_SELECT_SAVE, MC_READY_EFFECT_ACCEPT},
        {{MC_PAGE_SLOT_ACTION, MC_ACTION_IDLE, 0, 0, 0, 0, 0, 0},
         {PAD_CONFIRM, 0, 3, 0, 0, 0, 0},
         MC_PAGE_MODE_SELECT, MC_ACTION_IDLE,
         MC_PROMPT_CARD_FULL, MC_READY_EFFECT_INVALID},
    };
    MemoryCardCursorResult cursor;
    EXPECT_EQ(1, MemoryCardControllerShouldPoll(0, 0));
    EXPECT_EQ(0, MemoryCardControllerShouldPoll(1, 0));
    EXPECT_EQ(1, MemoryCardControllerShouldPoll(1, 1));

    for (index = 0; index < 6; index++)
        MemoryCardControllerApplyStatus(&state, 0);
    EXPECT_EQ(0, state.selection);
    MemoryCardControllerApplyStatus(&state, 0);
    EXPECT_EQ(3, state.selection);

    MemoryCardControllerApplyStatus(&state, 1);
    EXPECT_EQ(1, state.selection);
    EXPECT_EQ(2, state.subState);
    MemoryCardControllerResolveDetection(&state);
    EXPECT_EQ(2, state.menuState);

    state = (SaveSession){3, -3, 0, -3, 0, 4, -1, 0, 3};
    MemoryCardControllerResolveDetection(&state);
    EXPECT_EQ(-3, state.menuState);
    EXPECT_EQ(0, state.errorTicks);

    state = (SaveSession){1, 3, 0, 3, 0, 0, -1, 0, 3};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(3, state.menuState);
    EXPECT_EQ(1, state.lastMenuState);

    state = (SaveSession){-1, 1, 0, 1, 0, 0, 0, 1, 1};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(2, state.menuState);
    EXPECT_EQ(0, state.errorPending);
    EXPECT_EQ(3, state.errorCountdown);

    state = (SaveSession){2, -3, 0, -3, 0, 0, 0, 0, 2};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(2, state.menuState);
    EXPECT_EQ(1, state.errorPending);
    EXPECT_EQ(1, state.errorCountdown);
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(-3, state.menuState);

    state = (SaveSession){-2, -1, 0, -1, 0, 0, 0, 0, 3};
    MemoryCardControllerResolveTransition(&state);
    EXPECT_EQ(-1, state.menuState);

    action = (MemoryCardActionSession){
        MC_ACTION_SAVE_PREPARE, 30, 0, 0, 1, 0, 0};
    Reduce(&action, MC_ACTION_EVENT_TICK, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_DELAY, action.state);
    EXPECT_EQ(10, action.timer);
    action.timer = 2;
    Reduce(&action, MC_ACTION_EVENT_TICK, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_DELAY, action.state);
    Reduce(&action, MC_ACTION_EVENT_TICK, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_WRITE, action.state);
    EXPECT_EQ(MC_ACTION_EFFECT_WRITE_SLOT,
              MemoryCardActionRequestedEffect(&action));
    Reduce(&action, MC_ACTION_EVENT_EFFECT_COMPLETE, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_POST_WRITE, action.state);
    Reduce(&action, MC_ACTION_EVENT_TICK, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_REFRESH, action.state);
    Reduce(&action, MC_ACTION_EVENT_EFFECT_COMPLETE, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_SETTLE_PREPARE, action.state);
    Reduce(&action, MC_ACTION_EVENT_TICK, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_SETTLE_DELAY, action.state);
    EXPECT_EQ(5, action.timer);

    action = (MemoryCardActionSession){MC_ACTION_SAVE_WAIT_CARD, 0, 0, 1, 1, 0, 0};
    for (index = 0; index < 3; index++)
        Reduce(&action, MC_ACTION_EVENT_CARD_STATUS, 1, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_WAIT_CARD, action.state);
    Reduce(&action, MC_ACTION_EVENT_CARD_STATUS, 1, 0, 2);
    EXPECT_EQ(MC_ACTION_SAVE_SHOW_RESULT, action.state);

    Reduce(&action, MC_ACTION_EVENT_IO_RESULT, 1, 1, 2);
    EXPECT_EQ(MC_ACTION_SAVE_RESULT_DELAY, action.state);
    EXPECT_EQ(0x12, action.prompt);
    EXPECT_EQ(0x3C, action.timer);
    action.timer = 1;
    Reduce(&action, MC_ACTION_EVENT_TICK, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_IDLE, action.state);
    EXPECT_EQ(0, action.menuPage);
    EXPECT_EQ(2, action.menuRowCursor);

    action = (MemoryCardActionSession){MC_ACTION_LOAD_WAIT_CARD, 0, 3, 1, 1, 0, 0};
    EXPECT_EQ(MC_ACTION_EFFECT_POLL_CARD,
              MemoryCardActionRequestedEffect(&action));
    Reduce(&action, MC_ACTION_EVENT_CARD_STATUS, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_LOAD_WAIT_CARD, action.state);
    Reduce(&action, MC_ACTION_EVENT_CARD_STATUS, 1, 0, 2);
    EXPECT_EQ(MC_ACTION_LOAD_SHOW_RESULT, action.state);
    Reduce(&action, MC_ACTION_EVENT_IO_RESULT, 0, 0, 2);
    EXPECT_EQ(MC_ACTION_LOAD_RESULT_DELAY, action.state);
    EXPECT_EQ(0x10, action.prompt);

    read = (MemoryCardReadSession){MC_READ_WAIT_SCENE, 0, 0, 0, 0, 1};
    ReduceRead(&read, MC_READ_EVENT_TICK, 30, 0, 0, 0, 0);
    EXPECT_EQ(MC_READ_WAIT_SCENE, read.state);
    ReduceRead(&read, MC_READ_EVENT_TICK, 31, 0, 0, 0, 0);
    EXPECT_EQ(MC_READ_WAIT_CARD, read.state);
    ReduceRead(&read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(MC_READ_WAIT_CARD, read.state);
    ReduceRead(&read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(MC_READ_PREPARE, read.state);
    ReduceRead(&read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(MC_READ_DELAY, read.state);
    for (index = 0; index < 5; index++)
        readResult = ReduceRead(
            &read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(MC_READ_REFRESH, read.state);
    readResult = ReduceRead(
        &read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(MC_READ_EFFECT_REFRESH_SLOTS, readResult.effect);
    ReduceRead(&read, MC_READ_EVENT_REFRESH_RESULT, 31, 1, 5, 0, 0);
    EXPECT_EQ(MC_READ_POST_REFRESH, read.state);
    EXPECT_EQ(2, read.menuSubState);
    ReduceRead(&read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    for (index = 0; index < 5; index++)
        ReduceRead(&read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(MC_READ_SETTLE_DELAY, read.state);
    for (index = 0; index < 5; index++)
        ReduceRead(&read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(MC_READ_COMPLETE, read.state);
    readResult = ReduceRead(
        &read, MC_READ_EVENT_TICK, 31, 1, 0, 0, 0);
    EXPECT_EQ(1, readResult.complete);

    read = (MemoryCardReadSession){MC_READ_WAIT_CARD, 0, 1, 120, 0, 1};
    readResult = ReduceRead(
        &read, MC_READ_EVENT_TICK, 31, 0, 0, PAD_CANCEL, 0);
    EXPECT_EQ(MC_READ_EFFECT_EXIT, readResult.effect);
    EXPECT_EQ(0, read.elapsed);

    format = (MemoryCardFormatSession){
        MC_FORMAT_IDLE, 0, 0, 0, MC_PAGE_SLOT_ACTION, 0, 0, 0, 0};
    formatResult = ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 0, PAD_CONFIRM, 0, 0);
    EXPECT_EQ(MC_FORMAT_CONFIRM, format.state);
    EXPECT_EQ(MC_FORMAT_EFFECT_ACCEPT, formatResult.effects);
    formatResult = ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 0, 0, PAD_LEFT, 0);
    EXPECT_EQ(1, format.confirmChoice);
    EXPECT_EQ(MC_FORMAT_EFFECT_MOVE, formatResult.effects);
    ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 0, PAD_CONFIRM, 0, 0);
    EXPECT_EQ(MC_FORMAT_PREPARE, format.state);
    ReduceFormat(&format, MC_FORMAT_EVENT_TICK, 0, 0, 0, 0, 0);
    EXPECT_EQ(MC_FORMAT_DELAY, format.state);
    for (index = 0; index < 20; index++)
        formatResult = ReduceFormat(
            &format, MC_FORMAT_EVENT_TICK, 0, 0, 0, 0, 0);
    EXPECT_EQ(MC_FORMAT_EXECUTE, format.state);
    formatResult = ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 0, 0, 0, 0);
    EXPECT_EQ(MC_FORMAT_EFFECT_FORMAT, formatResult.effects);
    ReduceFormat(
        &format, MC_FORMAT_EVENT_IO_RESULT, 1, 0, 0, 0, 0);
    EXPECT_EQ(MC_FORMAT_SUCCESS_DELAY, format.state);
    for (index = 0; index < 60; index++)
        ReduceFormat(&format, MC_FORMAT_EVENT_TICK, 0, 0, 0, 0, 0);
    EXPECT_EQ(MC_FORMAT_SUCCESS, format.state);
    formatResult = ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 0, PAD_CANCEL, 0, 0);
    EXPECT_EQ(MC_FORMAT_IDLE, format.state);
    EXPECT_EQ(MC_FORMAT_EFFECT_BACK | MC_FORMAT_EFFECT_EXIT,
              formatResult.effects);

    format = (MemoryCardFormatSession){
        MC_FORMAT_EXECUTE, 0, 0, 1, MC_PAGE_SLOT_ACTION, 0, 0, 0, 0};
    ReduceFormat(&format, MC_FORMAT_EVENT_IO_RESULT, 0, 0, 0, 0, 0);
    EXPECT_EQ(MC_FORMAT_ERROR, format.state);
    formatResult = ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 0, PAD_CANCEL, 0, 0);
    EXPECT_EQ(MC_FORMAT_IDLE, format.state);
    EXPECT_EQ(MC_FORMAT_EFFECT_BACK, formatResult.effects);

    format = (MemoryCardFormatSession){
        MC_FORMAT_IDLE, 0, 0, 0, MC_PAGE_MODE_SELECT, 0, 0, 0, 0};
    formatResult = ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 3, PAD_CONFIRM, 0, 0);
    EXPECT_EQ(MC_PAGE_SLOT_ACTION, format.menuPage);
    EXPECT_EQ(0, format.saveMode);
    EXPECT_EQ(MC_FORMAT_EFFECT_ACCEPT, formatResult.effects);
    format = (MemoryCardFormatSession){
        MC_FORMAT_IDLE, 0, 0, 0, MC_PAGE_MODE_SELECT, 0, 0, 1, 0};
    formatResult = ReduceFormat(
        &format, MC_FORMAT_EVENT_TICK, 0, 3, PAD_CONFIRM, 0, 0);
    EXPECT_EQ(MC_PAGE_SLOT_ACTION, format.menuPage);
    EXPECT_EQ(1, format.saveMode);
    EXPECT_EQ(MC_FORMAT_EFFECT_INVALID, formatResult.effects);

    noCard = (MemoryCardNoCardSession){
        MC_NO_CARD_PREPARE, 0, MC_PAGE_MODE_SELECT, 1, 7, 2};
    noCardInput = (MemoryCardNoCardInput){0, 0, 3, 0};
    noCardResult = MemoryCardNoCardReduce(&noCard, &noCardInput);
    EXPECT_EQ(MC_NO_CARD_DELAY, noCard.state);
    EXPECT_EQ(5, noCard.timer);
    EXPECT_EQ(0, noCard.slotUsedMask);
    EXPECT_EQ(MC_NO_CARD_EFFECT_CLEAR_SLOTS, noCardResult.effects);
    for (index = 0; index < 5; index++)
        MemoryCardNoCardReduce(&noCard, &noCardInput);
    EXPECT_EQ(MC_NO_CARD_INPUT, noCard.state);
    noCardInput.pressed = PAD_CONFIRM;
    noCardResult = MemoryCardNoCardReduce(&noCard, &noCardInput);
    EXPECT_EQ(MC_NO_CARD_EFFECT_INVALID, noCardResult.effects);
    noCardInput.pressed = 0;
    noCardInput.pressedRepeat = PAD_DOWN;
    noCardResult = MemoryCardNoCardReduce(&noCard, &noCardInput);
    EXPECT_EQ(2, noCard.menuRowCursor);
    EXPECT_EQ(MC_NO_CARD_EFFECT_MOVE, noCardResult.effects);
    noCardInput.pressed = PAD_CONFIRM;
    noCardInput.pressedRepeat = 0;
    noCardResult = MemoryCardNoCardReduce(&noCard, &noCardInput);
    EXPECT_EQ(MC_NO_CARD_PREPARE, noCard.state);
    EXPECT_EQ(MC_NO_CARD_EFFECT_ACCEPT | MC_NO_CARD_EFFECT_EXIT,
              noCardResult.effects);

    for (index = 0;
         index < (int)(sizeof(readyCases) / sizeof(readyCases[0]));
         index++) {
        MemoryCardReadySession ready = readyCases[index].initial;
        MemoryCardReadyResult readyResult =
            MemoryCardReadyReduce(&ready, &readyCases[index].input);
        EXPECT_EQ(readyCases[index].expectedPage, ready.page);
        EXPECT_EQ(readyCases[index].expectedAction, ready.actionState);
        EXPECT_EQ(readyCases[index].expectedPrompt, ready.prompt);
        EXPECT_EQ(readyCases[index].expectedEffects, readyResult.effects);
    }

    {
        MemoryCardReadySession ready = {
            MC_PAGE_SLOT_ACTION, MC_ACTION_CONFIRM_OVERWRITE,
            0, 2, 0, 0, 0, 0};
        MemoryCardReadyInput input = {0, PAD_LEFT, 3, 4, 0, 0, 0};
        MemoryCardReadyResult readyResult =
            MemoryCardReadyReduce(&ready, &input);
        EXPECT_EQ(1, ready.confirmChoice);
        EXPECT_EQ(MC_READY_EFFECT_MOVE, readyResult.effects);
        input.pressed = PAD_CONFIRM;
        input.pressedRepeat = 0;
        readyResult = MemoryCardReadyReduce(&ready, &input);
        EXPECT_EQ(MC_ACTION_SAVE_PREPARE, ready.actionState);
        EXPECT_EQ(MC_READY_EFFECT_ACCEPT, readyResult.effects);
    }

    cursor = MemoryCardMoveMenuRow(1, 0, 2, PAD_UP | PAD_DOWN);
    EXPECT_EQ(2, cursor.value);
    EXPECT_EQ(1, cursor.moved);
    cursor = MemoryCardMoveMenuRow(2, 0, 2, PAD_DOWN);
    EXPECT_EQ(2, cursor.value);
    EXPECT_EQ(0, cursor.moved);
    cursor = MemoryCardSetBinaryChoice(0, PAD_LEFT | PAD_RIGHT);
    EXPECT_EQ(1, cursor.value);
    EXPECT_EQ(1, cursor.moved);
    return 0;
}
