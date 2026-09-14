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
void RunCardWorkingActions(MemoryCardAction *action, MemoryCardSlots *slots,
                           s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    switch (action->state) {
    case CARD_WORK_WAIT_FOR_SCENE:
        if ((u32)g_SceneTimer < CARD_WORK_START_FRAME) break;
        g_McSettleTicks = 0;
        action->timer = CARD_WORK_CANCEL_DELAY_FRAMES;
        action->state = CARD_WORK_WAIT_FOR_CARD;
        break;
    case CARD_WORK_WAIT_FOR_CARD:
        action->busy = 0;
        if (action->timer > 0) action->timer--;
        if ((g_PadPressed & PAD_CANCEL) &&
            action->timer == 0) {
            g_McSettleTicks = 0;
            if (fadeBusy == 0) {
                PlaySoundCue(3);
                StartMenuExitFade();
            }
        }
        if (g_McCardStatus != MC_MENU_STATE_READY) break;
        g_McSettleTicks++;
        if (g_McSettleTicks < CARD_WORK_READY_FRAMES) break;
        g_McSettleTicks = 0;
        action->state = CARD_WORK_BEGIN_STATUS_DELAY;
        break;
    case CARD_WORK_BEGIN_STATUS_DELAY:
        action->busy = 1;
        action->timer = CARD_WORK_DELAY_FRAMES;
        action->state = CARD_WORK_WAIT_STATUS_DELAY;
        break;
    case CARD_WORK_WAIT_STATUS_DELAY:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        action->state = CARD_WORK_REFRESH_STATUS;
        break;
    case CARD_WORK_REFRESH_STATUS:
        slots->usedMask = RefreshMemoryCardSaveStatus(slots->headers);
        action->state = CARD_WORK_BEGIN_SETTLE_DELAY;
        break;
    case CARD_WORK_BEGIN_SETTLE_DELAY:
        action->timer = CARD_WORK_DELAY_FRAMES;
        action->state = CARD_WORK_WAIT_SETTLE_DELAY;
        break;
    case CARD_WORK_WAIT_SETTLE_DELAY:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        action->timer = CARD_WORK_DELAY_FRAMES;
        action->busy = 0;
        action->state = CARD_WORK_WAIT_FINAL_DELAY;
        break;
    case CARD_WORK_WAIT_FINAL_DELAY:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        action->state = CARD_WORK_RETURN_READY;
        break;
    case CARD_WORK_RETURN_READY:
        if (g_McMenuSelection != MC_MENU_STATE_READY) break;
        g_McMenuState = g_McMenuSelection;
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

static void ExitNoCardMenu(MemoryCardAction *action, s32 soundCue,
                           int resetAction) {
    if (resetAction) {
        action->state = NO_CARD_ACTION_INIT;
    }
    PlaySoundCue(soundCue);
    StartMenuExitFade();
}

static void RunNoCardRootPage(MemoryCardAction *action, s32 fadeBusy) {
    AdjustMenuSelectionVertical(&g_McMenuRowCursor, 0,
                                MemoryCardMenuRowCount() - 1);

    if (PollMenuConfirmInput() != 0) {
        if (g_McMenuRowCursor != MemoryCardMenuRowCount() - 1) {
            PlaySoundCue(5);
        } else if (fadeBusy == 0) {
            ExitNoCardMenu(action, 2, 1);
        }
        return;
    }

    if ((g_PadPressed & PAD_CANCEL) != 0 && fadeBusy == 0) {
        ExitNoCardMenu(action, 3, 1);
    }
}

static void RunNoCardEmptyPage(MemoryCardAction *action, s32 fadeBusy) {
    if ((g_PadPressed & PAD_CANCEL) != 0 && fadeBusy == 0) {
        ExitNoCardMenu(action, 3, 0);
    }
}

static void RunNoCardReadyState(MemoryCardAction *action, s32 fadeBusy) {
    switch (g_McMenuPage) {
    case 0:
        RunNoCardRootPage(action, fadeBusy);
        break;
    case 1:
        /* This page has no rows and deliberately preserves the action state. */
        RunNoCardEmptyPage(action, fadeBusy);
        break;
    default:
        break;
    }
}

void RunNoCardActions(MemoryCardAction *action, MemoryCardSlots *slots,
                      s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_NO_CARD;
    action->busy = 0;
    switch (action->state) {
    case NO_CARD_ACTION_INIT:
        action->timer = NO_CARD_READY_DELAY_FRAMES;
        slots->usedMask = 0;
        ClearSaveHeaderRows(slots->headers);
        slots->lastSlot = 0;
        action->state = NO_CARD_ACTION_WAIT;
        break;

    case NO_CARD_ACTION_WAIT:
        if (MemoryCardCountdownElapsed(&action->timer)) {
            action->state = NO_CARD_ACTION_READY;
        }
        break;

    case NO_CARD_ACTION_READY:
        RunNoCardReadyState(action, fadeBusy);
        break;

    default:
        break;
    }
}
