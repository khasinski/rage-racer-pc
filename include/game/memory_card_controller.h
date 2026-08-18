#ifndef GAME_MEMORY_CARD_CONTROLLER_H
#define GAME_MEMORY_CARD_CONTROLLER_H

#include "common.h"

typedef struct SaveSession {
    s32 menuState;
    s32 selection;
    s32 subState;
    s32 cardStatus;
    s32 noCardTicks;
    s32 errorTicks;
    s32 lastMenuState;
    s32 errorPending;
    s32 errorCountdown;
} SaveSession;

s32 MemoryCardControllerShouldPoll(s32 actionBusy, s32 errorPending);
void MemoryCardControllerApplyStatus(SaveSession *state,
                                     s32 cardStatus);
void MemoryCardControllerResolveDetection(SaveSession *state);
void MemoryCardControllerResolveTransition(SaveSession *state);

#endif
