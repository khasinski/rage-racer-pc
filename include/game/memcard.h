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

extern MemoryCardPrompt g_McMenuPhase;

typedef struct MemoryCardMessageRow {
    char *text;
    u8 column;
    u8 reserved[3];
} MemoryCardMessageRow;

void AdvanceSaveHeaderCounter(void);
void RestartMemoryCard(void);
void ClearSaveHeaderRows(GameSaveHeaderRow *rows);
void BuildSaveIconBlock(
    GameSaveIconBlock *block,
    const char *title,
    s32 iconTile,
    s32 imageX,
    s32 imageY);
void WriteSaveHeaderRow(GameSaveHeaderRow *row);
s32 LoadSaveStateBlock(const GameSaveBlock *block);
s32 WriteMemoryCardSaveFile(
    char *path,
    char *title,
    GameSaveIconBlock *iconBlock,
    GameSaveHeaderRow *header,
    GameSaveBlock *saveBlock);
s32 WriteMemoryCardSaveSlot(
    s32 slot,
    GameSaveHeaderRow *header);
s32 ReadVerifiedSaveHeader(
    s32 fd,
    GameSaveHeaderRow *header);
s32 ScanMemoryCardSaveHeaders(GameSaveHeaderRow *headers);
s32 LoadMemoryCardSaveSlot(
    s32 slot,
    GameSaveHeaderRow *header);
s32 CountMemoryCardFiles(s32 device, s32 port);
s32 CalculateMemoryCardFreeBlocks(s32 fileCount);
s32 RefreshMemoryCardSaveStatus(GameSaveHeaderRow *headers);
char *FormatSaveElapsedTime(char *dst, u32 ticks);
void DrawMemoryCardSaveRows(
    s32 flags,
    GameSaveHeaderRow *rows);

/*
 * Memory card BIOS front end. These were labelled Cd until the event classes
 * were decoded: every one operates on SwCARD/HwCARD, never on the drive.
 */
void ClearMemoryCardHwEvents(void);
void ClearMemoryCardSwEvents(void);
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
MemoryCardEvent WaitMemoryCardHwEvent(void);
MemoryCardEvent WaitMemoryCardSwEvent(void);
MemoryCardEvent PollMemoryCardHwEvent(void);
void OpenMemoryCardEvents(void);
void EnableMemoryCardEvents(void);
void DisableMemoryCardEvents(void);
void CloseMemoryCardEvents(void);
/* libcard _card_clear (see psyq/): _new_card() + _card_write(chan, 0x3F, 0). */
s32 FormatMemoryCard(s32 port, s32 slot);

/* Moved here from menu.h and audio.h: these belong to the card, not to
 * the menu or the mixer. */
void StartMemoryCardEvents(void);
void StopMemoryCardEvents(void);
void EnterMemoryCardMenu(void);
void EnterMemoryCardMenuFromLoad(void);
void UpdateMemoryCardMenu(void);

/* Declared identically by 20 translation units before this
 * header carried them. */

extern s32 g_McFadeLevel;
extern s32 g_McFadeStep;
extern s32 g_McFreeBlocks;
extern s32 g_McFromLoadMenu;
extern s32 g_McMenuPage;
extern s32 g_McMenuRowCount;
extern s32 g_McMenuRowCursor;

/* Declared identically by 64 translation units before this
 * header carried them. */

extern char g_FmtCardDevice[];
extern char g_FmtCardWildcard[];
extern char g_FmtPlayTime[];
extern char g_FmtSaveChecksum[];
extern char g_FmtSaveRow[];
extern char g_FmtSaveRowEmpty[];
extern char g_FmtSaveRowTail[];
extern char g_FmtString[];
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
extern u8 g_McHelpText[];
extern s32 g_McHwEventError;
extern s32 g_McHwEventIoe;
extern s32 g_McHwEventNew;
extern s32 g_McHwEventTimeout;
extern s32 g_McLastCardStatus;
extern s32 g_McLastMenuState;
extern s32 g_McLastSlot;
extern s16 g_McMessageColumnX[];
extern MemoryCardMessageRow *g_McMessageRows[];
extern u8 g_McMessageText[];
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
typedef enum MemoryCardStatusState {
    MC_STATUS_INVALID = -1,
    MC_STATUS_REQUEST_INFO,
    MC_STATUS_WAIT_INFO,
    MC_STATUS_REQUEST_LOAD,
    MC_STATUS_WAIT_LOAD,
    MC_STATUS_PUBLISH_RESULT
} MemoryCardStatusState;
extern MemoryCardStatusState g_McStatusState;
extern s32 g_McSwEventError;
extern s32 g_McSwEventIoe;
extern s32 g_McSwEventNew;
extern s32 g_McSwEventTimeout;
extern char g_MsgSaveChecksumOk[];
extern s32 g_SaveElapsedTicks;
extern char g_SaveFilePath[];
enum { SAVE_NAME_CHARSET_SIZE = 44 };
extern u8 g_SaveNameCharset[SAVE_NAME_CHARSET_SIZE];
extern char g_SaveTitleSjis[];

void BiosBuInit(void);
void DrawMemoryCardMessage(s32 message);
s32 PollMemoryCardStatus(s32 a, s32 b);
void StoreSaveStateBlock(GameSaveBlock *block);
void DrawMemoryCardScreen(s32 showSlotBar, s32 fromLoadMenu,
                          s32 selectedRow, s32 selectedSlot);

#endif
