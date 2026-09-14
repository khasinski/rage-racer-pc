#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

/* Whether a slot already holds a save. */
static int CardSlotIsUsed(const MemoryCardSession *memoryCard, s32 slot) {
    return ((memoryCard->slots.usedMask >> slot) & 1) != 0;
}

enum {
    CARD_IO_SETTLE_DELAY_FRAMES = 5,
    CARD_SAVE_DELAY_FRAMES = 10,
    CARD_LOAD_DELAY_FRAMES = 15,
    CARD_RESULT_DISPLAY_FRAMES = 60,
    CARD_STABLE_STATUS_FRAMES = 4,
};

static int CardStatusSettledAfterIo(MemoryCardSession *memoryCard) {
    s32 status = PollMemoryCardStatus(&memoryCard->poll, 0, 0);
    /* The asynchronous driver reports pending between completed probes.
     * Pending is not evidence that the card disappeared: resetting here
     * prevents four successful probes from ever accumulating. */
    if (status == MC_CARD_RESULT_PENDING) return 0;
    if (status != MC_MENU_STATE_READY) {
        memoryCard->settleTicks = 0;
        return 0;
    }
    return ++memoryCard->settleTicks >= CARD_STABLE_STATUS_FRAMES;
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

static void BeginSave(MemoryCardSession *memoryCard) {
    memoryCard->phase = MC_PROMPT_ACCESSING;
    memoryCard->action.timer = CARD_SAVE_DELAY_FRAMES;
    memoryCard->action.busy = 1;
    memoryCard->action.state = CARD_SLOT_ACTION_WAIT_SAVE_DELAY;
}

static void BeginLoad(MemoryCardSession *memoryCard) {
    memoryCard->action.timer = CARD_IO_SETTLE_DELAY_FRAMES;
    memoryCard->action.state = CARD_SLOT_ACTION_WAIT_LOAD_PREP;
}

static void PickLoadSlot(MemoryCardSession *memoryCard) {
    if ((memoryCard->slots.usedMask & 7) == 0) {
        memoryCard->phase = MC_PROMPT_NO_DATA;
        if (g_PadPressed & PAD_CONFIRM) {
            PlaySoundCue(5);
            memoryCard->menuPage = 0;
        }
        return;
    }

    memoryCard->phase = MC_PROMPT_SELECT_LOAD;
    if (g_PadPressed & PAD_CONFIRM) {
        if (CardSlotIsUsed(memoryCard, memoryCard->slot)) {
            PlaySoundCue(2);
            memoryCard->action.confirmChoice = 0;
            BeginLoad(memoryCard);
        } else {
            PlaySoundCue(5);
            memoryCard->action.state = CARD_SLOT_ACTION_SHOW_NO_FILE;
        }
    }
}

static void PickSaveSlot(MemoryCardSession *memoryCard) {
    if (memoryCard->freeBlocks != 0 || (memoryCard->slots.usedMask & 7) != 0) {
        memoryCard->phase = MC_PROMPT_SELECT_SAVE;
        if (!(g_PadPressed & PAD_CONFIRM)) return;

        PlaySoundCue(2);
        if (CardSlotIsUsed(memoryCard, memoryCard->slot)) {
            memoryCard->action.confirmChoice = 0;
            memoryCard->action.state = CARD_SLOT_ACTION_CONFIRM_OVERWRITE;
        } else if (memoryCard->freeBlocks != 0) {
            BeginSave(memoryCard);
        } else {
            memoryCard->action.state = CARD_SLOT_ACTION_SHOW_CARD_FULL;
        }
        return;
    }

    memoryCard->phase = MC_PROMPT_CARD_FULL;
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(5);
        memoryCard->menuPage = 0;
    } else if (PollMenuBackInput() != 0) {
        memoryCard->menuPage = 0;
    }
}

/*
 * Pick a slot to save or load. Retail asks for back twice on the card-full
 * path, once in PickSaveSlot and once below, and each ask plays its own cue.
 */
static void PickCardSlot(MemoryCardSession *memoryCard) {
    AdjustMenuSelectionVertical(&memoryCard->slot, 0, 2);
    if (memoryCard->saveMode != 0) {
        PickLoadSlot(memoryCard);
    } else {
        PickSaveSlot(memoryCard);
    }

    if (PollMenuBackInput() == 0) return;
    memoryCard->menuPage = 0;
}

static void WriteSelectedSaveSlot(MemoryCardSession *memoryCard) {
    s32 slot = memoryCard->slot;

    memoryCard->action.result = WriteMemoryCardSaveSlot(slot, &memoryCard->slots.headers[slot]);
    if (memoryCard->action.result != 0) {
        memoryCard->slots.usedMask = RefreshMemoryCardSaveStatus(
            memoryCard->slots.headers, &memoryCard->freeBlocks);
    }
    memoryCard->action.timer = CARD_IO_SETTLE_DELAY_FRAMES;
    memoryCard->action.state = CARD_SLOT_ACTION_WAIT_SAVE_SETTLE;
}

static void ReadSelectedSaveSlot(MemoryCardSession *memoryCard) {
    s32 slot = memoryCard->slot;

    memoryCard->action.result = LoadMemoryCardSaveSlot(slot, &memoryCard->slots.headers[slot]);
    if (memoryCard->action.result != 0) {
        memoryCard->slots.lastSlot = slot;
    }
    memoryCard->action.timer = CARD_IO_SETTLE_DELAY_FRAMES;
    memoryCard->action.state = CARD_SLOT_ACTION_WAIT_LOAD_SETTLE;
}

void RunCardSlotActions(MemoryCardSession *memoryCard) {
    switch (memoryCard->action.state) {
    case CARD_SLOT_ACTION_PICK:
        PickCardSlot(memoryCard);
        break;
    case CARD_SLOT_ACTION_CONFIRM_OVERWRITE:
        memoryCard->phase = MC_PROMPT_OVERWRITE_ASK + (memoryCard->slot * 2) +
                        memoryCard->action.confirmChoice;
        SetMenuBinaryChoiceHorizontal(&memoryCard->action.confirmChoice);
        if (PollMenuConfirmInput() != 0) {
            if (memoryCard->action.confirmChoice != 0) {
                BeginSave(memoryCard);
            } else {
                memoryCard->action.state = CARD_SLOT_ACTION_PICK;
            }
        } else if (PollMenuBackInput() != 0) {
            memoryCard->action.state = CARD_SLOT_ACTION_PICK;
        }
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_DELAY:
        memoryCard->action.busy = 1;
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        WriteSelectedSaveSlot(memoryCard);
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_SETTLE:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->settleTicks = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_WAIT_SAVE_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_CARD:
        if (!CardStatusSettledAfterIo(memoryCard)) break;
        memoryCard->phase = memoryCard->action.result != 0 ? MC_PROMPT_SAVE_OK
                                             : MC_PROMPT_CARD_ERROR;
        memoryCard->action.timer = CARD_RESULT_DISPLAY_FRAMES;
        memoryCard->action.busy = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_WAIT_SAVE_RESULT;
        break;

    case CARD_SLOT_ACTION_WAIT_SAVE_RESULT:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->menuPage = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_PICK;
        memoryCard->menuRow = MemoryCardMenuRowCount(memoryCard) - 1;
        break;

    case CARD_SLOT_ACTION_SHOW_CARD_FULL:
        memoryCard->phase = MC_PROMPT_CARD_FULL;
        if (!CardResultPromptDismissed()) break;
        memoryCard->menuPage = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_PICK;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_PREP:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->phase = MC_PROMPT_ACCESSING;
        memoryCard->action.timer = CARD_LOAD_DELAY_FRAMES;
        memoryCard->action.busy = 1;
        memoryCard->action.state = CARD_SLOT_ACTION_WAIT_LOAD_DELAY;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_DELAY:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        ReadSelectedSaveSlot(memoryCard);
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_SETTLE:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->settleTicks = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_WAIT_LOAD_CARD;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_CARD:
        if (!CardStatusSettledAfterIo(memoryCard)) break;
        memoryCard->phase = memoryCard->action.result != 0 ? MC_PROMPT_LOAD_OK
                                             : MC_PROMPT_CARD_ERROR;
        memoryCard->action.timer = CARD_RESULT_DISPLAY_FRAMES;
        memoryCard->action.busy = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_WAIT_LOAD_RESULT;
        break;

    case CARD_SLOT_ACTION_WAIT_LOAD_RESULT:
        if (!MemoryCardCountdownElapsed(&memoryCard->action.timer)) break;
        memoryCard->menuPage = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_PICK;
        memoryCard->menuRow = MemoryCardMenuRowCount(memoryCard) - 1;
        break;

    case CARD_SLOT_ACTION_SHOW_NO_FILE:
        memoryCard->phase = MC_PROMPT_NO_FILE;
        if (!CardResultPromptDismissed()) break;
        memoryCard->menuPage = 0;
        memoryCard->action.state = CARD_SLOT_ACTION_PICK;
        break;

    default:
        break;
    }
}
