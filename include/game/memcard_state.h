#ifndef GAME_MEMCARD_STATE_H
#define GAME_MEMCARD_STATE_H

#include "game/memcard_types.h"
#include "game/save_format.h"

typedef struct MemoryCardSlots {
    GameSaveHeaderRow headers[MEMORY_CARD_SAVE_SLOT_COUNT];
    s32 usedMask;
    s32 lastSlot;
} MemoryCardSlots;

typedef struct MemoryCardSession {
    MemoryCardAction action;
    MemoryCardPoll poll;
    MemoryCardSlots slots;
    MemoryCardPrompt phase;
    s32 cardStatus;
    s32 errorCountdown;
    s32 errorPending;
    s32 errorTicks;
    s32 fadeLevel;
    s32 fadeStep;
    s32 freeBlocks;
    s32 fromLoadMenu;
    s32 lastMenuState;
    s32 menuPage;
    s32 menuRow;
    s32 menuSelection;
    s32 menuState;
    s32 noCardTicks;
    s32 saveMode;
    s32 settleTicks;
    s32 slot;
} MemoryCardSession;

#endif
