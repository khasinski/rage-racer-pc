#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

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

static int CardStatusSettledAfterIo(MemoryCardPoll *poll) {
    s32 status = PollMemoryCardStatus(poll, 0, 0);
    /* The asynchronous driver reports pending between completed probes.
     * Pending is not evidence that the card disappeared: resetting here
     * prevents four successful probes from ever accumulating. */
    if (status == MC_CARD_RESULT_PENDING) return 0;
    if (status != MC_MENU_STATE_READY) {
        g_McSettleTicks = 0;
        return 0;
    }
    return ++g_McSettleTicks >= CARD_STABLE_STATUS_FRAMES;
}

/* Confirm and back dismiss result messages in exactly the same way. */
static int CardResultPromptDismissed(void) {
    return PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0;
}

typedef enum CardSlotActionState {
    CARD_SLOT_ACTION_PICK = 0x00,
    CARD_SLOT_ACTION_CONFIRM_OVERWRITE = 0x0A,
    CARD_SLOT_ACTION_WAIT_SAVE_DELAY = 0x0C,
    CARD_SLOT_ACTION_WAIT_SAVE_SETTLE = 0x12,
    CARD_SLOT_ACTION_WAIT_SAVE_CARD = 0x13,
    CARD_SLOT_ACTION_WAIT_SAVE_RESULT = 0x15,
    CARD_SLOT_ACTION_SHOW_CARD_FULL = 0x19,
    CARD_SLOT_ACTION_WAIT_LOAD_PREP = 0x1F,
    CARD_SLOT_ACTION_WAIT_LOAD_DELAY = 0x21,
    CARD_SLOT_ACTION_WAIT_LOAD_SETTLE = 0x24,
    CARD_SLOT_ACTION_WAIT_LOAD_CARD = 0x25,
    CARD_SLOT_ACTION_WAIT_LOAD_RESULT = 0x27,
    CARD_SLOT_ACTION_SHOW_NO_FILE = 0x28,
} CardSlotActionState;

static void BeginSave(MemoryCardAction *action) {
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    action->timer = CARD_SAVE_DELAY_FRAMES;
    action->busy = 1;
    action->state = CARD_SLOT_ACTION_WAIT_SAVE_DELAY;
}

static void BeginLoad(MemoryCardAction *action) {
    action->timer = CARD_IO_SETTLE_DELAY_FRAMES;
    action->state = CARD_SLOT_ACTION_WAIT_LOAD_PREP;
}

static void PickLoadSlot(MemoryCardAction *action) {
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
            action->confirmChoice = 0;
            BeginLoad(action);
        } else {
            PlaySoundCue(5);
            action->state = CARD_SLOT_ACTION_SHOW_NO_FILE;
        }
    }
}

static void PickSaveSlot(MemoryCardAction *action) {
    if (g_McFreeBlocks != 0 || (g_McSlotUsedMask & 7) != 0) {
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if (!(g_PadPressed & PAD_CONFIRM)) return;

        PlaySoundCue(2);
        if (CardSlotIsUsed(g_McSlotCursor)) {
            action->confirmChoice = 0;
            action->state = CARD_SLOT_ACTION_CONFIRM_OVERWRITE;
        } else if (g_McFreeBlocks != 0) {
            BeginSave(action);
        } else {
            action->state = CARD_SLOT_ACTION_SHOW_CARD_FULL;
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
static void PickCardSlot(MemoryCardAction *action) {
    AdjustMenuSelectionVertical(&g_McSlotCursor, 0, 2);
    if (g_McSaveMode != 0) {
        PickLoadSlot(action);
    } else {
        PickSaveSlot(action);
    }

    if (PollMenuBackInput() == 0) return;
    g_McMenuPage = 0;
}

static void WriteSelectedSaveSlot(MemoryCardAction *action) {
    s32 slot = g_McSlotCursor;

    action->result = WriteMemoryCardSaveSlot(slot, &g_McSaveHeaders[slot]);
    if (action->result != 0) {
        g_McSlotUsedMask = RefreshMemoryCardSaveStatus(g_McSaveHeaders);
    }
    action->timer = CARD_IO_SETTLE_DELAY_FRAMES;
    action->state = CARD_SLOT_ACTION_WAIT_SAVE_SETTLE;
}

static void ReadSelectedSaveSlot(MemoryCardAction *action) {
    s32 slot = g_McSlotCursor;

    action->result = LoadMemoryCardSaveSlot(slot, &g_McSaveHeaders[slot]);
    if (action->result != 0) {
        g_McLastSlot = slot;
    }
    action->timer = CARD_IO_SETTLE_DELAY_FRAMES;
    action->state = CARD_SLOT_ACTION_WAIT_LOAD_SETTLE;
}

void RunCardSlotActions(MemoryCardAction *action, MemoryCardPoll *poll) {
    switch (action->state) {
    case CARD_SLOT_ACTION_PICK:
        PickCardSlot(action);
        break;
    case CARD_SLOT_ACTION_CONFIRM_OVERWRITE:
        g_McMenuPhase = MC_PROMPT_OVERWRITE_ASK + (g_McSlotCursor * 2) +
                        action->confirmChoice;
        SetMenuBinaryChoiceHorizontal(&action->confirmChoice);
        if (PollMenuConfirmInput() != 0) {
            if (action->confirmChoice != 0) {
                BeginSave(action);
            } else {
                action->state = CARD_SLOT_ACTION_PICK;
            }
        } else if (PollMenuBackInput() != 0) {
            action->state = CARD_SLOT_ACTION_PICK;
        }
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_DELAY:
        action->busy = 1;
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        WriteSelectedSaveSlot(action);
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_SETTLE:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        g_McSettleTicks = 0;
        action->state = CARD_SLOT_ACTION_WAIT_SAVE_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_CARD:
        if (!CardStatusSettledAfterIo(poll)) break;
        g_McMenuPhase = action->result != 0 ? MC_PROMPT_SAVE_OK
                                             : MC_PROMPT_CARD_ERROR;
        action->timer = CARD_RESULT_DISPLAY_FRAMES;
        action->busy = 0;
        action->state = CARD_SLOT_ACTION_WAIT_SAVE_RESULT;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_RESULT:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        g_McMenuPage = 0;
        action->state = CARD_SLOT_ACTION_PICK;
        g_McMenuRowCursor = MemoryCardMenuRowCount() - 1;
        break;

    case CARD_SLOT_ACTION_SHOW_CARD_FULL:
        g_McMenuPhase = MC_PROMPT_CARD_FULL;
        if (!CardResultPromptDismissed()) break;
        g_McMenuPage = 0;
        action->state = CARD_SLOT_ACTION_PICK;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_PREP:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        g_McMenuPhase = MC_PROMPT_ACCESSING;
        action->timer = CARD_LOAD_DELAY_FRAMES;
        action->busy = 1;
        action->state = CARD_SLOT_ACTION_WAIT_LOAD_DELAY;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_DELAY:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        ReadSelectedSaveSlot(action);
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_SETTLE:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        g_McSettleTicks = 0;
        action->state = CARD_SLOT_ACTION_WAIT_LOAD_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_CARD:
        if (!CardStatusSettledAfterIo(poll)) break;
        g_McMenuPhase = action->result != 0 ? MC_PROMPT_LOAD_OK
                                             : MC_PROMPT_CARD_ERROR;
        action->timer = CARD_RESULT_DISPLAY_FRAMES;
        action->busy = 0;
        action->state = CARD_SLOT_ACTION_WAIT_LOAD_RESULT;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_RESULT:
        if (!MemoryCardCountdownElapsed(&action->timer)) break;
        g_McMenuPage = 0;
        action->state = CARD_SLOT_ACTION_PICK;
        g_McMenuRowCursor = MemoryCardMenuRowCount() - 1;
        break;

    case CARD_SLOT_ACTION_SHOW_NO_FILE:
        g_McMenuPhase = MC_PROMPT_NO_FILE;
        if (!CardResultPromptDismissed()) break;
        g_McMenuPage = 0;
        action->state = CARD_SLOT_ACTION_PICK;
        break;

    default:
        break;
    }
}
