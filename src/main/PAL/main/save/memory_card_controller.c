#include "game/memory_card_controller.h"
#include "game/memory_card_types.h"
#include "game/pad.h"

s32 MemoryCardControllerShouldPoll(s32 actionBusy, s32 errorPending) {
    return actionBusy == 0 || errorPending != 0;
}

void MemoryCardControllerApplyStatus(SaveSession *state,
                                     s32 cardStatus) {
    s32 subState;
    state->cardStatus = cardStatus;
    if (cardStatus == 0) {
        s32 previousTicks = state->noCardTicks++;
        if (previousTicks >= 6) state->selection = MC_MENU_DETECTING_CARD;
        return;
    }
    switch (cardStatus) {
    case 1: subState = 2; break;
    case 2: subState = 1; break;
    case -1: subState = 0xA; break;
    case -2: subState = 0xB; break;
    default: subState = 0x11; break;
    }
    state->noCardTicks = 0;
    state->subState = subState;
    state->selection = cardStatus;
}

void MemoryCardControllerResolveDetection(SaveSession *state) {
    switch (state->selection) {
    case 1:
        if (state->cardStatus == 1)
            state->menuState = state->lastMenuState != MC_MENU_READING_CARD
                ? MC_MENU_READING_CARD : MC_MENU_READY;
        break;
    case 2:
        state->menuState = MC_MENU_READING_CARD;
        break;
    case -1:
    case -2:
        state->menuState = state->selection;
        break;
    case 3:
        break;
    default:
        if (state->cardStatus == MC_MENU_CARD_ERROR) {
            s32 previousTicks = state->errorTicks++;
            if (previousTicks >= 4) state->menuState = MC_MENU_CARD_ERROR;
        }
        break;
    }
    if (state->menuState != MC_MENU_DETECTING_CARD) state->errorTicks = 0;
}

static void ResolveCardError(SaveSession *state) {
    state->errorPending = 1;
    if (state->cardStatus == MC_MENU_CARD_ERROR) {
        state->errorCountdown--;
        if (state->errorCountdown == 0)
            state->menuState = MC_MENU_CARD_ERROR;
    }
}

static void ClearPendingError(SaveSession *state) {
    if (state->errorPending != 0) {
        state->errorPending = 0;
        state->errorCountdown = 3;
    }
}

void MemoryCardControllerResolveTransition(SaveSession *state) {
    s32 current = state->menuState;

    if (state->selection == MC_MENU_DETECTING_CARD) {
        state->lastMenuState = current;
        state->menuState = MC_MENU_DETECTING_CARD;
        return;
    }

    switch (current) {
    case MC_MENU_READY:
        switch (state->selection) {
        case -2:
        case -1:
        case 2:
            state->menuState = state->selection;
            break;
        case 1:
            ClearPendingError(state);
            break;
        default:
            ResolveCardError(state);
            break;
        }
        break;
    case MC_MENU_READING_CARD:
        switch (state->selection) {
        case -2:
        case -1:
            state->menuState = state->selection;
            break;
        case 2:
            ClearPendingError(state);
            break;
        case 1:
            break;
        default:
            ResolveCardError(state);
            break;
        }
        break;
    case MC_MENU_NO_CARD:
        switch (state->selection) {
        case 1:
        case 2:
            state->menuState = MC_MENU_READING_CARD;
            ClearPendingError(state);
            break;
        case -1:
            ClearPendingError(state);
            break;
        case -2:
            state->menuState = MC_MENU_NEW_CARD;
            break;
        case 3:
            break;
        default:
            ResolveCardError(state);
            break;
        }
        break;
    case MC_MENU_NEW_CARD:
        switch (state->selection) {
        case 1:
        case 2:
            state->menuState = MC_MENU_READING_CARD;
            break;
        case -1:
            state->menuState = MC_MENU_NO_CARD;
            break;
        case -2:
            ClearPendingError(state);
            break;
        default:
            ResolveCardError(state);
            break;
        }
        break;
    }
}

MemoryCardReadResult MemoryCardReadReduce(
    MemoryCardReadSession *state, const MemoryCardReadEvent *event) {
    MemoryCardReadResult result = {MC_READ_EFFECT_NONE, 0};

    if (event->type == MC_READ_EVENT_REFRESH_RESULT) {
        s32 slots = event->refreshResult;

        state->menuSubState = slots == 0 ? 0xC
            : (slots & 7) != 0 ? 2 : 0xE;
        state->state = MC_READ_POST_REFRESH;
        return result;
    }

    switch (state->state) {
    case MC_READ_WAIT_SCENE:
        if ((u32)event->sceneTimer >= 0x1F) {
            state->cardOkFrames = 0;
            state->elapsed = 0;
            state->state = MC_READ_WAIT_CARD;
        }
        break;
    case MC_READ_WAIT_CARD:
        state->busy = 0;
        state->elapsed++;
        if ((event->pressed & PAD_CANCEL) != 0 && state->elapsed >= 0x79) {
            state->cardOkFrames = 0;
            state->elapsed = 0;
            if (!event->fadeBusy) result.effect = MC_READ_EFFECT_EXIT;
        }
        if (event->cardStatus == 1) {
            state->cardOkFrames++;
            if (state->cardOkFrames >= 2) {
                state->cardOkFrames = 0;
                state->elapsed = 0;
                state->state = MC_READ_PREPARE;
            }
        }
        break;
    case MC_READ_PREPARE:
        state->busy = 1;
        state->timer = 5;
        state->state = MC_READ_DELAY;
        break;
    case MC_READ_DELAY:
        if (--state->timer == 0) state->state = MC_READ_REFRESH;
        break;
    case MC_READ_REFRESH:
        result.effect = MC_READ_EFFECT_REFRESH_SLOTS;
        break;
    case MC_READ_POST_REFRESH:
        state->timer = 5;
        state->state = MC_READ_SETTLE_PREPARE;
        break;
    case MC_READ_SETTLE_PREPARE:
        if (--state->timer == 0) {
            state->timer = 5;
            state->busy = 0;
            state->state = MC_READ_SETTLE_DELAY;
        }
        break;
    case MC_READ_SETTLE_DELAY:
        if (--state->timer == 0) state->state = MC_READ_COMPLETE;
        break;
    case MC_READ_COMPLETE:
        result.complete = event->cardStatus == 1;
        break;
    default:
        break;
    }
    return result;
}

MemoryCardFormatResult MemoryCardFormatReduce(
    MemoryCardFormatSession *state, const MemoryCardFormatEvent *event) {
    MemoryCardFormatResult result = {MC_FORMAT_EFFECT_NONE};
    const u8 confirm = (event->pressed & PAD_CONFIRM) != 0;
    const u8 cancel = (event->pressed & PAD_CANCEL) != 0;

    if (event->type == MC_FORMAT_EVENT_IO_RESULT) {
        if (event->ioResult == 1) {
            state->state = MC_FORMAT_SUCCESS_DELAY;
            state->timer = 0x3C;
        } else {
            state->state = MC_FORMAT_ERROR;
        }
        return result;
    }

    if (state->menuPage == MC_PAGE_MODE_SELECT) {
        MemoryCardCursorResult cursor = MemoryCardMoveMenuRow(
            state->menuRowCursor, 0, event->menuRowCount - 1,
            event->pressedRepeat);
        state->menuSubState = 0xB;
        state->prompt = MC_PROMPT_NONE;
        state->menuRowCursor = cursor.value;
        if (cursor.moved) result.effects |= MC_FORMAT_EFFECT_MOVE;
        if (confirm) {
            if (state->menuRowCursor == event->menuRowCount - 1) {
                if (!event->fadeBusy)
                    result.effects |=
                        MC_FORMAT_EFFECT_ACCEPT | MC_FORMAT_EFFECT_EXIT;
            } else {
                state->menuPage = MC_PAGE_SLOT_ACTION;
                state->confirmChoice = 0;
                state->saveMode = state->menuRowCursor;
                result.effects |= state->menuRowCursor == 0
                    ? MC_FORMAT_EFFECT_ACCEPT : MC_FORMAT_EFFECT_INVALID;
            }
        } else if (cancel && !event->fadeBusy) {
            result.effects |=
                MC_FORMAT_EFFECT_BACK | MC_FORMAT_EFFECT_EXIT;
        }
        return result;
    }

    switch (state->state) {
    case MC_FORMAT_IDLE:
        if (state->saveMode != 0) {
            state->prompt = MC_PROMPT_NO_DATA;
            if (confirm || cancel) {
                state->menuPage = MC_PAGE_MODE_SELECT;
                result.effects = confirm
                    ? MC_FORMAT_EFFECT_ACCEPT : MC_FORMAT_EFFECT_BACK;
            }
        } else {
            state->prompt = MC_PROMPT_NEW_CARD;
            if (confirm) {
                state->state = MC_FORMAT_CONFIRM;
                result.effects = MC_FORMAT_EFFECT_ACCEPT;
            } else if (cancel) {
                state->menuPage = MC_PAGE_MODE_SELECT;
                result.effects = MC_FORMAT_EFFECT_BACK;
            }
        }
        break;
    case MC_FORMAT_CONFIRM: {
        MemoryCardCursorResult choice =
            MemoryCardSetBinaryChoice(state->confirmChoice,
                                      event->pressedRepeat);
        state->confirmChoice = choice.value;
        state->prompt = state->confirmChoice + 7;
        if (choice.moved) result.effects |= MC_FORMAT_EFFECT_MOVE;
        if (confirm) {
            result.effects |= MC_FORMAT_EFFECT_ACCEPT;
            if (state->confirmChoice != 0)
                state->state = MC_FORMAT_PREPARE;
            else {
                state->state = MC_FORMAT_IDLE;
                state->menuPage = MC_PAGE_MODE_SELECT;
            }
        } else if (cancel) {
            state->state = MC_FORMAT_IDLE;
            state->menuPage = MC_PAGE_MODE_SELECT;
            result.effects |= MC_FORMAT_EFFECT_BACK;
        }
        break;
    }
    case MC_FORMAT_PREPARE:
        state->busy = 1;
        state->timer = 0x14;
        state->state = MC_FORMAT_DELAY;
        break;
    case MC_FORMAT_DELAY:
        if (--state->timer == 0) state->state = MC_FORMAT_EXECUTE;
        break;
    case MC_FORMAT_EXECUTE:
        result.effects = MC_FORMAT_EFFECT_FORMAT;
        break;
    case MC_FORMAT_SUCCESS_DELAY:
        state->prompt = MC_PROMPT_FORMAT_OK;
        if (--state->timer == 0) {
            state->busy = 0;
            state->state = MC_FORMAT_SUCCESS;
        }
        break;
    case MC_FORMAT_SUCCESS:
        state->prompt = MC_PROMPT_FORMAT_OK;
        if (cancel) {
            state->busy = 0;
            state->state = MC_FORMAT_IDLE;
            state->confirmChoice = 0;
            state->timer = 0;
            if (!event->fadeBusy)
                result.effects =
                    MC_FORMAT_EFFECT_BACK | MC_FORMAT_EFFECT_EXIT;
        }
        break;
    case MC_FORMAT_ERROR:
        state->menuSubState = 0x12;
        state->prompt = MC_PROMPT_CARD_ERROR;
        state->busy = 0;
        if (confirm || cancel) {
            state->state = MC_FORMAT_IDLE;
            result.effects = confirm
                ? MC_FORMAT_EFFECT_ACCEPT : MC_FORMAT_EFFECT_BACK;
        }
        break;
    default:
        break;
    }
    return result;
}

MemoryCardNoCardResult MemoryCardNoCardReduce(
    MemoryCardNoCardSession *state, const MemoryCardNoCardInput *input) {
    MemoryCardNoCardResult result = {MC_NO_CARD_EFFECT_NONE};

    switch (state->state) {
    case MC_NO_CARD_PREPARE:
        state->timer = 5;
        state->slotUsedMask = 0;
        state->lastSlot = 0;
        state->state = MC_NO_CARD_DELAY;
        result.effects = MC_NO_CARD_EFFECT_CLEAR_SLOTS;
        break;
    case MC_NO_CARD_DELAY:
        if (--state->timer == 0) state->state = MC_NO_CARD_INPUT;
        break;
    case MC_NO_CARD_INPUT:
        if (state->menuPage == MC_PAGE_MODE_SELECT) {
            MemoryCardCursorResult cursor = MemoryCardMoveMenuRow(
                state->menuRowCursor, 0, input->menuRowCount - 1,
                input->pressedRepeat);
            state->menuRowCursor = cursor.value;
            if (cursor.moved) result.effects |= MC_NO_CARD_EFFECT_MOVE;
            if ((input->pressed & PAD_CONFIRM) != 0) {
                if (state->menuRowCursor == input->menuRowCount - 1) {
                    if (!input->fadeBusy) {
                        state->state = MC_NO_CARD_PREPARE;
                        result.effects |= MC_NO_CARD_EFFECT_ACCEPT |
                            MC_NO_CARD_EFFECT_EXIT;
                    }
                } else {
                    result.effects |= MC_NO_CARD_EFFECT_INVALID;
                }
            } else if ((input->pressed & PAD_CANCEL) != 0 &&
                       !input->fadeBusy) {
                state->state = MC_NO_CARD_PREPARE;
                result.effects |=
                    MC_NO_CARD_EFFECT_BACK | MC_NO_CARD_EFFECT_EXIT;
            }
        } else if (state->menuPage == MC_PAGE_SLOT_ACTION &&
                   (input->pressed & PAD_CANCEL) != 0 &&
                   !input->fadeBusy) {
            result.effects =
                MC_NO_CARD_EFFECT_BACK | MC_NO_CARD_EFFECT_EXIT;
        }
        break;
    default:
        break;
    }
    return result;
}

static void MemoryCardReadyReturnToMode(MemoryCardReadySession *state) {
    state->page = MC_PAGE_MODE_SELECT;
    state->actionState = MC_ACTION_IDLE;
}

static void MemoryCardReadyReduceMode(
    MemoryCardReadySession *state, const MemoryCardReadyInput *input,
    MemoryCardReadyResult *result) {
    MemoryCardCursorResult cursor = MemoryCardMoveMenuRow(
        state->menuRowCursor, 0, input->menuRowCount - 1,
        input->pressedRepeat);
    state->prompt = MC_PROMPT_NONE;
    state->menuRowCursor = cursor.value;
    if (cursor.moved) result->effects |= MC_READY_EFFECT_MOVE;

    if ((input->pressed & PAD_CONFIRM) != 0) {
        if (state->menuRowCursor < input->menuRowCount - 1) {
            state->page = MC_PAGE_SLOT_ACTION;
            state->actionState = MC_ACTION_IDLE;
            state->slotCursor = input->lastSlot;
            state->saveMode = state->menuRowCursor;
            result->effects |= MC_READY_EFFECT_ACCEPT;
        } else if (!input->fadeBusy) {
            result->effects |=
                MC_READY_EFFECT_ACCEPT | MC_READY_EFFECT_EXIT;
        }
    } else if ((input->pressed & PAD_CANCEL) != 0 && !input->fadeBusy) {
        result->effects |= MC_READY_EFFECT_BACK | MC_READY_EFFECT_EXIT;
    }
}

static void MemoryCardReadyReduceSlotIdle(
    MemoryCardReadySession *state, const MemoryCardReadyInput *input,
    MemoryCardReadyResult *result) {
    MemoryCardCursorResult cursor = MemoryCardMoveMenuRow(
        state->slotCursor, 0, 2, input->pressedRepeat);
    const s32 hasAnySave = (input->slotUsedMask & 7) != 0;
    const s32 slotUsed =
        (input->slotUsedMask >> state->slotCursor) & 1;
    const u8 confirm = (input->pressed & PAD_CONFIRM) != 0;
    const u8 cancel = (input->pressed & PAD_CANCEL) != 0;

    state->slotCursor = cursor.value;
    if (cursor.moved) result->effects |= MC_READY_EFFECT_MOVE;

    if (state->saveMode != 0) {
        state->prompt = hasAnySave
            ? MC_PROMPT_SELECT_LOAD : MC_PROMPT_NO_DATA;
        if (confirm) {
            if (!hasAnySave) {
                MemoryCardReadyReturnToMode(state);
                result->effects |= MC_READY_EFFECT_INVALID;
            } else if (slotUsed) {
                state->confirmChoice = 0;
                state->actionState = MC_ACTION_LOAD_PREPARE;
                result->effects |= MC_READY_EFFECT_ACCEPT;
            } else {
                state->actionState = MC_ACTION_NO_FILE;
                result->effects |= MC_READY_EFFECT_INVALID;
            }
        }
    } else if (input->freeBlocks != 0) {
        state->prompt = MC_PROMPT_SELECT_SAVE;
        if (confirm) {
            state->confirmChoice = 0;
            if (slotUsed) {
                state->actionState = MC_ACTION_CONFIRM_OVERWRITE;
            } else {
                state->timer = 0x1E;
                state->actionState = MC_ACTION_SAVE_PREPARE;
            }
            result->effects |= MC_READY_EFFECT_ACCEPT;
        }
    } else if (hasAnySave) {
        state->prompt = MC_PROMPT_SELECT_SAVE;
        if (confirm) {
            state->confirmChoice = 0;
            state->actionState = slotUsed
                ? MC_ACTION_CONFIRM_OVERWRITE : MC_ACTION_CARD_FULL;
            result->effects |= MC_READY_EFFECT_ACCEPT;
        }
    } else {
        state->prompt = MC_PROMPT_CARD_FULL;
        if (confirm) {
            MemoryCardReadyReturnToMode(state);
            result->effects |= MC_READY_EFFECT_INVALID;
        }
    }

    if (cancel) {
        state->page = MC_PAGE_MODE_SELECT;
        result->effects |= MC_READY_EFFECT_BACK;
    }
}

MemoryCardReadyResult MemoryCardReadyReduce(
    MemoryCardReadySession *state, const MemoryCardReadyInput *input) {
    MemoryCardReadyResult result = {MC_READY_EFFECT_NONE};
    const u8 confirm = (input->pressed & PAD_CONFIRM) != 0;
    const u8 cancel = (input->pressed & PAD_CANCEL) != 0;

    if (state->page == MC_PAGE_MODE_SELECT) {
        MemoryCardReadyReduceMode(state, input, &result);
        return result;
    }

    switch (state->actionState) {
    case MC_ACTION_IDLE:
        MemoryCardReadyReduceSlotIdle(state, input, &result);
        break;
    case MC_ACTION_CONFIRM_OVERWRITE: {
        MemoryCardCursorResult choice = MemoryCardSetBinaryChoice(
            state->confirmChoice, input->pressedRepeat);
        state->confirmChoice = choice.value;
        state->prompt = state->slotCursor * 2 + state->confirmChoice + 9;
        if (choice.moved) result.effects |= MC_READY_EFFECT_MOVE;
        if (confirm) {
            state->actionState = state->confirmChoice != 0
                ? MC_ACTION_SAVE_PREPARE : MC_ACTION_IDLE;
            result.effects |= MC_READY_EFFECT_ACCEPT;
        } else if (cancel) {
            state->actionState = MC_ACTION_IDLE;
            result.effects |= MC_READY_EFFECT_BACK;
        }
        break;
    }
    case MC_ACTION_CARD_FULL:
        state->prompt = MC_PROMPT_CARD_FULL;
        if (confirm || cancel) {
            MemoryCardReadyReturnToMode(state);
            result.effects |= confirm
                ? MC_READY_EFFECT_ACCEPT : MC_READY_EFFECT_BACK;
        }
        break;
    case MC_ACTION_NO_FILE:
        state->prompt = MC_PROMPT_NO_FILE;
        if (confirm || cancel) {
            MemoryCardReadyReturnToMode(state);
            result.effects |= confirm
                ? MC_READY_EFFECT_ACCEPT : MC_READY_EFFECT_BACK;
        }
        break;
    default:
        break;
    }
    return result;
}

static void MemoryCardActionTick(MemoryCardActionSession *state,
                                 s32 finalRowCursor) {
    switch (state->state) {
    case MC_ACTION_SAVE_PREPARE:
        state->prompt = MC_PROMPT_ACCESSING;
        state->timer = 0xA;
        state->state = MC_ACTION_SAVE_DELAY;
        break;
    case MC_ACTION_SAVE_DELAY:
        state->busy = 1;
        if (state->timer > 0) state->timer--;
        if (state->timer == 0) state->state = MC_ACTION_SAVE_WRITE;
        break;
    case MC_ACTION_SAVE_POST_WRITE:
        state->state = MC_ACTION_SAVE_REFRESH;
        break;
    case MC_ACTION_SAVE_SETTLE_PREPARE:
        state->timer = 5;
        state->state = MC_ACTION_SAVE_SETTLE_DELAY;
        break;
    case MC_ACTION_SAVE_SETTLE_DELAY:
        if (state->timer > 0) state->timer--;
        if (state->timer == 0) {
            state->settleTicks = 0;
            state->state = MC_ACTION_SAVE_WAIT_CARD;
        }
        break;
    case MC_ACTION_SAVE_RESULT_DELAY:
    case MC_ACTION_LOAD_RESULT_DELAY:
        if (state->timer > 0) state->timer--;
        if (state->timer == 0) {
            state->menuPage = 0;
            state->menuRowCursor = finalRowCursor;
            state->state = MC_ACTION_IDLE;
        }
        break;
    case MC_ACTION_LOAD_INITIAL_DELAY:
        if (state->timer > 0) state->timer--;
        if (state->timer == 0)
            state->state = MC_ACTION_LOAD_ACCESS_PREPARE;
        break;
    case MC_ACTION_LOAD_PREPARE:
        state->timer = 5;
        state->state = MC_ACTION_LOAD_INITIAL_DELAY;
        break;
    case MC_ACTION_LOAD_ACCESS_PREPARE:
        state->prompt = MC_PROMPT_ACCESSING;
        state->timer = 0xF;
        state->busy = 1;
        state->state = MC_ACTION_LOAD_ACCESS_DELAY;
        break;
    case MC_ACTION_LOAD_ACCESS_DELAY:
        if (state->timer > 0) state->timer--;
        if (state->timer == 0) state->state = MC_ACTION_LOAD_READ;
        break;
    case MC_ACTION_LOAD_SETTLE_DELAY:
        if (state->timer > 0) state->timer--;
        if (state->timer == 0) {
            state->settleTicks = 0;
            state->state = MC_ACTION_LOAD_WAIT_CARD;
        }
        break;
    case MC_ACTION_LOAD_SETTLE_PREPARE:
        state->timer = 5;
        state->state = MC_ACTION_LOAD_SETTLE_DELAY;
        break;
    default:
        break;
    }
}

static void MemoryCardActionCompleteEffect(MemoryCardActionSession *state) {
    switch (state->state) {
    case MC_ACTION_SAVE_WRITE:
        state->state = MC_ACTION_SAVE_POST_WRITE;
        break;
    case MC_ACTION_SAVE_REFRESH:
        state->state = MC_ACTION_SAVE_SETTLE_PREPARE;
        break;
    case MC_ACTION_LOAD_READ:
        state->state = MC_ACTION_LOAD_SETTLE_PREPARE;
        break;
    default:
        break;
    }
}

static void MemoryCardActionObserveCard(MemoryCardActionSession *state,
                                        s32 cardStatus) {
    if (state->state != MC_ACTION_SAVE_WAIT_CARD &&
        state->state != MC_ACTION_LOAD_WAIT_CARD) {
        return;
    }
    if (cardStatus != 1) return;
    state->settleTicks++;
    if (state->settleTicks < 4) return;
    state->state = state->state == MC_ACTION_SAVE_WAIT_CARD
        ? MC_ACTION_SAVE_SHOW_RESULT
        : MC_ACTION_LOAD_SHOW_RESULT;
}

static void MemoryCardActionShowResult(MemoryCardActionSession *state,
                                       s32 saveOperation, s32 succeeded) {
    state->prompt = succeeded != 0
        ? (saveOperation != 0
            ? MC_RESULT_PROMPT_SAVE_OK
            : MC_RESULT_PROMPT_LOAD_OK)
        : MC_RESULT_PROMPT_CARD_ERROR;
    state->timer = 0x3C;
    state->busy = 0;
    state->state = saveOperation != 0
        ? MC_ACTION_SAVE_RESULT_DELAY
        : MC_ACTION_LOAD_RESULT_DELAY;
}

MemoryCardActionResult MemoryCardActionReduce(
    MemoryCardActionSession *state, const MemoryCardActionEvent *event) {
    MemoryCardActionResult result;
    MemoryCardActionState previous = state->state;

    result.effect = MemoryCardActionRequestedEffect(state);

    switch (event->type) {
    case MC_ACTION_EVENT_TICK:
        MemoryCardActionTick(state, event->finalRowCursor);
        break;
    case MC_ACTION_EVENT_CARD_STATUS:
        MemoryCardActionObserveCard(state, event->value);
        break;
    case MC_ACTION_EVENT_IO_RESULT:
        MemoryCardActionShowResult(
            state, event->saveOperation, event->value);
        break;
    case MC_ACTION_EVENT_EFFECT_COMPLETE:
        MemoryCardActionCompleteEffect(state);
        break;
    }
    result.stateChanged = state->state != previous;
    return result;
}

MemoryCardActionEffect MemoryCardActionRequestedEffect(
    const MemoryCardActionSession *state) {
    switch (state->state) {
    case MC_ACTION_SAVE_WRITE:
        return MC_ACTION_EFFECT_WRITE_SLOT;
    case MC_ACTION_SAVE_REFRESH:
        return MC_ACTION_EFFECT_REFRESH_SLOTS;
    case MC_ACTION_LOAD_READ:
        return MC_ACTION_EFFECT_LOAD_SLOT;
    case MC_ACTION_SAVE_WAIT_CARD:
    case MC_ACTION_LOAD_WAIT_CARD:
        return MC_ACTION_EFFECT_POLL_CARD;
    default:
        return MC_ACTION_EFFECT_NONE;
    }
}

MemoryCardCursorResult MemoryCardMoveMenuRow(
    s32 value, s32 minimum, s32 maximum, u16 pressedRepeat) {
    MemoryCardCursorResult result;

    result.value = value;
    result.moved = 0;
    if ((pressedRepeat & PAD_DOWN) != 0) {
        if (result.value < maximum) {
            result.value++;
            result.moved = 1;
        }
    } else if ((pressedRepeat & PAD_UP) != 0) {
        if (result.value > minimum) {
            result.value--;
            result.moved = 1;
        }
    }
    return result;
}

MemoryCardCursorResult MemoryCardSetBinaryChoice(
    s32 value, u16 pressedRepeat) {
    MemoryCardCursorResult result;

    result.value = value;
    result.moved = 0;
    if ((pressedRepeat & PAD_LEFT) != 0) {
        result.value = 1;
    } else if ((pressedRepeat & PAD_RIGHT) != 0) {
        result.value = 0;
    }
    result.moved = result.value != value;
    return result;
}
