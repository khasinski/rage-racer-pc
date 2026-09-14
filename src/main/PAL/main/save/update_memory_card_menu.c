#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"
#include "game/scene_runtime.h"

enum {
    BUSY_ERROR_DEBOUNCE_FRAMES = 5,
    CARD_ERROR_COUNTDOWN_FRAMES = 3,
    NO_CARD_DEBOUNCE_FRAMES = 7,
};

/*
 * The card is mid-operation. Nothing to choose here; the cancel button
 * is the only way out, and only once the fade has finished.
 */
static void RunCardBusyState(MemoryCardSession *memoryCard, s32 fadeBusy) {
    memoryCard->phase = MC_PROMPT_ACCESSING;
    memoryCard->action.busy = 0;
    if ((g_PadPressed & PAD_CANCEL) && !fadeBusy) {
        PlaySoundCue(3);
        StartMenuExitFade(memoryCard);
    }
    switch (memoryCard->menuSelection) {
    case MC_MENU_STATE_READY:
        if (memoryCard->cardStatus == MC_MENU_STATE_READY) {
            memoryCard->menuState = memoryCard->lastMenuState != MC_MENU_STATE_WORKING
                                ? MC_MENU_STATE_WORKING
                                : memoryCard->cardStatus;
        }
        break;
    case MC_MENU_STATE_WORKING:
        memoryCard->menuState = MC_MENU_STATE_WORKING;
        break;
    case MC_MENU_STATE_NO_CARD:
    case MC_MENU_STATE_UNFORMATTED:
        memoryCard->menuState = memoryCard->menuSelection;
        break;
    case MC_MENU_STATE_BUSY:
        break;
    case MC_MENU_STATE_ERROR:
    default:
        if (memoryCard->cardStatus == MC_MENU_STATE_ERROR) {
            if (++memoryCard->errorTicks >= BUSY_ERROR_DEBOUNCE_FRAMES) {
                memoryCard->menuState = memoryCard->cardStatus;
            }
        }
        break;
    }
    if (memoryCard->menuState != MC_MENU_STATE_BUSY) {
        memoryCard->errorTicks = 0;
    }
}

/*
 * The list of things the player can do with a readable card. The last row
 * is the way out.
 */
static void RunCardMenuRows(MemoryCardSession *memoryCard, s32 fadeBusy) {
    u16 pad;

    memoryCard->phase = MC_PROMPT_NONE;
    AdjustMenuSelectionVertical(&memoryCard->menuRow, 0,
                                MemoryCardMenuRowCount(memoryCard) - 1);
    pad = g_PadPressed;
    if (pad & PAD_CONFIRM) {
        if (memoryCard->menuRow < MemoryCardMenuRowCount(memoryCard) - 1) {
            PlaySoundCue(2);
            memoryCard->menuPage = 1;
            memoryCard->action.state = 0;
            memoryCard->action.result = 0;
            memoryCard->slot = memoryCard->slots.lastSlot;
            memoryCard->saveMode = memoryCard->menuRow;
            return;
        }
        if (fadeBusy) return;
        PlaySoundCue(2);
    } else {
        if ((pad & PAD_CANCEL) == 0 || fadeBusy) return;
        PlaySoundCue(3);
    }
    memoryCard->action.busy = 0;
    StartMenuExitFade(memoryCard);
}

static void ResetCardAction(MemoryCardSession *memoryCard) {
    memoryCard->action.state = 0;
    memoryCard->action.result = 0;
    memoryCard->action.confirmChoice = 0;
    memoryCard->action.busy = 0;
}

static void ClearPendingCardError(MemoryCardSession *memoryCard) {
    if (memoryCard->errorPending == 0) return;
    memoryCard->errorPending = 0;
    memoryCard->errorCountdown = CARD_ERROR_COUNTDOWN_FRAMES;
}

static void TrackPersistentCardError(MemoryCardSession *memoryCard) {
    memoryCard->errorPending = 1;
    if (memoryCard->cardStatus != MC_MENU_STATE_ERROR) return;
    if (MemoryCardCountdownElapsed(&memoryCard->errorCountdown)) {
        memoryCard->menuState = MC_MENU_STATE_ERROR;
    }
}

static void RunCardReadyState(MemoryCardSession *memoryCard, s32 fadeBusy) {
    /* Page 0 is the list of things to do with the card, page 1 is picking a
     * slot; any other page is not one this screen has, so it goes back. */
    if (memoryCard->menuPage == 0) {
        RunCardMenuRows(memoryCard, fadeBusy);
    } else if (memoryCard->menuPage == 1) {
        RunCardSlotActions(memoryCard);
    } else {
        memoryCard->menuPage = 0;
        memoryCard->slot = 0;
        ResetCardAction(memoryCard);
        memoryCard->action.timer = 0;
        memoryCard->menuRow = MemoryCardMenuRowCount(memoryCard) - 1;
    }
    switch (memoryCard->menuSelection) {
    case MC_MENU_STATE_BUSY:
        memoryCard->lastMenuState = memoryCard->menuState;
        /* fallthrough */
    case MC_MENU_STATE_UNFORMATTED:
    case MC_MENU_STATE_NO_CARD:
    case MC_MENU_STATE_WORKING:
        memoryCard->menuState = memoryCard->menuSelection;
        break;
    case MC_MENU_STATE_READY:
        ClearPendingCardError(memoryCard);
        break;
    case MC_MENU_STATE_ERROR:
    default:
        TrackPersistentCardError(memoryCard);
        break;
    }
    if (memoryCard->menuState != MC_MENU_STATE_READY) {
        ResetCardAction(memoryCard);
    }
}

static void RunCardWorkingState(MemoryCardSession *memoryCard, s32 fadeBusy) {
    RunCardWorkingActions(memoryCard, fadeBusy);

    switch (memoryCard->menuSelection) {
    case MC_MENU_STATE_BUSY:
        memoryCard->lastMenuState = memoryCard->menuState;
        /* fallthrough */
    case MC_MENU_STATE_UNFORMATTED:
    case MC_MENU_STATE_NO_CARD:
        memoryCard->menuState = memoryCard->menuSelection;
        break;
    case MC_MENU_STATE_WORKING:
        ClearPendingCardError(memoryCard);
        break;
    case MC_MENU_STATE_READY:
        break;
    case MC_MENU_STATE_ERROR:
    case 0:
    default:
        TrackPersistentCardError(memoryCard);
        break;
    }

    if (memoryCard->menuState == MC_MENU_STATE_WORKING) return;
    memoryCard->phase = MC_PROMPT_ACCESSING;
    memoryCard->action.state = 0;
    memoryCard->action.result = 0;
    memoryCard->action.confirmChoice = 0;
}

static void RunNoCardState(MemoryCardSession *memoryCard, s32 fadeBusy) {
    RunNoCardActions(memoryCard, fadeBusy);
    switch (memoryCard->menuSelection) {
    case MC_MENU_STATE_READY:
    case MC_MENU_STATE_WORKING:
        memoryCard->menuState = MC_MENU_STATE_WORKING;
        /* fall through */
    case MC_MENU_STATE_NO_CARD:
        ClearPendingCardError(memoryCard);
        break;
    case MC_MENU_STATE_UNFORMATTED:
        memoryCard->menuState = MC_MENU_STATE_UNFORMATTED;
        break;
    default:
    case MC_MENU_STATE_ERROR:
    case 0:
        TrackPersistentCardError(memoryCard);
        break;
    case MC_MENU_STATE_BUSY:
        break;
    }

    if (memoryCard->menuState != MC_MENU_STATE_NO_CARD) {
        memoryCard->action.state = 0;
    }
}

static void RunUnformattedCardState(MemoryCardSession *memoryCard, s32 fadeBusy) {
    RunUnformattedCardPage(memoryCard, fadeBusy);
    switch (memoryCard->menuSelection) {
    case MC_MENU_STATE_READY:
    case MC_MENU_STATE_WORKING:
        memoryCard->menuState = MC_MENU_STATE_WORKING;
        break;
    case MC_MENU_STATE_BUSY:
        memoryCard->lastMenuState = memoryCard->menuState;
        memoryCard->menuState = MC_MENU_STATE_BUSY;
        break;
    case MC_MENU_STATE_NO_CARD:
        memoryCard->menuState = MC_MENU_STATE_NO_CARD;
        break;
    case MC_MENU_STATE_UNFORMATTED:
        ClearPendingCardError(memoryCard);
        break;
    case MC_MENU_STATE_ERROR:
    default:
        TrackPersistentCardError(memoryCard);
        break;
    }

    if (memoryCard->menuState != MC_MENU_STATE_UNFORMATTED) {
        ResetCardAction(memoryCard);
    }
}

/*
 * The card answered with something the menu has no name for.
 */
static void RunCardErrorState(MemoryCardSession *memoryCard, s32 fadeBusy) {
    memoryCard->phase = MC_PROMPT_CARD_ERROR;
    if ((g_PadPressed & PAD_CANCEL) && !fadeBusy) {
        PlaySoundCue(3);
        memoryCard->action.busy = 0;
        StartMenuExitFade(memoryCard);
    }

    if (memoryCard->menuSelection == MC_MENU_STATE_ERROR) {
        return;
    }
    if (memoryCard->menuSelection == MC_MENU_STATE_BUSY) {
        memoryCard->lastMenuState = memoryCard->menuState;
    }
    memoryCard->menuState = memoryCard->menuSelection;
    ClearPendingCardError(memoryCard);
}

static void PollCardMenuSelection(MemoryCardSession *memoryCard) {
    s32 status;

    if (memoryCard->action.busy != 0 && memoryCard->errorPending == 0) return;

    status = PollMemoryCardStatus(&memoryCard->poll, 0, 0);
    memoryCard->cardStatus = status;
    if (status == 0) {
        /* Debounce a card being reseated before changing the screen. */
        if (++memoryCard->noCardTicks >= NO_CARD_DEBOUNCE_FRAMES) {
            memoryCard->menuSelection = MC_MENU_STATE_BUSY;
        }
        return;
    }

    memoryCard->noCardTicks = 0;
    memoryCard->menuSelection = status;
}

void UpdateMemoryCardMenu(void) {
    MemoryCardSession *memoryCard = SceneRuntimeMemoryCard();
    s32 fadeBusy = UpdateMemoryCardFade(memoryCard);
    if (!AdvanceMemoryCardMenuStartup(memoryCard)) {
        DrawMemoryCardMenu(memoryCard);
        return;
    }
    /* An action already under way owns the card, so its status is not asked
     * again until it reports an error. */
    PollCardMenuSelection(memoryCard);

    /*
     * What the menu does this frame is decided by what the card is: each of
     * these owns one state and nothing else.
     */
    switch (memoryCard->menuState) {
    case MC_MENU_STATE_BUSY:
        RunCardBusyState(memoryCard, fadeBusy);
        break;
    case MC_MENU_STATE_READY:
        RunCardReadyState(memoryCard, fadeBusy);
        break;
    case MC_MENU_STATE_WORKING:
        RunCardWorkingState(memoryCard, fadeBusy);
        break;
    case MC_MENU_STATE_NO_CARD:
        RunNoCardState(memoryCard, fadeBusy);
        break;
    case MC_MENU_STATE_UNFORMATTED:
        RunUnformattedCardState(memoryCard, fadeBusy);
        break;
    case MC_MENU_STATE_ERROR:
    default:
        RunCardErrorState(memoryCard, fadeBusy);
        break;
    }
    DrawMemoryCardMenu(memoryCard);
}
