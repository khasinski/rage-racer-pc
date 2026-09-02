#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

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

enum {
    CARD_IO_SETTLE_DELAY_FRAMES = 5,
    CARD_SAVE_DELAY_FRAMES = 10,
    CARD_LOAD_DELAY_FRAMES = 15,
    CARD_RESULT_DISPLAY_FRAMES = 60,
    CARD_STABLE_STATUS_FRAMES = 4,
};

static int CardStatusSettledAfterIo(void) {
    if (PollMemoryCardStatus(0, 0) != MC_MENU_STATE_READY) return 0;
    return ++g_McSettleTicks >= CARD_STABLE_STATUS_FRAMES;
}

/* Confirm and back dismiss result messages in exactly the same way. */
static int CardResultPromptDismissed(void) {
    return PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0;
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

static void PickLoadSlot(void) {
    if ((g_McSlotUsedMask & 7) == 0) {
        g_McMenuPhase = MC_PROMPT_NO_DATA;
        if (g_PadPressed & PAD_CONFIRM) {
            PlaySoundCue(5);
            g_McMenuPage = 0;
        }
        return;
    }

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
}

static void PickSaveSlot(void) {
    if (g_McFreeBlocks != 0) {
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if (g_PadPressed & PAD_CONFIRM) {
            if (CardSlotIsUsed(g_McSlotCursor)) {
                PlaySoundCue(2);
                g_McConfirmChoice = 0;
                g_McActionState = CARD_SLOT_ACTION_CONFIRM_OVERWRITE;
            } else {
                PlaySoundCue(2);
                g_McActionState = CARD_SLOT_ACTION_BEGIN_SAVE;
            }
        }
        return;
    }

    if ((g_McSlotUsedMask & 7) != 0) {
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
        return;
    }

    g_McMenuPhase = MC_PROMPT_CARD_FULL;
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(5);
        g_McMenuPage = 0;
    } else if (PollMenuBackInput() != 0) {
        g_McMenuPage = 0;
    }
}

/*
 * Pick a slot to save or load. Retail asks for back twice on the card-full
 * path, once in PickSaveSlot and once below, and each ask plays its own cue.
 */
static void PickCardSlot(void) {
    AdjustMenuSelectionVertical(&g_McSlotCursor, 0, 2);
    if (g_McSaveMode != 0) {
        PickLoadSlot();
    } else {
        PickSaveSlot();
    }

    if (PollMenuBackInput() == 0) return;
    g_McMenuPage = 0;
}

void RunCardSlotActions(void) {
    switch (g_McActionState) {
    case CARD_SLOT_ACTION_PICK:
        PickCardSlot();
        break;
    case CARD_SLOT_ACTION_CONFIRM_OVERWRITE:
        g_McMenuPhase = MC_PROMPT_OVERWRITE_ASK + (g_McSlotCursor * 2) +
                        g_McConfirmChoice;
        SetMenuBinaryChoiceHorizontal(&g_McConfirmChoice);
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
        g_McActionTimer = CARD_SAVE_DELAY_FRAMES;
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
        g_McActionState = CARD_SLOT_ACTION_FINISH_WRITE;
        break;
    }

    case CARD_SLOT_ACTION_FINISH_WRITE:
        g_McActionState = CARD_SLOT_ACTION_REFRESH_AFTER_SAVE;
        break;

    case CARD_SLOT_ACTION_REFRESH_AFTER_SAVE:
        if (g_McActionResult != 0) {
            s32 usedMask = RefreshMemoryCardSaveStatus(g_McSaveHeaders);

            g_McSlotUsedMask = usedMask;
        }
        g_McActionState = CARD_SLOT_ACTION_BEGIN_SAVE_SETTLE;
        break;

    case CARD_SLOT_ACTION_BEGIN_SAVE_SETTLE:
        g_McActionTimer = CARD_IO_SETTLE_DELAY_FRAMES;
        g_McActionState = CARD_SLOT_ACTION_WAIT_SAVE_SETTLE;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_SETTLE:
        if (!CardActionTimerElapsed()) break;
        g_McSettleTicks = 0;
        g_McActionState = CARD_SLOT_ACTION_WAIT_SAVE_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_CARD:
        if (!CardStatusSettledAfterIo()) break;
        g_McActionState = CARD_SLOT_ACTION_SHOW_SAVE_RESULT;
        break;

    case CARD_SLOT_ACTION_SHOW_SAVE_RESULT:
        g_McMenuPhase = g_McActionResult != 0 ? MC_PROMPT_SAVE_OK
                                             : MC_PROMPT_CARD_ERROR;
        g_McActionTimer = CARD_RESULT_DISPLAY_FRAMES;
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
        g_McActionTimer = CARD_IO_SETTLE_DELAY_FRAMES;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_PREP;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_PREP:
        if (!CardActionTimerElapsed()) break;
        g_McActionState = CARD_SLOT_ACTION_BEGIN_LOAD_DELAY;
        break;

    case CARD_SLOT_ACTION_BEGIN_LOAD_DELAY:
        g_McMenuPhase = MC_PROMPT_ACCESSING;
        g_McActionTimer = CARD_LOAD_DELAY_FRAMES;
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
        if (g_McActionResult != 0) {
            g_McLastSlot = g_McSlotCursor;
        }
        g_McActionTimer = CARD_RESULT_DISPLAY_FRAMES;
        g_McActionState = CARD_SLOT_ACTION_BEGIN_LOAD_SETTLE;
        break;
    }
    case CARD_SLOT_ACTION_BEGIN_LOAD_SETTLE:
        g_McActionTimer = CARD_IO_SETTLE_DELAY_FRAMES;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_SETTLE;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_SETTLE:
        if (!CardActionTimerElapsed()) break;
        g_McSettleTicks = 0;
        g_McActionState = CARD_SLOT_ACTION_WAIT_LOAD_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_CARD:
        if (!CardStatusSettledAfterIo()) break;
        g_McActionState = CARD_SLOT_ACTION_SHOW_LOAD_RESULT;
        break;

    case CARD_SLOT_ACTION_SHOW_LOAD_RESULT:
        g_McMenuPhase = g_McActionResult != 0 ? MC_PROMPT_LOAD_OK
                                             : MC_PROMPT_CARD_ERROR;
        g_McActionTimer = CARD_RESULT_DISPLAY_FRAMES;
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
