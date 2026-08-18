#ifndef GAME_MEMORY_CARD_CONTROLLER_H
#define GAME_MEMORY_CARD_CONTROLLER_H

#include "common.h"

typedef struct MemoryCardControllerState {
    s32 menuState;
    s32 selection;
    s32 subState;
    s32 cardStatus;
    s32 noCardTicks;
    s32 errorTicks;
    s32 lastMenuState;
    s32 errorPending;
    s32 errorCountdown;
} MemoryCardControllerState;

s32 MemoryCardControllerShouldPoll(s32 actionBusy, s32 errorPending);
void MemoryCardControllerApplyStatus(MemoryCardControllerState *state,
                                     s32 cardStatus);
void MemoryCardControllerResolveDetection(MemoryCardControllerState *state);
void MemoryCardControllerResolveTransition(MemoryCardControllerState *state);

#endif
