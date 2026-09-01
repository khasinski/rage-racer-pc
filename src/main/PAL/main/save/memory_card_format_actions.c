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

void RunUnformattedCardPage(s32 fadeBusy) {
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
}
