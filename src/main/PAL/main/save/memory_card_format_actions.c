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

static void ReturnToUnformattedCardRoot(MemoryCardAction *action) {
    g_McMenuPage = 0;
    action->state = FORMAT_CARD_ACTION_PROMPT;
}

static void RunUnformattedCardRootPage(MemoryCardAction *action, s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_NONE;
    AdjustMenuSelectionVertical(&g_McMenuRowCursor, 0,
                                MemoryCardMenuRowCount() - 1);

    if (!(g_PadPressed & PAD_CONFIRM)) {
        if ((g_PadPressed & PAD_CANCEL) && !fadeBusy) {
            PlaySoundCue(3);
            action->busy = 0;
            StartMenuExitFade();
        }
        return;
    }

    if (g_McMenuRowCursor == 0) {
        PlaySoundCue(2);
        g_McMenuPage = 1;
        action->confirmChoice = 0;
        g_McSaveMode = 0;
    } else if (g_McMenuRowCursor == MemoryCardMenuRowCount() - 1) {
        if (fadeBusy) return;
        PlaySoundCue(2);
        action->busy = 0;
        StartMenuExitFade();
    } else {
        PlaySoundCue(5);
        g_McMenuPage = 1;
        g_McSaveMode = g_McMenuRowCursor;
    }
}

static void RunFormatPrompt(MemoryCardAction *action) {
    if (g_McSaveMode != 0) {
        g_McMenuPhase = MC_PROMPT_NO_DATA;
        if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
            ReturnToUnformattedCardRoot(action);
        }
        return;
    }

    g_McMenuPhase = MC_PROMPT_NEW_CARD;
    if (PollMenuConfirmInput() != 0) {
        action->state = FORMAT_CARD_ACTION_CONFIRM;
    } else if (PollMenuBackInput() != 0) {
        ReturnToUnformattedCardRoot(action);
    }
}

static void RunFormatConfirmation(MemoryCardAction *action) {
    u16 confirm;

    g_McMenuPhase = action->confirmChoice + MC_PROMPT_FORMAT_ASK;
    SetMenuBinaryChoiceHorizontal(&action->confirmChoice);
    confirm = PollMenuConfirmInput();
    if (action->confirmChoice != 0 && confirm != 0) {
        action->state = FORMAT_CARD_ACTION_BEGIN_DELAY;
    } else if (confirm != 0 || PollMenuBackInput() != 0) {
        ReturnToUnformattedCardRoot(action);
    }
}

static void RunFormatOperation(MemoryCardAction *action) {
    action->result = FormatMemoryCard(0, 0);
    if (action->result == 1) {
        action->state = FORMAT_CARD_ACTION_SHOW_SUCCESS;
        action->timer = FORMAT_RESULT_DISPLAY_FRAMES;
    } else {
        action->state = FORMAT_CARD_ACTION_SHOW_ERROR;
    }
}

static void RunFormatCardActions(MemoryCardAction *action, s32 fadeBusy) {
    switch (action->state) {
    case FORMAT_CARD_ACTION_PROMPT:
        RunFormatPrompt(action);
        break;

    case FORMAT_CARD_ACTION_CONFIRM:
        RunFormatConfirmation(action);
        break;

    case FORMAT_CARD_ACTION_BEGIN_DELAY:
        action->busy = 1;
        action->timer = FORMAT_CARD_DELAY_FRAMES;
        action->state = FORMAT_CARD_ACTION_WAIT_DELAY;
        break;

    case FORMAT_CARD_ACTION_WAIT_DELAY:
        if (MemoryCardCountdownElapsed(&action->timer)) {
            action->state = FORMAT_CARD_ACTION_RUN;
        }
        break;

    case FORMAT_CARD_ACTION_RUN:
        RunFormatOperation(action);
        break;

    case FORMAT_CARD_ACTION_SHOW_SUCCESS:
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        if (MemoryCardCountdownElapsed(&action->timer)) {
            action->busy = 0;
            action->state = FORMAT_CARD_ACTION_WAIT_TO_EXIT;
        }
        break;

    case FORMAT_CARD_ACTION_WAIT_TO_EXIT:
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        if (!(g_PadPressed & PAD_CANCEL)) break;
        action->busy = 0;
        action->state = FORMAT_CARD_ACTION_PROMPT;
        action->result = 0;
        action->confirmChoice = 0;
        action->timer = 0;
        if (!fadeBusy) {
            PlaySoundCue(3);
            StartMenuExitFade();
        }
        break;

    case FORMAT_CARD_ACTION_SHOW_ERROR:
        g_McMenuPhase = MC_PROMPT_CARD_ERROR;
        action->busy = 0;
        if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
            action->state = FORMAT_CARD_ACTION_PROMPT;
        }
        break;

    default:
        break;
    }
}

void RunUnformattedCardPage(MemoryCardAction *action, s32 fadeBusy) {
    switch (g_McMenuPage) {
    case 0:
        RunUnformattedCardRootPage(action, fadeBusy);
        break;
    case 1:
        RunFormatCardActions(action, fadeBusy);
        break;
    default:
        break;
    }
}
