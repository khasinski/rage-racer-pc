#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

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
            if (g_McErrorTicks >= 4) {
                g_McMenuState = g_McCardStatus;
            }
            g_McErrorTicks++;
        }
        break;
    }
    if (g_McMenuState != MC_MENU_STATE_BUSY) {
        g_McErrorTicks = 0;
    }
}

/*
 * A card the game can use. This is the save and load menu itself, and
 * every prompt that hangs off picking a slot.
 */
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
    g_McErrorCountdown = 3;
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

/*
 * A format or a save running, stepping through its own stages while the
 * screen says it is busy.
 */
static void RunCardWorkingState(s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    switch (g_McActionState) {
    case CARD_WORK_WAIT_FOR_SCENE:
        if ((u32)g_SceneTimer < 0x1F) break;
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        g_McActionState = CARD_WORK_WAIT_FOR_CARD;
        break;
    case CARD_WORK_WAIT_FOR_CARD:
        g_McActionBusy = 0;
        g_McActionElapsed++;
        if ((g_PadPressed & PAD_CANCEL) && g_McActionElapsed >= 0x79) {
            g_McCardOkFrames = 0;
            g_McActionElapsed = 0;
            if (fadeBusy == 0) {
                PlaySoundCue(3);
                StartMenuExitFade();
            }
        }
        if (g_McCardStatus != MC_MENU_STATE_READY) break;
        g_McCardOkFrames++;
        if (g_McCardOkFrames < 2) break;
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        g_McActionState = CARD_WORK_BEGIN_STATUS_DELAY;
        break;
    case CARD_WORK_BEGIN_STATUS_DELAY:
        g_McActionBusy = 1;
        g_McActionTimer = 5;
        g_McActionState = CARD_WORK_WAIT_STATUS_DELAY;
        break;
    case CARD_WORK_WAIT_STATUS_DELAY:
        if (--g_McActionTimer != 0) break;
        g_McActionState = CARD_WORK_REFRESH_STATUS;
        break;
    case CARD_WORK_REFRESH_STATUS:
        g_McSlotUsedMask = RefreshMemoryCardSaveStatus(1, g_McSaveHeaders);
        g_McActionState = CARD_WORK_BEGIN_SETTLE_DELAY;
        break;
    case CARD_WORK_BEGIN_SETTLE_DELAY:
        g_McActionTimer = 5;
        g_McActionState = CARD_WORK_WAIT_SETTLE_DELAY;
        break;
    case CARD_WORK_WAIT_SETTLE_DELAY:
        if (--g_McActionTimer != 0) break;
        g_McActionTimer = 5;
        g_McActionBusy = 0;
        g_McActionState = CARD_WORK_WAIT_FINAL_DELAY;
        break;
    case CARD_WORK_WAIT_FINAL_DELAY:
        if (--g_McActionTimer != 0) break;
        g_McActionState = CARD_WORK_RETURN_READY;
        break;
    case CARD_WORK_RETURN_READY:
        if (g_McMenuSelection != MC_MENU_STATE_READY) break;
        g_McMenuState = g_McMenuSelection;
        break;
    default:
        break;
    }

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

/*
 * Nothing in the slot.
 */
typedef enum NoCardActionState {
    NO_CARD_ACTION_INIT = 0,
    NO_CARD_ACTION_WAIT = 1,
    NO_CARD_ACTION_READY = 3,
} NoCardActionState;

static void RunNoCardState(s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_NO_CARD;
    g_McActionBusy = 0;
    switch (g_McActionState) {
    case NO_CARD_ACTION_INIT:
        g_McActionTimer = 5;
        g_McSlotUsedMask = 0;
        ClearSaveHeaderRows(g_McSaveHeaders);
        g_McLastSlot = 0;
        g_McActionState = NO_CARD_ACTION_WAIT;
        break;

    case NO_CARD_ACTION_WAIT:
        if (--g_McActionTimer == 0) {
            g_McActionState = NO_CARD_ACTION_READY;
        }
        break;

    case NO_CARD_ACTION_READY:
        if (g_McMenuPage == 0) {
            AdjustMenuSelectionHorizontal(&g_McMenuRowCursor, 0,
                                          g_McMenuRowCount - 1);
            if (PollMenuConfirmInput() != 0) {
                if (g_McMenuRowCursor != g_McMenuRowCount - 1) {
                    PlaySoundCue(5);
                    break;
                }
                if (fadeBusy != 0) {
                    break;
                }
                g_McActionState = NO_CARD_ACTION_INIT;
                PlaySoundCue(2);
                StartMenuExitFade();
                break;
            }
            if ((g_PadPressed & PAD_CANCEL) != 0 && fadeBusy == 0) {
                g_McActionState = NO_CARD_ACTION_INIT;
                PlaySoundCue(3);
                StartMenuExitFade();
            }
        } else if (g_McMenuPage == 1) {
            /* The second page has no rows to walk, so cancel is the only way
             * off it, and unlike the first page it leaves the action state
             * where it was. */
            if ((g_PadPressed & PAD_CANCEL) != 0 && fadeBusy == 0) {
                PlaySoundCue(3);
                StartMenuExitFade();
            }
        }
        break;

    default:
        break;
    }
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
        g_McActionState = NO_CARD_ACTION_INIT;
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

void UpdateMemoryCardMenu(void) {
    s32 fadeBusy;

    fadeBusy = UpdateMemoryCardFade();
    if (!AdvanceMemoryCardMenuStartup()) {
        DrawMemoryCardMenu();
        return;
    }
    /* An action already under way owns the card, so its status is not asked
     * again until it reports an error. */
    if (g_McActionBusy == 0 || g_McErrorPending != 0) {
        s32 status = PollMemoryCardStatus(0, 0);

        g_McCardStatus = status;
        if (status == 0) {
            /* No card. Six frames of that in a row before the screen says so,
             * so a card being reseated does not flash the message. */
            s32 ticks = g_McNoCardTicks;

            g_McNoCardTicks = ticks + 1;
            if (ticks >= 6) {
                g_McMenuSelection = MC_MENU_STATE_BUSY;
            }
        } else {
            g_McNoCardTicks = 0;
            g_McMenuSelection = g_McCardStatus;
        }
    }

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
