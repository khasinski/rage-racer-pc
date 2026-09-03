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

s32 UpdateMemoryCardFade(void);
s32 AdvanceMemoryCardMenuStartup(void);
void DrawMemoryCardMenu(void);
void RunCardSlotActions(void);
void RunUnformattedCardPage(s32 fadeBusy);
void RunCardWorkingActions(s32 fadeBusy);
void RunNoCardActions(s32 fadeBusy);

#endif
