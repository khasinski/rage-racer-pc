#ifndef GAME_MEMCARD_INTERNAL_H
#define GAME_MEMCARD_INTERNAL_H

#include "game/memcard.h"

extern s32 g_FrameSyncThreshold;
extern s32 GameMenuLoadPhase;

typedef enum MemoryCardMenuState {
    MC_MENU_STATE_ERROR = -3,
    MC_MENU_STATE_UNFORMATTED = -2,
    MC_MENU_STATE_NO_CARD = -1,
    MC_MENU_STATE_READY = 1,
    MC_MENU_STATE_WORKING = 2,
    MC_MENU_STATE_BUSY = 3,
} MemoryCardMenuState;

/* Advance a positive frame countdown and report its deadline. Invalid or
 * already elapsed values finish immediately instead of counting away from
 * zero or overflowing at INT_MIN. */
static inline int MemoryCardCountdownElapsed(s32 *frames) {
    if (*frames > 1) {
        (*frames)--;
        return 0;
    }
    *frames = 0;
    return 1;
}

void ClearSaveHeaderRows(GameSaveHeaderRow *rows);
s32 WriteMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *header);
s32 ScanMemoryCardSaveHeaders(GameSaveHeaderRow *headers);
s32 LoadMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *header);
s32 CountMemoryCardFiles(s32 device, s32 port);
s32 CalculateMemoryCardFreeBlocks(s32 fileCount);
s32 RefreshMemoryCardSaveStatus(GameSaveHeaderRow *headers);
enum { SAVE_ELAPSED_TIME_CAPACITY = 16 };
char *FormatSaveElapsedTime(char dst[SAVE_ELAPSED_TIME_CAPACITY], u32 ticks);
void DrawMemoryCardSaveRows(s32 flags, GameSaveHeaderRow *rows);

void ClearMemoryCardHwEvents(void);
void ClearMemoryCardSwEvents(void);
MemoryCardEvent WaitMemoryCardSwEvent(void);
MemoryCardEvent PollMemoryCardHwEvent(void);
/* libcard _card_clear (see psyq/): _new_card() + _card_write(chan, 0x3F, 0). */
s32 FormatMemoryCard(s32 port, s32 slot);
void StartMemoryCardEvents(void);
void StopMemoryCardEvents(void);

void AdjustMenuSelectionVertical(s32 *value, s32 min, s32 max);
void SetMenuBinaryChoiceHorizontal(s32 *value);
u16 PollMenuConfirmInput(void);
u16 PollMenuBackInput(void);
void DrawMenuFadeOverlay(s32 brightness);
void StartMenuExitFade(void);
s32 UpdateMemoryCardFade(void);
s32 AdvanceMemoryCardMenuStartup(void);
void DrawMemoryCardMenu(void);
void RunCardSlotActions(void);
void RunUnformattedCardPage(s32 fadeBusy);
void RunCardWorkingActions(s32 fadeBusy);
void RunNoCardActions(s32 fadeBusy);
void DrawMemoryCardMessage(s32 message);
s32 PollMemoryCardStatus(s32 port, s32 slot);
void DrawMemoryCardScreen(s32 showSlotBar, s32 fromLoadMenu,
                          s32 selectedRow, s32 selectedSlot);

#endif
