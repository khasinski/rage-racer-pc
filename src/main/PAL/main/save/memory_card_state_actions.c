#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

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

enum {
    CARD_WORK_START_FRAME = 31,
    CARD_WORK_CANCEL_DELAY_FRAMES = 121,
    CARD_WORK_READY_FRAMES = 2,
    CARD_WORK_DELAY_FRAMES = 5,
};

/*
 * A format or a save running, stepping through its own stages while the
 * screen says it is busy.
 */
void RunCardWorkingActions(s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    switch (g_McActionState) {
    case CARD_WORK_WAIT_FOR_SCENE:
        if ((u32)g_SceneTimer < CARD_WORK_START_FRAME) break;
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        g_McActionState = CARD_WORK_WAIT_FOR_CARD;
        break;
    case CARD_WORK_WAIT_FOR_CARD:
        g_McActionBusy = 0;
        g_McActionElapsed++;
        if ((g_PadPressed & PAD_CANCEL) &&
            g_McActionElapsed >= CARD_WORK_CANCEL_DELAY_FRAMES) {
            g_McCardOkFrames = 0;
            g_McActionElapsed = 0;
            if (fadeBusy == 0) {
                PlaySoundCue(3);
                StartMenuExitFade();
            }
        }
        if (g_McCardStatus != MC_MENU_STATE_READY) break;
        g_McCardOkFrames++;
        if (g_McCardOkFrames < CARD_WORK_READY_FRAMES) break;
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        g_McActionState = CARD_WORK_BEGIN_STATUS_DELAY;
        break;
    case CARD_WORK_BEGIN_STATUS_DELAY:
        g_McActionBusy = 1;
        g_McActionTimer = CARD_WORK_DELAY_FRAMES;
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
        g_McActionTimer = CARD_WORK_DELAY_FRAMES;
        g_McActionState = CARD_WORK_WAIT_SETTLE_DELAY;
        break;
    case CARD_WORK_WAIT_SETTLE_DELAY:
        if (--g_McActionTimer != 0) break;
        g_McActionTimer = CARD_WORK_DELAY_FRAMES;
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
}

/*
 * Nothing in the slot.
 */
typedef enum NoCardActionState {
    NO_CARD_ACTION_INIT = 0,
    NO_CARD_ACTION_WAIT = 1,
    NO_CARD_ACTION_READY = 3,
} NoCardActionState;

enum { NO_CARD_READY_DELAY_FRAMES = 5 };

void RunNoCardActions(s32 fadeBusy) {
    g_McMenuPhase = MC_PROMPT_NO_CARD;
    g_McActionBusy = 0;
    switch (g_McActionState) {
    case NO_CARD_ACTION_INIT:
        g_McActionTimer = NO_CARD_READY_DELAY_FRAMES;
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
}
