#include "common.h"
#include "game/game_input.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/state.h"
#include "game/menu.h"
#include "psyq/gpu.h"
#include "game/audio.h"
#include "game/game_context.h"
#include "game/memory_card_controller.h"
#include "game/menu_runtime.h"

static void ApplyMemoryCardFeedback(
    u32 effects, u32 move, u32 accept, u32 back, u32 invalid, u32 exit) {
    u32 runtimeEffects = MENU_RUNTIME_EFFECT_NONE;

    if ((effects & move) != 0) runtimeEffects |= MENU_RUNTIME_EFFECT_MOVE;
    if ((effects & accept) != 0) runtimeEffects |= MENU_RUNTIME_EFFECT_ACCEPT;
    if ((effects & back) != 0) runtimeEffects |= MENU_RUNTIME_EFFECT_BACK;
    if ((effects & invalid) != 0)
        runtimeEffects |= MENU_RUNTIME_EFFECT_INVALID;
    if ((effects & exit) != 0) runtimeEffects |= MENU_RUNTIME_EFFECT_EXIT;
    MenuFlowApplyEffects(runtimeEffects);
}

static s32 UpdateMemoryCardFade(void) {
    s32 busy = 0;
    s32 step;

    if (g_SceneTimer == 2) SetDispMask(1);
    if ((u32)g_SceneTimer < 6) {
        DrawMenuFadeOverlay(g_McFadeLevel);
        return 0;
    }
    step = g_McFadeStep;
    if (step < 0) {
        g_McFadeLevel += step;
        busy = 1;
        if (g_McFadeLevel <= 0) {
            g_McFadeStep = 0;
            g_McFadeLevel = 0;
        }
    } else if (step > 0) {
        g_McActionBusy = 1;
        g_McFadeLevel += step;
        busy = 1;
        if (g_McFadeLevel >= 0xFF) {
            g_McFadeStep = 0;
            g_McFadeLevel = 0;
            g_McActionBusy = 0;
            GameSceneSet(SCENE_FRONTEND_ENTER);
        }
    }
    DrawMenuFadeOverlay(g_McFadeLevel);
    return busy;
}

static s32 AdvanceMemoryCardMenuStartup(void) {
    s32 next;
    if ((u32)g_SceneTimer >= 5) {
        g_SceneTimer++;
        return 1;
    }
    next = ++g_SceneTimer;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    if (next == 3) {
        g_McSlotUsedMask = 0;
        ClearSaveHeaderRows(g_McSaveHeaders);
        g_McLastMenuState = MC_MENU_NO_CARD;
        g_McMenuPhase = MC_PROMPT_NONE;
        g_McMenuSelection = next;
        g_McMenuState = MC_MENU_DETECTING_CARD;
        g_McActionState = MC_ACTION_IDLE;
        g_McActionResult = 0;
        g_McConfirmChoice = 0;
        g_McStateChangeCount = 0;
        g_McActionTimer = 0;
        g_McActionBusy = 0;
        g_McDrawEnabled = 1;
    }
    return 0;
}

static void ResolveMemoryCardMenuTransition(
    SaveSession *session) {
    session->menuState = g_McMenuState;
    session->selection = g_McMenuSelection;
    session->cardStatus = g_McCardStatus;
    session->lastMenuState = g_McLastMenuState;
    session->errorPending = g_McErrorPending;
    session->errorCountdown = g_McErrorCountdown;
    MemoryCardControllerResolveTransition(session);
    g_McMenuState = session->menuState;
    g_McLastMenuState = session->lastMenuState;
    g_McErrorPending = session->errorPending;
    g_McErrorCountdown = session->errorCountdown;
}

static MemoryCardActionSession CurrentMemoryCardAction(void) {
    MemoryCardActionSession action;
    action.state = (MemoryCardActionState)g_McActionState;
    action.timer = g_McActionTimer;
    action.settleTicks = g_McSettleTicks;
    action.busy = g_McActionBusy;
    action.menuPage = g_McMenuPage;
    action.menuRowCursor = g_McMenuRowCursor;
    action.prompt = g_McMenuPhase;
    return action;
}

static void ApplyMemoryCardAction(const MemoryCardActionSession *action) {
    g_McActionState = action->state;
    g_McActionTimer = action->timer;
    g_McSettleTicks = action->settleTicks;
    g_McActionBusy = action->busy;
    g_McMenuPage = action->menuPage;
    g_McMenuRowCursor = action->menuRowCursor;
    g_McMenuPhase = (MemoryCardPrompt)action->prompt;
}

static MemoryCardActionResult ReduceMemoryCardAction(
    MemoryCardActionEventType type, s32 value, s32 saveOperation) {
    MemoryCardActionSession action = CurrentMemoryCardAction();
    MemoryCardActionEvent event = {
        type, value, saveOperation, g_McMenuRowCount - 1};
    MemoryCardActionResult result = MemoryCardActionReduce(&action, &event);

    ApplyMemoryCardAction(&action);
    return result;
}

static void PerformMemoryCardSaveWrite(void) {
    s32 slot = g_McSlotCursor;
    s32 result;

    g_McMenuSubState = 5;
    result = WriteMemoryCardSaveSlot(slot, &g_McSaveHeaders[slot]);
    g_McActionResult = result;
    if (result != 0) {
        g_McActionOk = 1;
        g_McMenuSubState = 6;
    } else {
        g_McActionOk = 0;
        g_McMenuSubState = 0x10;
    }
    g_McSavedLoadPhase = GameMenuLoadPhase;
}

static void PerformMemoryCardSaveRefresh(void) {
    if (g_McActionResult != 0) {
        s32 result = RefreshMemoryCardSaveStatus(0, g_McSaveHeaders);
        g_McSlotUsedMask = result;
        if (result != 0) {
            if ((result & 0xFFFF) == 0) g_McMenuSubState = 0xE;
        } else {
            g_McMenuSubState = 0xC;
        }
        g_McSavedLoadPhase = GameMenuLoadPhase;
    }
}

static void PerformMemoryCardLoadRead(void) {
    s32 slot = g_McSlotCursor;

    g_McActionResult = LoadMemoryCardSaveSlot(
        slot, &g_McSaveHeaders[slot]);
    /* Retail reports the load sequence as completed in either branch; the
     * sub-state carries the actual success/error presentation. */
    g_McActionOk = 1;
    if (g_McActionResult != 0) {
        g_McMenuSubState = 8;
        g_McLastSlot = slot;
    } else {
        g_McMenuSubState = 0xF;
    }
    g_McActionTimer = 0x3C;
    g_McSavedLoadPhase = GameMenuLoadPhase;
}

static void UpdateMemoryCardActionTransaction(void) {
    MemoryCardActionResult result;

    if (g_McActionState == MC_ACTION_LOAD_PREPARE)
        g_McMenuSubState = 7;
    if (g_McActionState == MC_ACTION_SAVE_SHOW_RESULT) {
        ReduceMemoryCardAction(MC_ACTION_EVENT_IO_RESULT, g_McActionOk, 1);
        return;
    }
    if (g_McActionState == MC_ACTION_LOAD_SHOW_RESULT) {
        ReduceMemoryCardAction(MC_ACTION_EVENT_IO_RESULT, g_McActionOk, 0);
        return;
    }

    result = ReduceMemoryCardAction(MC_ACTION_EVENT_TICK, 0, 0);
    switch (result.effect) {
    case MC_ACTION_EFFECT_WRITE_SLOT:
        PerformMemoryCardSaveWrite();
        ReduceMemoryCardAction(MC_ACTION_EVENT_EFFECT_COMPLETE, 0, 0);
        break;
    case MC_ACTION_EFFECT_REFRESH_SLOTS:
        PerformMemoryCardSaveRefresh();
        ReduceMemoryCardAction(MC_ACTION_EVENT_EFFECT_COMPLETE, 0, 0);
        break;
    case MC_ACTION_EFFECT_LOAD_SLOT:
        PerformMemoryCardLoadRead();
        ReduceMemoryCardAction(MC_ACTION_EVENT_EFFECT_COMPLETE, 0, 0);
        break;
    case MC_ACTION_EFFECT_POLL_CARD:
        ReduceMemoryCardAction(
            MC_ACTION_EVENT_CARD_STATUS, PollMemoryCardStatus(0, 0), 0);
        break;
    case MC_ACTION_EFFECT_NONE:
        break;
    }
}

static MemoryCardReadSession CurrentMemoryCardRead(void) {
    MemoryCardReadSession read = {
        (MemoryCardReadState)g_McActionState,
        g_McActionTimer,
        g_McCardOkFrames,
        g_McActionElapsed,
        g_McActionBusy,
        g_McMenuSubState};
    return read;
}

static void ApplyMemoryCardRead(const MemoryCardReadSession *read) {
    g_McActionState = read->state;
    g_McActionTimer = read->timer;
    g_McCardOkFrames = read->cardOkFrames;
    g_McActionElapsed = read->elapsed;
    g_McActionBusy = read->busy;
    g_McMenuSubState = read->menuSubState;
}

static MemoryCardReadResult ReduceMemoryCardRead(
    MemoryCardReadEventType type, s32 refreshResult, s32 fadeBusy) {
    MemoryCardReadSession read = CurrentMemoryCardRead();
    MemoryCardReadEvent event = {
        type, g_SceneTimer, g_McCardStatus, refreshResult,
        g_GameInput.pressed, fadeBusy != 0};
    MemoryCardReadResult result = MemoryCardReadReduce(&read, &event);

    ApplyMemoryCardRead(&read);
    return result;
}

static MemoryCardFormatSession CurrentMemoryCardFormat(void) {
    MemoryCardFormatSession format = {
        (MemoryCardFormatState)g_McActionState,
        g_McActionTimer,
        g_McConfirmChoice,
        g_McActionBusy,
        g_McMenuPage,
        g_McMenuSubState,
        g_McMenuPhase,
        g_McMenuRowCursor,
        g_McSaveMode};
    return format;
}

static void ApplyMemoryCardFormat(const MemoryCardFormatSession *format) {
    g_McActionState = format->state;
    g_McActionTimer = format->timer;
    g_McConfirmChoice = format->confirmChoice;
    g_McActionBusy = format->busy;
    g_McMenuPage = format->menuPage;
    g_McMenuSubState = format->menuSubState;
    g_McMenuPhase = (MemoryCardPrompt)format->prompt;
    g_McMenuRowCursor = format->menuRowCursor;
    g_McSaveMode = format->saveMode;
}

static MemoryCardFormatResult ReduceMemoryCardFormat(
    MemoryCardFormatEventType type, s32 ioResult, s32 fadeBusy) {
    MemoryCardFormatSession format = CurrentMemoryCardFormat();
    MemoryCardFormatEvent event = {
        type, ioResult, g_McMenuRowCount, g_GameInput.pressed,
        g_GameInput.pressedRepeat, fadeBusy != 0};
    MemoryCardFormatResult result = MemoryCardFormatReduce(&format, &event);

    ApplyMemoryCardFormat(&format);
    return result;
}

static void ApplyMemoryCardFormatEffects(u32 effects) {
    ApplyMemoryCardFeedback(
        effects, MC_FORMAT_EFFECT_MOVE, MC_FORMAT_EFFECT_ACCEPT,
        MC_FORMAT_EFFECT_BACK, MC_FORMAT_EFFECT_INVALID,
        MC_FORMAT_EFFECT_EXIT);
}

static MemoryCardNoCardResult ReduceMemoryCardNoCard(s32 fadeBusy) {
    MemoryCardNoCardSession noCard = {
        (MemoryCardNoCardState)g_McActionState,
        g_McActionTimer,
        g_McMenuPage,
        g_McMenuRowCursor,
        g_McSlotUsedMask,
        g_McLastSlot};
    MemoryCardNoCardInput input = {
        g_GameInput.pressed, g_GameInput.pressedRepeat,
        g_McMenuRowCount, fadeBusy != 0};
    MemoryCardNoCardResult result =
        MemoryCardNoCardReduce(&noCard, &input);

    g_McActionState = noCard.state;
    g_McActionTimer = noCard.timer;
    g_McMenuPage = noCard.menuPage;
    g_McMenuRowCursor = noCard.menuRowCursor;
    g_McSlotUsedMask = noCard.slotUsedMask;
    g_McLastSlot = noCard.lastSlot;
    if ((result.effects & MC_NO_CARD_EFFECT_CLEAR_SLOTS) != 0)
        ClearSaveHeaderRows(g_McSaveHeaders);
    ApplyMemoryCardFeedback(
        result.effects, MC_NO_CARD_EFFECT_MOVE, MC_NO_CARD_EFFECT_ACCEPT,
        MC_NO_CARD_EFFECT_BACK, MC_NO_CARD_EFFECT_INVALID,
        MC_NO_CARD_EFFECT_EXIT);
    return result;
}

static s32 MemoryCardReadyHandlesCurrentState(void) {
    if (g_McMenuPage == MC_PAGE_MODE_SELECT) return 1;
    return g_McActionState == MC_ACTION_IDLE ||
        g_McActionState == MC_ACTION_CONFIRM_OVERWRITE ||
        g_McActionState == MC_ACTION_CARD_FULL ||
        g_McActionState == MC_ACTION_NO_FILE;
}

static void ReduceMemoryCardReady(s32 fadeBusy) {
    MemoryCardReadySession ready = {
        (MemoryCardPage)g_McMenuPage,
        (MemoryCardActionState)g_McActionState,
        g_McMenuRowCursor,
        g_McSlotCursor,
        g_McSaveMode,
        g_McConfirmChoice,
        g_McActionTimer,
        g_McMenuPhase};
    MemoryCardReadyInput input = {
        g_GameInput.pressed, g_GameInput.pressedRepeat,
        g_McMenuRowCount, g_McSlotUsedMask, g_McFreeBlocks,
        g_McLastSlot, fadeBusy != 0};
    MemoryCardReadyResult result = MemoryCardReadyReduce(&ready, &input);

    g_McMenuPage = ready.page;
    g_McActionState = ready.actionState;
    g_McMenuRowCursor = ready.menuRowCursor;
    g_McSlotCursor = ready.slotCursor;
    g_McSaveMode = ready.saveMode;
    g_McConfirmChoice = ready.confirmChoice;
    g_McActionTimer = ready.timer;
    g_McMenuPhase = (MemoryCardPrompt)ready.prompt;
    if ((result.effects & MC_READY_EFFECT_EXIT) != 0) {
        g_McActionBusy = 0;
    }
    ApplyMemoryCardFeedback(
        result.effects, MC_READY_EFFECT_MOVE, MC_READY_EFFECT_ACCEPT,
        MC_READY_EFFECT_BACK, MC_READY_EFFECT_INVALID,
        MC_READY_EFFECT_EXIT);
}

void UpdateMemoryCardMenu(void) {
    s32 fadeBusy;
    s32 tmp;
    SaveSession session;

    fadeBusy = UpdateMemoryCardFade();
    if (!AdvanceMemoryCardMenuStartup()) goto menu_state_update_done;
    session = (SaveSession){
        g_McMenuState, g_McMenuSelection, g_McMenuSubState, g_McCardStatus,
        g_McNoCardTicks, g_McErrorTicks, g_McLastMenuState,
        g_McErrorPending, g_McErrorCountdown};
    if (MemoryCardControllerShouldPoll(g_McActionBusy, g_McErrorPending))
        MemoryCardControllerApplyStatus(
            &session, PollMemoryCardStatus(0, 0));
    g_McCardStatus = session.cardStatus;
    g_McNoCardTicks = session.noCardTicks;
    g_McMenuSubState = session.subState;
    g_McMenuSelection = session.selection;

    switch (g_McMenuState) {

    case MC_MENU_DETECTING_CARD:
    {
    u16 lpad = g_GameInput.pressed;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    g_McActionBusy = 0;
    if ((lpad & 0x90) && !fadeBusy) {
        PlaySoundCue(3);
        StartMenuExitFade();
    }
    }
    session.menuState = g_McMenuState;
    session.selection = g_McMenuSelection;
    session.cardStatus = g_McCardStatus;
    session.errorTicks = g_McErrorTicks;
    MemoryCardControllerResolveDetection(&session);
    g_McMenuState = session.menuState;
    g_McErrorTicks = session.errorTicks;
    break;

    case MC_MENU_READY:
    if (MemoryCardReadyHandlesCurrentState()) {
        ReduceMemoryCardReady(fadeBusy);
    } else if (g_McMenuPage == MC_PAGE_SLOT_ACTION) {
        UpdateMemoryCardActionTransaction();
    } else {
        {
            s32 cm1 = g_McMenuRowCount;
            g_McMenuPage = MC_PAGE_MODE_SELECT;
            g_McSlotCursor = 0;
            g_McActionState = MC_ACTION_IDLE;
            g_McActionBusy = 0;
            g_McActionResult = 0;
            g_McConfirmChoice = 0;
            g_McActionTimer = 0;
            cm1 = cm1 - 1;
            g_McMenuRowCursor = cm1;
        }
    }
    ResolveMemoryCardMenuTransition(&session);
    if (!(g_McMenuState == 1)) {
    g_McActionState = MC_ACTION_IDLE;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    g_McActionBusy = 0;
    }
    break;

    case MC_MENU_READING_CARD:
    {
        MemoryCardReadResult readResult;

        g_McMenuSubState = 1;
        g_McMenuPhase = MC_PROMPT_ACCESSING;
        readResult = ReduceMemoryCardRead(MC_READ_EVENT_TICK, 0, fadeBusy);
        if (readResult.effect == MC_READ_EFFECT_EXIT) {
            PlaySoundCue(3);
            StartMenuExitFade();
        } else if (readResult.effect == MC_READ_EFFECT_REFRESH_SLOTS) {
            s32 refreshResult =
                RefreshMemoryCardSaveStatus(1, g_McSaveHeaders);
            g_McSlotUsedMask = refreshResult;
            ReduceMemoryCardRead(
                MC_READ_EVENT_REFRESH_RESULT, refreshResult, fadeBusy);
            g_McSavedLoadPhase = GameMenuLoadPhase;
        }
        if (readResult.complete) g_McMenuState = g_McMenuSelection;
    }

    ResolveMemoryCardMenuTransition(&session);

    if (g_McMenuState == 2) break;
    g_McMenuSubState = 1;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    g_McActionState = MC_READ_WAIT_SCENE;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    break;
    case MC_MENU_NO_CARD:
    g_McMenuSubState = 0xA;
    g_McMenuPhase = MC_PROMPT_NO_CARD;
    g_McActionBusy = 0;
    ReduceMemoryCardNoCard(fadeBusy);
    ResolveMemoryCardMenuTransition(&session);

    if (!(g_McMenuState == -1)) {
    g_McActionState = MC_NO_CARD_PREPARE;
    }
    break;
    case MC_MENU_NEW_CARD:
    {
        MemoryCardFormatResult formatResult =
            ReduceMemoryCardFormat(MC_FORMAT_EVENT_TICK, 0, fadeBusy);

        if ((formatResult.effects & MC_FORMAT_EFFECT_FORMAT) != 0) {
            g_McActionResult = FormatMemoryCard(0, 0);
            formatResult = ReduceMemoryCardFormat(
                MC_FORMAT_EVENT_IO_RESULT, g_McActionResult, fadeBusy);
        }
        ApplyMemoryCardFormatEffects(formatResult.effects);
    }
    ResolveMemoryCardMenuTransition(&session);

    if (!(g_McMenuState == -2)) {
    g_McActionState = MC_FORMAT_IDLE;
    g_McActionBusy = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    break;
    case MC_MENU_CARD_ERROR:
    default:
    g_McMenuSubState = 0x11;
    {
    u16 lpad = g_GameInput.pressed;
    g_McMenuPhase = MC_PROMPT_CARD_ERROR;
    if ((lpad & 0x90) && !fadeBusy) {
        PlaySoundCue(3);
        g_McActionBusy = 0;
        StartMenuExitFade();
    }
    }
    {
        s32 sel = g_McMenuSelection;
        s32 three = 3;
        if (sel == -3) break;
        if (sel == three) {
            g_McLastMenuState = g_McMenuState;
        }
        tmp = g_McStateChangeCount;
        g_McMenuState = sel;
        g_McStateChangeCount = tmp + 1;
        if (g_McErrorPending != 0) {
            g_McErrorPending = 0;
            g_McErrorCountdown = three;
        }
    }

    }
    }
menu_state_update_done:
    if (g_McDrawEnabled != 0) {
        DrawMemoryCardScreen(g_McMenuPage, g_McFromLoadMenu, g_McMenuRowCursor, g_McSlotCursor);
        if (g_McMenuPhase != 0) {
            DrawMemoryCardMessage(g_McMenuPhase - 1);
        }
        DrawMemoryCardSaveRows(g_McSlotUsedMask, g_McSaveHeaders);
    }
}
