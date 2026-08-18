#include "game/memory_card_controller.h"

s32 MemoryCardControllerShouldPoll(s32 actionBusy, s32 errorPending) {
    return actionBusy == 0 || errorPending != 0;
}

void MemoryCardControllerApplyStatus(MemoryCardControllerState *state,
                                     s32 cardStatus) {
    s32 subState;
    state->cardStatus = cardStatus;
    if (cardStatus == 0) {
        s32 previousTicks = state->noCardTicks++;
        if (previousTicks >= 6) state->selection = 3;
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

void MemoryCardControllerResolveDetection(MemoryCardControllerState *state) {
    switch (state->selection) {
    case 1:
        if (state->cardStatus == 1)
            state->menuState = state->lastMenuState != 2 ? 2 : 1;
        break;
    case 2:
        state->menuState = 2;
        break;
    case -1:
    case -2:
        state->menuState = state->selection;
        break;
    case 3:
        break;
    default:
        if (state->cardStatus == -3) {
            s32 previousTicks = state->errorTicks++;
            if (previousTicks >= 4) state->menuState = -3;
        }
        break;
    }
    if (state->menuState != 3) state->errorTicks = 0;
}

static void ResolveCardError(MemoryCardControllerState *state) {
    state->errorPending = 1;
    if (state->cardStatus == -3) {
        state->errorCountdown--;
        if (state->errorCountdown == 0) state->menuState = -3;
    }
}

static void ClearPendingError(MemoryCardControllerState *state) {
    if (state->errorPending != 0) {
        state->errorPending = 0;
        state->errorCountdown = 3;
    }
}

void MemoryCardControllerResolveTransition(MemoryCardControllerState *state) {
    s32 current = state->menuState;

    if (state->selection == 3) {
        state->lastMenuState = current;
        state->menuState = 3;
        return;
    }

    switch (current) {
    case 1:
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
    case 2:
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
    case -1:
        switch (state->selection) {
        case 1:
        case 2:
            state->menuState = 2;
            ClearPendingError(state);
            break;
        case -1:
            ClearPendingError(state);
            break;
        case -2:
            state->menuState = -2;
            break;
        case 3:
            break;
        default:
            ResolveCardError(state);
            break;
        }
        break;
    case -2:
        switch (state->selection) {
        case 1:
        case 2:
            state->menuState = 2;
            break;
        case -1:
            state->menuState = -1;
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
