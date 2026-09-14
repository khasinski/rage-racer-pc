#ifndef GAME_MEMCARD_H
#define GAME_MEMCARD_H

#include "common.h"

#include "game/memcard_types.h"
#include "game/memcard_state.h"
#include "game/menu_types.h"
#include "game/save_format.h"

#include "psyq/gpu.h"
#include "psyq/kernel.h"

/* Memory-card prompts use the retail strings reached through g_McMessageRows. */
enum {
    MEMORY_CARD_MESSAGE_COUNT = MC_PROMPT_NO_FILE,
    MEMORY_CARD_MESSAGE_COLUMN_COUNT = 5,
    /* Both retail tables leave two zero bytes after their three fixed-width
     * rows. Keep that tail explicit because the symbols are ABI fixtures. */
    MEMORY_CARD_SAVE_TABLE_PADDING = 2,
    MEMORY_CARD_SAVE_PATH_STORAGE_SIZE =
        MEMORY_CARD_SAVE_SLOT_COUNT * MC_SAVE_PATH_SIZE +
        MEMORY_CARD_SAVE_TABLE_PADDING,
    MEMORY_CARD_SAVE_TITLE_STORAGE_SIZE =
        MEMORY_CARD_SAVE_SLOT_COUNT * MC_SAVE_TITLE_SIZE +
        MEMORY_CARD_SAVE_TABLE_PADDING,
};


typedef struct MemoryCardMessageRow {
    char *text;
    u8 column;
    u8 reserved[3];
} MemoryCardMessageRow;

void AdvanceSaveHeaderCounter(void);
void RestartMemoryCard(void);

/*
 * Memory card BIOS front end. These were labelled Cd until the event classes
 * were decoded: every one operates on SwCARD/HwCARD, never on the drive.
 */
typedef enum MemoryCardEvent {
    MC_EVENT_INVALID = -1,
    MC_EVENT_NONE,
    MC_EVENT_IO_COMPLETE,
    MC_EVENT_ERROR,
    MC_EVENT_TIMEOUT,
    MC_EVENT_NEW_CARD
} MemoryCardEvent;

typedef enum MemoryCardResult {
    MC_CARD_RESULT_ERROR = -3,
    MC_CARD_RESULT_UNFORMATTED = -2,
    MC_CARD_RESULT_NO_CARD = -1,
    MC_CARD_RESULT_PENDING = 0,
    MC_CARD_RESULT_READY = 1,
    MC_CARD_RESULT_NEW_CARD = 2,
} MemoryCardResult;
void EnterMemoryCardMenu(void);
void EnterMemoryCardMenuFromLoad(void);
void UpdateMemoryCardMenu(void);

extern s16 g_McMessageColumnX[MEMORY_CARD_MESSAGE_COLUMN_COUNT];
extern MemoryCardMessageRow *g_McMessageRows[MEMORY_CARD_MESSAGE_COUNT];
extern s32 g_SaveElapsedTicks;
extern char g_SaveFilePath[MEMORY_CARD_SAVE_PATH_STORAGE_SIZE];
enum {
    SAVE_NAME_CHARACTER_COUNT = 42,
    SAVE_NAME_CHARSET_STORAGE_SIZE = 44,
};
extern char g_SaveTitleSjis[MEMORY_CARD_SAVE_TITLE_STORAGE_SIZE];

#endif
