#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

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

enum {
    FORMAT_CARD_DELAY_FRAMES = 20,
    FORMAT_RESULT_DISPLAY_FRAMES = 60,
};

static void ReturnToUnformattedCardRoot(MemoryCardSession *memoryCard) {
    memoryCard->menuPage = 0;
    memoryCard->action.state = FORMAT_CARD_ACTION_PROMPT;
}

static void RunUnformattedCardRootPage(MemoryCardSession *memoryCard, s32 fadeBusy) {
    memoryCard->phase = MC_PROMPT_NONE;
    AdjustMenuSelectionVertical(&memoryCard->menuRow, 0,
                                MemoryCardMenuRowCount(memoryCard) - 1);

    if (!(g_PadPressed & PAD_CONFIRM)) {
        if ((g_PadPressed & PAD_CANCEL) && !fadeBusy) {
            PlaySoundCue(3);
            memoryCard->action.busy = 0;
            StartMenuExitFade(memoryCard);
        }
        return;
    }

    if (memoryCard->menuRow == 0) {
        PlaySoundCue(2);
        memoryCard->menuPage = 1;
        memoryCard->action.confirmChoice = 0;
        memoryCard->saveMode = 0;
    } else if (memoryCard->menuRow == MemoryCardMenuRowCount(memoryCard) - 1) {
        if (fadeBusy) return;
        PlaySoundCue(2);
        memoryCard->action.busy = 0;
        StartMenuExitFade(memoryCard);
    } else {
        PlaySoundCue(5);
        memoryCard->menuPage = 1;
        memoryCard->saveMode = memoryCard->menuRow;
    }
}

static void RunFormatPrompt(MemoryCardSession *memoryCard) {
    if (memoryCard->saveMode != 0) {
        memoryCard->phase = MC_PROMPT_NO_DATA;
        if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
            ReturnToUnformattedCardRoot(memoryCard);
        }
        return;
    }

    memoryCard->phase = MC_PROMPT_NEW_CARD;
    if (PollMenuConfirmInput() != 0) {
        memoryCard->action.state = FORMAT_CARD_ACTION_CONFIRM;
    } else if (PollMenuBackInput() != 0) {
        ReturnToUnformattedCardRoot(memoryCard);
    }
}

static void RunFormatConfirmation(MemoryCardSession *memoryCard) {
    u16 confirm;

    memoryCard->phase = memoryCard->action.confirmChoice + MC_PROMPT_FORMAT_ASK;
    SetMenuBinaryChoiceHorizontal(&memoryCard->action.confirmChoice);
    confirm = PollMenuConfirmInput();
    if (memoryCard->action.confirmChoice != 0 && confirm != 0) {
        memoryCard->action.state = FORMAT_CARD_ACTION_BEGIN_DELAY;
    } else if (confirm != 0 || PollMenuBackInput() != 0) {
        ReturnToUnformattedCardRoot(memoryCard);
    }
}

static void RunFormatOperation(MemoryCardSession *memoryCard) {
    memoryCard->action.result = FormatMemoryCard(0, 0);
    if (memoryCard->action.result == 1) {
        memoryCard->action.state = FORMAT_CARD_ACTION_SHOW_SUCCESS;
        memoryCard->action.timer = FORMAT_RESULT_DISPLAY_FRAMES;
    } else {
        memoryCard->action.state = FORMAT_CARD_ACTION_SHOW_ERROR;
    }
}

static void RunFormatCardActions(MemoryCardSession *memoryCard, s32 fadeBusy) {
    switch (memoryCard->action.state) {
    case FORMAT_CARD_ACTION_PROMPT:
        RunFormatPrompt(memoryCard);
        break;

    case FORMAT_CARD_ACTION_CONFIRM:
        RunFormatConfirmation(memoryCard);
        break;

    case FORMAT_CARD_ACTION_BEGIN_DELAY:
        memoryCard->action.busy = 1;
        memoryCard->action.timer = FORMAT_CARD_DELAY_FRAMES;
        memoryCard->action.state = FORMAT_CARD_ACTION_WAIT_DELAY;
        break;

    case FORMAT_CARD_ACTION_WAIT_DELAY:
        if (MemoryCardCountdownElapsed(&memoryCard->action.timer)) {
            memoryCard->action.state = FORMAT_CARD_ACTION_RUN;
        }
        break;

    case FORMAT_CARD_ACTION_RUN:
        RunFormatOperation(memoryCard);
        break;

    case FORMAT_CARD_ACTION_SHOW_SUCCESS:
        memoryCard->phase = MC_PROMPT_FORMAT_OK;
        if (MemoryCardCountdownElapsed(&memoryCard->action.timer)) {
            memoryCard->action.busy = 0;
            memoryCard->action.state = FORMAT_CARD_ACTION_WAIT_TO_EXIT;
        }
        break;

    case FORMAT_CARD_ACTION_WAIT_TO_EXIT:
        memoryCard->phase = MC_PROMPT_FORMAT_OK;
        if (!(g_PadPressed & PAD_CANCEL)) break;
        memoryCard->action.busy = 0;
        memoryCard->action.state = FORMAT_CARD_ACTION_PROMPT;
        memoryCard->action.result = 0;
        memoryCard->action.confirmChoice = 0;
        memoryCard->action.timer = 0;
        if (!fadeBusy) {
            PlaySoundCue(3);
            StartMenuExitFade(memoryCard);
        }
        break;

    case FORMAT_CARD_ACTION_SHOW_ERROR:
        memoryCard->phase = MC_PROMPT_CARD_ERROR;
        memoryCard->action.busy = 0;
        if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
            memoryCard->action.state = FORMAT_CARD_ACTION_PROMPT;
        }
        break;

    default:
        break;
    }
}

void RunUnformattedCardPage(MemoryCardSession *memoryCard, s32 fadeBusy) {
    switch (memoryCard->menuPage) {
    case 0:
        RunUnformattedCardRootPage(memoryCard, fadeBusy);
        break;
    case 1:
        RunFormatCardActions(memoryCard, fadeBusy);
        break;
    default:
        break;
    }
}
