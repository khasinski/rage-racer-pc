#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

typedef enum CardWorkingActionState {
    CARD_WORK_WAIT_FOR_SCENE = 0,
    CARD_WORK_WAIT_FOR_CARD = 1,
    CARD_WORK_BEGIN_STATUS_DELAY = 2,
    CARD_WORK_WAIT_STATUS_DELAY = 3,
    CARD_WORK_REFRESH_STATUS = 5,
    CARD_WORK_BEGIN_SETTLE_DELAY = 6,
    CARD_WORK_WAIT_SETTLE_DELAY = 7,
    CARD_WORK_WAIT_FINAL_DELAY = 8,
    CARD_WORK_RETURN_READY = 9,
} CardWorkingActionState;

enum {
    CARD_WORK_START_FRAME = 31,
    CARD_WORK_CANCEL_DELAY_FRAMES = 121,
    CARD_WORK_READY_FRAMES = 2,
    CARD_WORK_DELAY_FRAMES = 5,
};

/*
 * A format or a save running, stepping through its own stages while the
 * screen says it is busy.
 */
void RunCardWorkingActions(MemoryCardSession *memoryCard, s32 fadeBusy) {
    memoryCard->phase = MC_PROMPT_ACCESSING;
    switch (memoryCard->action.state) {
    case CARD_WORK_WAIT_FOR_SCENE:
        if ((u32)g_SceneTimer < CARD_WORK_START_FRAME) break;
        memoryCard->settleTicks = 0;
        memoryCard->action.timer = CARD_WORK_CANCEL_DELAY_FRAMES;
        memoryCard->action.state = CARD_WORK_WAIT_FOR_CARD;
        break;
    case CARD_WORK_WAIT_FOR_CARD:
        memoryCard->action.busy = 0;
        if (memoryCard->action.timer > 0) memoryCard->action.timer--;
        if ((g_PadPressed & PAD_CANCEL) &&
            memoryCard->action.timer == 0) {
            memoryCard->settleTicks = 0;
            if (fadeBusy == 0) {
                PlaySoundCue(3);
                StartMenuExitFade(memoryCard);
            }
        }
        if (memoryCard->cardStatus != MC_MENU_STATE_READY) break;
        memoryCard->settleTicks++;
        if (memoryCard->settleTicks < CARD_WORK_READY_FRAMES) break;
        memoryCard->settleTicks = 0;
        memoryCard->action.state = CARD_WORK_BEGIN_STATUS_DELAY;
        break;
    case CARD_WORK_BEGIN_STATUS_DELAY:
        memoryCard->action.busy = 1;
        memoryCard->action.timer = CARD_WORK_DELAY_FRAMES;
        memoryCard->action.state = CARD_WORK_WAIT_STATUS_DELAY;
        break;
    case CARD_WORK_WAIT_STATUS_DELAY:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->action.state = CARD_WORK_REFRESH_STATUS;
        break;
    case CARD_WORK_REFRESH_STATUS:
        memoryCard->slots.usedMask = RefreshMemoryCardSaveStatus(
            memoryCard->slots.headers, &memoryCard->freeBlocks);
        memoryCard->action.state = CARD_WORK_BEGIN_SETTLE_DELAY;
        break;
    case CARD_WORK_BEGIN_SETTLE_DELAY:
        memoryCard->action.timer = CARD_WORK_DELAY_FRAMES;
        memoryCard->action.state = CARD_WORK_WAIT_SETTLE_DELAY;
        break;
    case CARD_WORK_WAIT_SETTLE_DELAY:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->action.timer = CARD_WORK_DELAY_FRAMES;
        memoryCard->action.busy = 0;
        memoryCard->action.state = CARD_WORK_WAIT_FINAL_DELAY;
        break;
    case CARD_WORK_WAIT_FINAL_DELAY:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->action.state = CARD_WORK_RETURN_READY;
        break;
    case CARD_WORK_RETURN_READY:
        if (memoryCard->menuSelection != MC_MENU_STATE_READY) break;
        memoryCard->menuState = memoryCard->menuSelection;
        break;
    default:
        break;
    }
}

/*
 * Nothing in the slot.
 */
typedef enum NoCardActionState {
    NO_CARD_ACTION_INIT = 0,
    NO_CARD_ACTION_WAIT = 1,
    NO_CARD_ACTION_READY = 3,
} NoCardActionState;

enum { NO_CARD_READY_DELAY_FRAMES = 5 };

static void ExitNoCardMenu(MemoryCardSession *memoryCard, s32 soundCue,
                           int resetAction) {
    if (resetAction) {
        memoryCard->action.state = NO_CARD_ACTION_INIT;
    }
    PlaySoundCue(soundCue);
    StartMenuExitFade(memoryCard);
}

static void RunNoCardRootPage(MemoryCardSession *memoryCard, s32 fadeBusy) {
    AdjustMenuSelectionVertical(&memoryCard->menuRow, 0,
                                MemoryCardMenuRowCount(memoryCard) - 1);

    if (PollMenuConfirmInput() != 0) {
        if (memoryCard->menuRow != MemoryCardMenuRowCount(memoryCard) - 1) {
            PlaySoundCue(5);
        } else if (fadeBusy == 0) {
            ExitNoCardMenu(memoryCard, 2, 1);
        }
        return;
    }

    if ((g_PadPressed & PAD_CANCEL) != 0 && fadeBusy == 0) {
        ExitNoCardMenu(memoryCard, 3, 1);
    }
}

static void RunNoCardEmptyPage(MemoryCardSession *memoryCard, s32 fadeBusy) {
    if ((g_PadPressed & PAD_CANCEL) != 0 && fadeBusy == 0) {
        ExitNoCardMenu(memoryCard, 3, 0);
    }
}

static void RunNoCardReadyState(MemoryCardSession *memoryCard, s32 fadeBusy) {
    switch (memoryCard->menuPage) {
    case 0:
        RunNoCardRootPage(memoryCard, fadeBusy);
        break;
    case 1:
        /* This page has no rows and deliberately preserves the action state. */
        RunNoCardEmptyPage(memoryCard, fadeBusy);
        break;
    default:
        break;
    }
}

void RunNoCardActions(MemoryCardSession *memoryCard, s32 fadeBusy) {
    memoryCard->phase = MC_PROMPT_NO_CARD;
    memoryCard->action.busy = 0;
    switch (memoryCard->action.state) {
    case NO_CARD_ACTION_INIT:
        memoryCard->action.timer = NO_CARD_READY_DELAY_FRAMES;
        memoryCard->slots.usedMask = 0;
        ClearSaveHeaderRows(memoryCard->slots.headers);
        memoryCard->slots.lastSlot = 0;
        memoryCard->action.state = NO_CARD_ACTION_WAIT;
        break;

    case NO_CARD_ACTION_WAIT:
        if (MemoryCardCountdownElapsed(&memoryCard->action.timer)) {
            memoryCard->action.state = NO_CARD_ACTION_READY;
        }
        break;

    case NO_CARD_ACTION_READY:
        RunNoCardReadyState(memoryCard, fadeBusy);
        break;

    default:
        break;
    }
}
