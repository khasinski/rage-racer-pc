#include "game/memory_card_controller.h"
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

void MemoryCardActionTick(MemoryCardActionSession *state,
                          s32 finalRowCursor) {
    if (state->timer > 0) state->timer--;
    if (state->timer != 0) return;

    switch (state->state) {
    case MC_ACTION_SAVE_DELAY:
        state->state = MC_ACTION_SAVE_WRITE;
        break;
    case MC_ACTION_SAVE_SETTLE_DELAY:
        state->settleTicks = 0;
        state->state = MC_ACTION_SAVE_WAIT_CARD;
        break;
    case MC_ACTION_SAVE_RESULT_DELAY:
    case MC_ACTION_LOAD_RESULT_DELAY:
        state->menuPage = 0;
        state->menuRowCursor = finalRowCursor;
        state->state = MC_ACTION_IDLE;
        break;
    case MC_ACTION_LOAD_INITIAL_DELAY:
        state->state = MC_ACTION_LOAD_ACCESS_PREPARE;
        break;
    case MC_ACTION_LOAD_ACCESS_DELAY:
        state->state = MC_ACTION_LOAD_READ;
        break;
    case MC_ACTION_LOAD_SETTLE_DELAY:
        state->settleTicks = 0;
        state->state = MC_ACTION_LOAD_WAIT_CARD;
        break;
    default:
        break;
    }
}

void MemoryCardActionObserveCard(MemoryCardActionSession *state,
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

void MemoryCardActionShowResult(MemoryCardActionSession *state,
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
