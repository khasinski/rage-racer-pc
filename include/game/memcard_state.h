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
} MemoryCardSession;

#endif
