#ifndef GAME_MEMCARD_TYPES_H
#define GAME_MEMCARD_TYPES_H

#include "common.h"

enum {
    MEMORY_CARD_SAVE_SLOT_COUNT = 3,
    MEMORY_CARD_MAX_FILES = 15,
    MEMORY_CARD_BLOCK_COUNT = 15,
    MEMORY_CARD_BLOCK_SIZE = 0x2000,
};

static inline int MemoryCardSaveSlotValid(s32 slot) {
    return (u32)slot < MEMORY_CARD_SAVE_SLOT_COUNT;
}

#endif
