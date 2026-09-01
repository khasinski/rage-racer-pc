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

/*
 * Several of the card actions sit still for a few frames before they act, so
 * that the message on screen can be read. Answers whether the wait is over.
 */
static int CardActionTimerElapsed(void) {
    g_McActionTimer -= 1;
    return g_McActionTimer == 0;
}

/* Whether a slot already holds a save. */
static int CardSlotIsUsed(s32 slot) {
    return ((g_McSlotUsedMask >> slot) & 1) != 0;
}

/* Confirm and back dismiss result messages in exactly the same way. */
static int CardResultPromptDismissed(void) {
    return PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0;
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

typedef enum CardSlotActionState {
    CARD_SLOT_ACTION_PICK = 0x00,
    CARD_SLOT_ACTION_CONFIRM_OVERWRITE = 0x0A,
    CARD_SLOT_ACTION_BEGIN_SAVE = 0x0B,
    CARD_SLOT_ACTION_WAIT_SAVE_DELAY = 0x0C,
    CARD_SLOT_ACTION_WRITE_SAVE = 0x0D,
    CARD_SLOT_ACTION_FINISH_WRITE = 0x0F,
    CARD_SLOT_ACTION_REFRESH_AFTER_SAVE = 0x10,
    CARD_SLOT_ACTION_BEGIN_SAVE_SETTLE = 0x11,
    CARD_SLOT_ACTION_WAIT_SAVE_SETTLE = 0x12,
    CARD_SLOT_ACTION_WAIT_SAVE_CARD = 0x13,
    CARD_SLOT_ACTION_SHOW_SAVE_RESULT = 0x14,
    CARD_SLOT_ACTION_WAIT_SAVE_RESULT = 0x15,
    CARD_SLOT_ACTION_SHOW_CARD_FULL = 0x19,
    CARD_SLOT_ACTION_BEGIN_LOAD = 0x1E,
    CARD_SLOT_ACTION_WAIT_LOAD_PREP = 0x1F,
    CARD_SLOT_ACTION_BEGIN_LOAD_DELAY = 0x20,
    CARD_SLOT_ACTION_WAIT_LOAD_DELAY = 0x21,
    CARD_SLOT_ACTION_READ_SAVE = 0x22,
    CARD_SLOT_ACTION_BEGIN_LOAD_SETTLE = 0x23,
    CARD_SLOT_ACTION_WAIT_LOAD_SETTLE = 0x24,
    CARD_SLOT_ACTION_WAIT_LOAD_CARD = 0x25,
    CARD_SLOT_ACTION_SHOW_LOAD_RESULT = 0x26,
    CARD_SLOT_ACTION_WAIT_LOAD_RESULT = 0x27,
    CARD_SLOT_ACTION_SHOW_NO_FILE = 0x28,
} CardSlotActionState;

/*
 * Picking a slot, which is where the card menu spends most of its time.
 */
static void PickCardSlot(void) {
    /*
     * Picking a slot. Which prompt the player sees, and what confirming
     * does, depends on whether this is a save or a load, whether the card
     * has room, and whether the slot under the cursor already holds a
     * file. Retail asks the back button twice on the card-full path, once
     * inside the branch and once on the way out, and each ask plays its
     * own cue, so both stay.
     */
    AdjustMenuSelectionHorizontal(&g_McSlotCursor, 0, 2);
    if (g_McSaveMode != 0) {
        if ((g_McSlotUsedMask % 8) != 0) {
            g_McMenuPhase = MC_PROMPT_SELECT_LOAD;
            if (g_PadPressed & PAD_CONFIRM) {
                if (CardSlotIsUsed(g_McSlotCursor)) {
                    PlaySoundCue(2);
                    g_McConfirmChoice = 0;
                    g_McActionState = CARD_SLOT_ACTION_BEGIN_LOAD;
                } else {
                    PlaySoundCue(5);
                    g_McActionState = CARD_SLOT_ACTION_SHOW_NO_FILE;
                }
            }
        } else {
            g_McMenuPhase = MC_PROMPT_NO_DATA;
            if (g_PadPressed & PAD_CONFIRM) {
                PlaySoundCue(5);
                g_McMenuPage = 0;
            }
        }
    } else if (g_McFreeBlocks != 0) {
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if (g_PadPressed & PAD_CONFIRM) {
            if (CardSlotIsUsed(g_McSlotCursor)) {
                PlaySoundCue(2);
                g_McConfirmChoice = 0;
                g_McActionState = CARD_SLOT_ACTION_CONFIRM_OVERWRITE;
            } else {
                PlaySoundCue(2);
                g_McActionTimer = 0x1E;
                g_McActionState = CARD_SLOT_ACTION_BEGIN_SAVE;
            }
        }
    } else if ((g_McSlotUsedMask % 8) != 0) {
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if (g_PadPressed & PAD_CONFIRM) {
            if (CardSlotIsUsed(g_McSlotCursor)) {
                PlaySoundCue(2);
                g_McConfirmChoice = 0;
                g_McActionState = CARD_SLOT_ACTION_CONFIRM_OVERWRITE;
            } else {
                PlaySoundCue(2);
                g_McActionState = CARD_SLOT_ACTION_SHOW_CARD_FULL;
            }
        }
    } else {
        g_McMenuPhase = MC_PROMPT_CARD_FULL;
        if (g_PadPressed & PAD_CONFIRM) {
            PlaySoundCue(5);
            g_McMenuPage = 0;
        } else if (PollMenuBackInput() != 0) {
            g_McMenuPage = 0;
        }
    }
    if (PollMenuBackInput() == 0) return;
    g_McMenuPage = 0;
}

static void RunCardSlotActions(void) {
    switch (g_McActionState) {
    case CARD_SLOT_ACTION_PICK:
        PickCardSlot();
        break;
    case CARD_SLOT_ACTION_CONFIRM_OVERWRITE:
        g_McMenuPhase = (g_McSlotCursor * 2) + g_McConfirmChoice + 9;
        SetMenuBinaryChoiceVertical(&g_McConfirmChoice);
        if (PollMenuConfirmInput() != 0) {
            g_McActionState = g_McConfirmChoice != 0
                                  ? CARD_SLOT_ACTION_BEGIN_SAVE
                                  : CARD_SLOT_ACTION_PICK;
        } else if (PollMenuBackInput() != 0) {
            g_McActionState = CARD_SLOT_ACTION_PICK;
        }
        break;

    case CARD_SLOT_ACTION_BEGIN_SAVE:
        g_McMenuPhase = MC_PROMPT_ACCESSING;
        g_McActionTimer = 0xA;
        g_McActionState = CARD_SLOT_ACTION_WAIT_SAVE_DELAY;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_DELAY:
        g_McActionBusy = 1;
        if (!CardActionTimerElapsed()) break;
        g_McActionState = CARD_SLOT_ACTION_WRITE_SAVE;
        break;

    case CARD_SLOT_ACTION_WRITE_SAVE: {
        s32 slot = g_McSlotCursor;
        s32 written;

        written = WriteMemoryCardSaveSlot(slot, &g_McSaveHeaders[slot]);
        g_McActionResult = written;
        g_McActionOk = written != 0;
        g_McActionState = CARD_SLOT_ACTION_FINISH_WRITE;
        break;
    }

    case CARD_SLOT_ACTION_FINISH_WRITE:
        g_McActionState = CARD_SLOT_ACTION_REFRESH_AFTER_SAVE;
        break;

    case CARD_SLOT_ACTION_REFRESH_AFTER_SAVE:
        if (g_McActionResult != 0) {
            s32 usedMask = RefreshMemoryCardSaveStatus(0, g_McSaveHeaders);

            g_McSlotUsedMask = usedMask;
        }
        g_McActionState = CARD_SLOT_ACTION_BEGIN_SAVE_SETTLE;
        break;

    case CARD_SLOT_ACTION_BEGIN_SAVE_SETTLE:
        g_McActionTimer = 5;
        g_McActionState = CARD_SLOT_ACTION_WAIT_SAVE_SETTLE;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_SETTLE:
        if (!CardActionTimerElapsed()) break;
        g_McSettleTicks = 0;
        g_McActionState = CARD_SLOT_ACTION_WAIT_SAVE_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_CARD: {
        s32 t;
        if (PollMemoryCardStatus(0, 0) != 1) break;
        t = g_McSettleTicks + 1;
        g_McSettleTicks = t;
        if (t < 4) break;
        g_McActionState = CARD_SLOT_ACTION_SHOW_SAVE_RESULT;
        break;
    }

    case CARD_SLOT_ACTION_SHOW_SAVE_RESULT:
        g_McMenuPhase = g_McActionOk ? MC_PROMPT_SAVE_OK
                                     : MC_PROMPT_CARD_ERROR;
        g_McActionTimer = 0x3C;
        g_McActionBusy = 0;
        g_McActionState = CARD_SLOT_ACTION_WAIT_SAVE_RESULT;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_RESULT:
        if (!CardActionTimerElapsed()) break;
        g_McMenuPage = 0;
        g_McActionState = CARD_SLOT_ACTION_PICK;
        g_McMenuRowCursor = g_McMenuRowCount - 1;
        break;

    case CARD_SLOT_ACTION_SHOW_CARD_FULL:
        g_McMenuPhase = MC_PROMPT_CARD_FULL;
        if (!CardResultPromptDismissed()) break;
        g_McMenuPage = 0;
        g_McActionState = CARD_SLOT_ACTION_PICK;
        break;

    case CARD_SLOT_ACTION_BEGIN_LOAD:
        g_McActionTimer = 5;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_PREP;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_PREP:
        if (!CardActionTimerElapsed()) break;
        g_McActionState = CARD_SLOT_ACTION_BEGIN_LOAD_DELAY;
        break;

    case CARD_SLOT_ACTION_BEGIN_LOAD_DELAY:
        g_McMenuPhase = MC_PROMPT_ACCESSING;
        g_McActionTimer = 0xF;
        g_McActionBusy = 1;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_DELAY;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_DELAY:
        if (!CardActionTimerElapsed()) break;
        g_McActionState = CARD_SLOT_ACTION_READ_SAVE;
        break;

    case CARD_SLOT_ACTION_READ_SAVE: {
        s32 slot = g_McSlotCursor;

        g_McActionResult = LoadMemoryCardSaveSlot(slot, &g_McSaveHeaders[slot]);
        g_McActionOk = g_McActionResult != 0;
        if (g_McActionResult != 0) {
            g_McLastSlot = g_McSlotCursor;
        }
        g_McActionTimer = 0x3C;
        g_McActionState = CARD_SLOT_ACTION_BEGIN_LOAD_SETTLE;
        break;
    }
    case CARD_SLOT_ACTION_BEGIN_LOAD_SETTLE:
        g_McActionTimer = 5;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_SETTLE;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_SETTLE:
        if (!CardActionTimerElapsed()) break;
        g_McSettleTicks = 0;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_CARD: {
        s32 t;
        if (PollMemoryCardStatus(0, 0) != 1) break;
        t = g_McSettleTicks + 1;
        g_McSettleTicks = t;
        if (t < 4) break;
        g_McActionState = CARD_SLOT_ACTION_SHOW_LOAD_RESULT;
        break;
    }

    case CARD_SLOT_ACTION_SHOW_LOAD_RESULT:
        g_McMenuPhase = g_McActionOk ? MC_PROMPT_LOAD_OK
                                     : MC_PROMPT_CARD_ERROR;
        g_McActionTimer = 0x3C;
        g_McActionBusy = 0;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_RESULT;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_RESULT:
        if (!CardActionTimerElapsed()) break;
        g_McMenuPage = 0;
        g_McActionState = CARD_SLOT_ACTION_PICK;
        g_McMenuRowCursor = g_McMenuRowCount - 1;
        break;

    case CARD_SLOT_ACTION_SHOW_NO_FILE:
        g_McMenuPhase = MC_PROMPT_NO_FILE;
        if (!CardResultPromptDismissed()) break;
        g_McMenuPage = 0;
        g_McActionState = CARD_SLOT_ACTION_PICK;
        break;

    default:
        break;
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

/*
 * A card the game cannot read: offer to format it, then report how that
 * went.
 */
typedef enum FormatCardActionState {
    FORMAT_CARD_ACTION_PROMPT = 0,
    FORMAT_CARD_ACTION_CONFIRM = 1,
    FORMAT_CARD_ACTION_BEGIN_DELAY = 2,
    FORMAT_CARD_ACTION_WAIT_DELAY = 3,
    FORMAT_CARD_ACTION_RUN = 5,
    FORMAT_CARD_ACTION_SHOW_SUCCESS = 7,
    FORMAT_CARD_ACTION_WAIT_TO_EXIT = 8,
    FORMAT_CARD_ACTION_SHOW_ERROR = 0xA,
} FormatCardActionState;

static void RunUnformattedCardRootPage(s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_NONE;
    AdjustMenuSelectionHorizontal(&g_McMenuRowCursor, 0,
                                  g_McMenuRowCount - 1);

    if (g_PadPressed & PAD_CONFIRM) {
        if (g_McMenuRowCursor == 0) {
            PlaySoundCue(2);
            g_McMenuPage = 1;
            g_McConfirmChoice = 0;
            g_McSaveMode = 0;
        } else if (g_McMenuRowCursor == g_McMenuRowCount - 1) {
            if (!fadeBusy) {
                PlaySoundCue(2);
                g_McActionBusy = 0;
                StartMenuExitFade();
            }
        } else {
            PlaySoundCue(5);
            g_McMenuPage = 1;
            g_McSaveMode = g_McMenuRowCursor;
        }
    } else if ((g_PadPressed & PAD_CANCEL) && !fadeBusy) {
        PlaySoundCue(3);
        g_McActionBusy = 0;
        StartMenuExitFade();
    }
}

static void RunFormatCardActions(s32 fadeBusy) {
    u16 confirm;

    switch (g_McActionState) {
    case FORMAT_CARD_ACTION_PROMPT:
        if (g_McSaveMode != 0) {
            g_McMenuPhase = MC_PROMPT_NO_DATA;
            if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
                g_McMenuPage = 0;
                g_McActionState = FORMAT_CARD_ACTION_PROMPT;
            }
            break;
        }
        g_McMenuPhase = MC_PROMPT_NEW_CARD;
        if (PollMenuConfirmInput() != 0) {
            g_McActionState = FORMAT_CARD_ACTION_CONFIRM;
        } else if (PollMenuBackInput() != 0) {
            g_McMenuPage = 0;
            g_McActionState = FORMAT_CARD_ACTION_PROMPT;
        }
        break;

    case FORMAT_CARD_ACTION_CONFIRM:
        g_McMenuPhase = g_McConfirmChoice + MC_PROMPT_FORMAT_ASK;
        SetMenuBinaryChoiceVertical(&g_McConfirmChoice);
        confirm = PollMenuConfirmInput();
        if (g_McConfirmChoice != 0 && confirm != 0) {
            g_McActionState = FORMAT_CARD_ACTION_BEGIN_DELAY;
        } else if (confirm != 0 || PollMenuBackInput() != 0) {
            g_McMenuPage = 0;
            g_McActionState = FORMAT_CARD_ACTION_PROMPT;
        }
        break;

    case FORMAT_CARD_ACTION_BEGIN_DELAY:
        g_McActionBusy = 1;
        g_McActionTimer = 0x14;
        g_McActionState = FORMAT_CARD_ACTION_WAIT_DELAY;
        break;

    case FORMAT_CARD_ACTION_WAIT_DELAY:
        if (--g_McActionTimer == 0) {
            g_McActionState = FORMAT_CARD_ACTION_RUN;
        }
        break;

    case FORMAT_CARD_ACTION_RUN:
        g_McActionResult = FormatMemoryCard(0, 0);
        if (g_McActionResult == 1) {
            g_McActionState = FORMAT_CARD_ACTION_SHOW_SUCCESS;
            g_McActionTimer = 0x3C;
        } else {
            g_McActionState = FORMAT_CARD_ACTION_SHOW_ERROR;
        }
        break;

    case FORMAT_CARD_ACTION_SHOW_SUCCESS:
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        if (--g_McActionTimer == 0) {
            g_McActionBusy = 0;
            g_McActionState = FORMAT_CARD_ACTION_WAIT_TO_EXIT;
        }
        break;

    case FORMAT_CARD_ACTION_WAIT_TO_EXIT:
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        if (!(g_PadPressed & PAD_CANCEL)) break;
        g_McActionBusy = 0;
        g_McActionState = FORMAT_CARD_ACTION_PROMPT;
        g_McActionResult = 0;
        g_McConfirmChoice = 0;
        g_McActionTimer = 0;
        if (!fadeBusy) {
            PlaySoundCue(3);
            StartMenuExitFade();
        }
        break;

    case FORMAT_CARD_ACTION_SHOW_ERROR:
        g_McMenuPhase = MC_PROMPT_CARD_ERROR;
        g_McActionBusy = 0;
        if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
            g_McActionState = FORMAT_CARD_ACTION_PROMPT;
        }
        break;

    default:
        break;
    }
}

static void RunUnformattedCardState(s32 fadeBusy) {
    switch (g_McMenuPage) {
    case 0:
        RunUnformattedCardRootPage(fadeBusy);
        break;

    case 1:
        RunFormatCardActions(fadeBusy);
        break;

    default:
        break;
    }
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
