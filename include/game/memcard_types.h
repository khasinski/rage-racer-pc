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

/* Prompt selected for the message below the memory-card slot list. */
typedef enum MemoryCardPrompt {
    MC_PROMPT_INVALID = -1,
    MC_PROMPT_NONE = 0x00,
    MC_PROMPT_SELECT_SAVE = 0x01,
    MC_PROMPT_SELECT_LOAD = 0x02,
    MC_PROMPT_NO_CARD = 0x03,
    MC_PROMPT_CARD_FULL = 0x04,
    MC_PROMPT_NO_DATA = 0x05,
    MC_PROMPT_NEW_CARD = 0x06,
    MC_PROMPT_FORMAT_ASK = 0x07,
    MC_PROMPT_OVERWRITE_ASK = 0x09,
    MC_PROMPT_ACCESSING = 0x0F,
    MC_PROMPT_CARD_ERROR = 0x10,
    MC_PROMPT_LOAD_OK = 0x11,
    MC_PROMPT_SAVE_OK = 0x12,
    MC_PROMPT_FORMAT_OK = 0x13,
    MC_PROMPT_NO_FILE = 0x14
} MemoryCardPrompt;

typedef enum MemoryCardStatusState {
    MC_STATUS_INVALID = -1,
    MC_STATUS_REQUEST_INFO,
    MC_STATUS_WAIT_INFO,
    MC_STATUS_REQUEST_LOAD,
    MC_STATUS_WAIT_LOAD,
    MC_STATUS_PUBLISH_RESULT
} MemoryCardStatusState;

#endif
