#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

enum {
    BUSY_ERROR_DEBOUNCE_FRAMES = 5,
    CARD_ERROR_COUNTDOWN_FRAMES = 3,
    NO_CARD_DEBOUNCE_FRAMES = 7,
};

/*
 * The card is mid-operation. Nothing to choose here; the cancel button
 * is the only way out, and only once the fade has finished.
 */
static void RunCardBusyState(s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    g_McActionBusy = 0;
    if ((g_PadPressed & PAD_CANCEL) && !fadeBusy) {
        PlaySoundCue(3);
        StartMenuExitFade();
    }
    switch (g_McMenuSelection) {
    case MC_MENU_STATE_READY:
        if (g_McCardStatus == MC_MENU_STATE_READY) {
            g_McMenuState = g_McLastMenuState != MC_MENU_STATE_WORKING
                                ? MC_MENU_STATE_WORKING
                                : g_McCardStatus;
        }
        break;
    case MC_MENU_STATE_WORKING:
        g_McMenuState = MC_MENU_STATE_WORKING;
        break;
    case MC_MENU_STATE_NO_CARD:
    case MC_MENU_STATE_UNFORMATTED:
        g_McMenuState = g_McMenuSelection;
        break;
    case MC_MENU_STATE_BUSY:
        break;
    case MC_MENU_STATE_ERROR:
    default:
        if (g_McCardStatus == MC_MENU_STATE_ERROR) {
            if (++g_McErrorTicks >= BUSY_ERROR_DEBOUNCE_FRAMES) {
                g_McMenuState = g_McCardStatus;
            }
        }
        break;
    }
    if (g_McMenuState != MC_MENU_STATE_BUSY) {
        g_McErrorTicks = 0;
    }
}

/*
 * The list of things the player can do with a readable card. The last row
 * is the way out.
 */
static void RunCardMenuRows(s32 fadeBusy) {
    u16 pad;

    g_McMenuPhase = MC_PROMPT_NONE;
    AdjustMenuSelectionHorizontal(&g_McMenuRowCursor, 0,
                                  g_McMenuRowCount - 1);
    pad = g_PadPressed;
    if (pad & PAD_CONFIRM) {
        if (g_McMenuRowCursor < g_McMenuRowCount - 1) {
            PlaySoundCue(2);
            g_McMenuPage = 1;
            g_McActionState = 0;
            g_McActionResult = 0;
            g_McSlotCursor = g_McLastSlot;
            g_McSaveMode = g_McMenuRowCursor;
            return;
        }
        if (fadeBusy) return;
        PlaySoundCue(2);
    } else {
        if ((pad & PAD_CANCEL) == 0 || fadeBusy) return;
        PlaySoundCue(3);
    }
    g_McActionBusy = 0;
    StartMenuExitFade();
}

static void ResetCardAction(void) {
    g_McActionState = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    g_McActionBusy = 0;
}

static void ClearPendingCardError(void) {
    if (g_McErrorPending == 0) return;
    g_McErrorPending = 0;
    g_McErrorCountdown = CARD_ERROR_COUNTDOWN_FRAMES;
}

static void TrackPersistentCardError(void) {
    g_McErrorPending = 1;
    if (g_McCardStatus != MC_MENU_STATE_ERROR) return;
    if (--g_McErrorCountdown == 0) {
        g_McMenuState = MC_MENU_STATE_ERROR;
    }
}

static void RunCardReadyState(s32 fadeBusy) {
    /* Page 0 is the list of things to do with the card, page 1 is picking a
     * slot; any other page is not one this screen has, so it goes back. */
    if (g_McMenuPage == 0) {
        RunCardMenuRows(fadeBusy);
    } else if (g_McMenuPage == 1) {
        RunCardSlotActions();
    } else {
        g_McMenuPage = 0;
        g_McSlotCursor = 0;
        ResetCardAction();
        g_McActionTimer = 0;
        g_McMenuRowCursor = g_McMenuRowCount - 1;
    }
    switch (g_McMenuSelection) {
    case MC_MENU_STATE_BUSY:
        g_McLastMenuState = g_McMenuState;
        /* fallthrough */
    case MC_MENU_STATE_UNFORMATTED:
    case MC_MENU_STATE_NO_CARD:
    case MC_MENU_STATE_WORKING:
        g_McMenuState = g_McMenuSelection;
        break;
    case MC_MENU_STATE_READY:
        ClearPendingCardError();
        break;
    case MC_MENU_STATE_ERROR:
    default:
        TrackPersistentCardError();
        break;
    }
    if (g_McMenuState != MC_MENU_STATE_READY) {
        ResetCardAction();
    }
}

static void RunCardWorkingState(s32 fadeBusy) {
    RunCardWorkingActions(fadeBusy);

    switch (g_McMenuSelection) {
    case MC_MENU_STATE_BUSY:
        g_McLastMenuState = g_McMenuState;
        /* fallthrough */
    case MC_MENU_STATE_UNFORMATTED:
    case MC_MENU_STATE_NO_CARD:
        g_McMenuState = g_McMenuSelection;
        break;
    case MC_MENU_STATE_WORKING:
        ClearPendingCardError();
        break;
    case MC_MENU_STATE_READY:
        break;
    case MC_MENU_STATE_ERROR:
    case 0:
    default:
        TrackPersistentCardError();
        break;
    }

    if (g_McMenuState == MC_MENU_STATE_WORKING) return;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    g_McActionState = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
}

static void RunNoCardState(s32 fadeBusy) {
    RunNoCardActions(fadeBusy);
    switch (g_McMenuSelection) {
    case MC_MENU_STATE_READY:
    case MC_MENU_STATE_WORKING:
        g_McMenuState = MC_MENU_STATE_WORKING;
        /* fall through */
    case MC_MENU_STATE_NO_CARD:
        ClearPendingCardError();
        break;
    case MC_MENU_STATE_UNFORMATTED:
        g_McMenuState = MC_MENU_STATE_UNFORMATTED;
        break;
    default:
    case MC_MENU_STATE_ERROR:
    case 0:
        TrackPersistentCardError();
        break;
    case MC_MENU_STATE_BUSY:
        break;
    }

    if (g_McMenuState != MC_MENU_STATE_NO_CARD) {
        g_McActionState = 0;
    }
}

static void RunUnformattedCardState(s32 fadeBusy) {
    RunUnformattedCardPage(fadeBusy);
    switch (g_McMenuSelection) {
    case MC_MENU_STATE_READY:
    case MC_MENU_STATE_WORKING:
        g_McMenuState = MC_MENU_STATE_WORKING;
        break;
    case MC_MENU_STATE_BUSY:
        g_McLastMenuState = g_McMenuState;
        g_McMenuState = MC_MENU_STATE_BUSY;
        break;
    case MC_MENU_STATE_NO_CARD:
        g_McMenuState = MC_MENU_STATE_NO_CARD;
        break;
    case MC_MENU_STATE_UNFORMATTED:
        ClearPendingCardError();
        break;
    case MC_MENU_STATE_ERROR:
    default:
        TrackPersistentCardError();
        break;
    }

    if (g_McMenuState != MC_MENU_STATE_UNFORMATTED) {
        ResetCardAction();
    }
}

/*
 * The card answered with something the menu has no name for.
 */
static void RunCardErrorState(s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_CARD_ERROR;
    if ((g_PadPressed & PAD_CANCEL) && !fadeBusy) {
        PlaySoundCue(3);
        g_McActionBusy = 0;
        StartMenuExitFade();
    }

    if (g_McMenuSelection == MC_MENU_STATE_ERROR) {
        return;
    }
    if (g_McMenuSelection == MC_MENU_STATE_BUSY) {
        g_McLastMenuState = g_McMenuState;
    }
    g_McMenuState = g_McMenuSelection;
    ClearPendingCardError();
}

static void PollCardMenuSelection(void) {
    s32 status;

    if (g_McActionBusy != 0 && g_McErrorPending == 0) return;

    status = PollMemoryCardStatus(0, 0);
    g_McCardStatus = status;
    if (status == 0) {
        /* Debounce a card being reseated before changing the screen. */
        if (++g_McNoCardTicks >= NO_CARD_DEBOUNCE_FRAMES) {
            g_McMenuSelection = MC_MENU_STATE_BUSY;
        }
        return;
    }

    g_McNoCardTicks = 0;
    g_McMenuSelection = status;
}

void UpdateMemoryCardMenu(void) {
    s32 fadeBusy;

    fadeBusy = UpdateMemoryCardFade();
    if (!AdvanceMemoryCardMenuStartup()) {
        DrawMemoryCardMenu();
        return;
    }
    /* An action already under way owns the card, so its status is not asked
     * again until it reports an error. */
    PollCardMenuSelection();

    /*
     * What the menu does this frame is decided by what the card is: each of
     * these owns one state and nothing else.
     */
    switch (g_McMenuState) {
    case MC_MENU_STATE_BUSY:
        RunCardBusyState(fadeBusy);
        break;
    case MC_MENU_STATE_READY:
        RunCardReadyState(fadeBusy);
        break;
    case MC_MENU_STATE_WORKING:
        RunCardWorkingState(fadeBusy);
        break;
    case MC_MENU_STATE_NO_CARD:
        RunNoCardState(fadeBusy);
        break;
    case MC_MENU_STATE_UNFORMATTED:
        RunUnformattedCardState(fadeBusy);
        break;
    case MC_MENU_STATE_ERROR:
    default:
        RunCardErrorState(fadeBusy);
        break;
    }
    DrawMemoryCardMenu();
}
