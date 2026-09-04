#ifndef GAME_MEMCARD_H
#define GAME_MEMCARD_H

#include "common.h"

#include "game/memcard_types.h"
#include "game/menu_types.h"
#include "game/save_format.h"

#include "psyq/gpu.h"
#include "psyq/kernel.h"

/*
 * g_McMenuPhase picks the prompt drawn under the slot list; 0 draws none, and
 * DrawMemoryCardMessage is called with the value minus one. The names come
 * from the retail strings the index reaches through g_McMessageRows, quoted here.
 */
enum {
    MEMORY_CARD_MESSAGE_COUNT = MC_PROMPT_NO_FILE,
    MEMORY_CARD_MESSAGE_COLUMN_COUNT = 5,
};

extern MemoryCardPrompt g_McMenuPhase;

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

extern s32 g_McFadeLevel;
extern s32 g_McFadeStep;
extern s32 g_McFreeBlocks;
extern s32 g_McFromLoadMenu;
extern s32 g_McMenuPage;
extern s32 g_McMenuRowCount;
extern s32 g_McMenuRowCursor;

extern char g_FmtCardDevice[];
extern char g_FmtCardWildcard[];
extern char g_FmtPlayTime[];
extern char g_FmtSaveRow[];
extern char g_FmtSaveRowEmpty[];
extern char g_FmtSaveRowTail[];
extern s32 g_McActionBusy;
extern s32 g_McActionElapsed;
extern s32 g_McActionResult;
extern s32 g_McActionState;
extern s32 g_McActionTimer;
extern s32 g_McCardFileCount;
extern s32 g_McCardOkFrames;
extern s32 g_McConfirmChoice;
extern DirEntry g_McDirEntries[];
extern s32 g_McErrorCountdown;
extern s32 g_McErrorPending;
extern s32 g_McErrorTicks;
extern s32 g_McHwEventError;
extern s32 g_McHwEventIoe;
extern s32 g_McHwEventNew;
extern s32 g_McHwEventTimeout;
extern s32 g_McLastCardStatus;
extern s32 g_McLastMenuState;
extern s32 g_McLastSlot;
extern s16 g_McMessageColumnX[MEMORY_CARD_MESSAGE_COLUMN_COUNT];
extern MemoryCardMessageRow *g_McMessageRows[MEMORY_CARD_MESSAGE_COUNT];
extern s32 g_McNoCardTicks;
extern s32 g_McPollStatus;
extern s32 g_McPollTicks;
extern GameSaveHeaderRow g_McSaveHeaders[];
extern s32 g_McSaveMode;
extern s32 g_McSettleTicks;
extern s32 g_McSlotCursor;
extern char g_McSlotLabelError[];
extern char g_McSlotLabelNoFile[];
extern char g_McSlotLabels[];
extern s32 g_McSlotUsedMask;
extern s32 g_McStatusResult;
extern MemoryCardStatusState g_McStatusState;
extern s32 g_McSwEventError;
extern s32 g_McSwEventIoe;
extern s32 g_McSwEventNew;
extern s32 g_McSwEventTimeout;
extern s32 g_SaveElapsedTicks;
extern char g_SaveFilePath[];
enum {
    SAVE_NAME_CHARACTER_COUNT = 42,
    SAVE_NAME_CHARSET_STORAGE_SIZE = 44,
};
extern char g_SaveNameCharset[SAVE_NAME_CHARSET_STORAGE_SIZE];
extern char g_SaveTitleSjis[];

#endif
