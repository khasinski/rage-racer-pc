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
    g_McActionState = MC_ACTION_SAVE_POST_WRITE;
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
    g_McActionState = MC_ACTION_SAVE_SETTLE_PREPARE;
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
    g_McActionState = MC_ACTION_LOAD_SETTLE_PREPARE;
    g_McSavedLoadPhase = GameMenuLoadPhase;
}

void UpdateMemoryCardMenu(void) {
    s32 fadeBusy;
    s32 tmp;
    u16 pad;
    s32 wtmp;
    s32 mst;
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
    switch (g_McMenuPage) {
    case MC_PAGE_MODE_SELECT:
    {
        s32 *p = &g_McMenuRowCursor;
        g_McMenuPhase = MC_PROMPT_NONE;
        AdjustMenuSelectionHorizontal(p, 0, g_McMenuRowCount - 1);
        pad = g_GameInput.pressed;
        if (!((pad & 0x860) == 0)) {
        if (*p < g_McMenuRowCount - 1) {
            PlaySoundCue(2);
            g_McMenuPage = MC_PAGE_SLOT_ACTION;
            g_McActionState = MC_ACTION_IDLE;
            g_McActionResult = 0;
            g_McSlotCursor = g_McLastSlot;
            g_McSaveMode = *p;
            break;
        }
        if (fadeBusy) break;
        PlaySoundCue(2);
        } else {
        if ((pad & 0x90) == 0 || fadeBusy) break;
        PlaySoundCue(3);
        }
        g_McActionBusy = 0;
        StartMenuExitFade();
    }
    break;

    case MC_PAGE_SLOT_ACTION:
    switch (g_McActionState) {
    case MC_ACTION_IDLE: {
        s32 *s0 = &g_McSlotCursor;
        s32 a0;
        s32 nv;
        AdjustMenuSelectionHorizontal(s0, 0, 2);
        if (!(g_McSaveMode == 0)) {
        a0 = g_McSlotUsedMask;
        if (!((a0 % 8) == 0)) {
        g_McMenuPhase = MC_PROMPT_SELECT_LOAD;
        if ((g_GameInput.pressed & PAD_CONFIRM) == 0) goto slot_prompt_done;
        if (!(((a0 >> *s0) & 1) == 0)) {
        PlaySoundCue(2);
        g_McConfirmChoice = 0;
        nv = MC_ACTION_LOAD_PREPARE;
        } else {
        PlaySoundCue(5);
        nv = MC_ACTION_NO_FILE;
        }
        goto L_b475;
        }
        g_McMenuPhase = MC_PROMPT_NO_DATA;
        if ((g_GameInput.pressed & PAD_CONFIRM) == 0) goto slot_prompt_done;
        } else {
        if (g_McFreeBlocks != 0) goto L_b448;
        a0 = g_McSlotUsedMask;
        if (!((a0 % 8) == 0)) {
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if ((g_GameInput.pressed & PAD_CONFIRM) == 0) goto slot_prompt_done;
        if (!(((a0 >> *s0) & 1) == 0)) {
        PlaySoundCue(2);
        g_McConfirmChoice = 0;
        nv = MC_ACTION_CONFIRM_OVERWRITE;
        } else {
        PlaySoundCue(2);
        nv = MC_ACTION_CARD_FULL;
        }
        goto L_b475;
        }
        g_McMenuPhase = MC_PROMPT_CARD_FULL;
        if ((g_GameInput.pressed & PAD_CONFIRM) == 0) goto L_b439;
        }
        PlaySoundCue(5);
        g_McMenuPage = MC_PAGE_MODE_SELECT;
        goto slot_prompt_done;
    L_b439:
        if (!((PollMenuBackInput() & 0xFFFF) == 0)) {
        g_McMenuPage = MC_PAGE_MODE_SELECT;
        goto slot_prompt_done;
    L_b448:
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if (!((g_GameInput.pressed & PAD_CONFIRM) == 0)) {
        if (!(((g_McSlotUsedMask >> *s0) & 1) == 0)) {
        PlaySoundCue(2);
        g_McConfirmChoice_v = 0;
        nv = MC_ACTION_CONFIRM_OVERWRITE;
        } else {
        PlaySoundCue(2);
        g_McActionTimer = 0x1E;
        nv = MC_ACTION_SAVE_PREPARE;
        }
    L_b475:
        g_McActionState = nv;
        }
        }
slot_prompt_done:
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        g_McMenuPage = MC_PAGE_MODE_SELECT;
        break;
    }

    case MC_ACTION_CONFIRM_OVERWRITE: {
        s32 *p = &g_McConfirmChoice;
        s32 hi = g_McSlotCursor * 2;
        s32 lo = g_McConfirmChoice + 9;
        g_McMenuPhase = hi + lo;
        SetMenuBinaryChoiceVertical(p);
        if (!(g_McConfirmChoice == 0)) {
        if (!((PollMenuConfirmInput() & 0xFFFF) == 0)) {
            g_McActionState = MC_ACTION_SAVE_PREPARE;
        break;
        }
        if (g_McConfirmChoice != 0) {
            if ((PollMenuBackInput() & 0xFFFF) == 0) break;
            g_McActionState = MC_ACTION_IDLE;
            break;
        }
        }
        if ((PollMenuConfirmInput() & 0xFFFF) != 0) {
            g_McActionState = MC_ACTION_IDLE;
            break;
        }
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        g_McActionState = MC_ACTION_IDLE;
        break;
    }

    case MC_ACTION_SAVE_PREPARE:
        g_McMenuPhase = MC_PROMPT_ACCESSING;
        g_McActionTimer = 0xA;
        g_McActionState = MC_ACTION_SAVE_DELAY;
        break;

    case MC_ACTION_SAVE_DELAY: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        g_McActionBusy = 1;
        action.busy = 1;
        MemoryCardActionTick(&action, g_McMenuRowCount - 1);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_SAVE_WRITE: {
        PerformMemoryCardSaveWrite();
        break;
    }

    case MC_ACTION_SAVE_POST_WRITE:
        g_McActionState = MC_ACTION_SAVE_REFRESH;
        break;

    case MC_ACTION_SAVE_REFRESH: {
        PerformMemoryCardSaveRefresh();
        break;
    }

    case MC_ACTION_SAVE_SETTLE_PREPARE:
        g_McActionTimer = 5;
        g_McActionState = MC_ACTION_SAVE_SETTLE_DELAY;
        break;

    case MC_ACTION_SAVE_SETTLE_DELAY: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionTick(&action, g_McMenuRowCount - 1);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_SAVE_WAIT_CARD: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionObserveCard(&action, PollMemoryCardStatus(0, 0));
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_SAVE_SHOW_RESULT: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionShowResult(&action, 1, g_McActionOk);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_SAVE_RESULT_DELAY: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionTick(&action, g_McMenuRowCount - 1);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_CARD_FULL:
        g_McMenuPhase = MC_PROMPT_CARD_FULL;
        if (!((PollMenuConfirmInput() & 0xFFFF) != 0)) {
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        }
        g_McMenuPage = MC_PAGE_MODE_SELECT;
        g_McActionState = MC_ACTION_IDLE;
        break;

    case MC_ACTION_LOAD_PREPARE:
        g_McMenuSubState = 7;
        g_McActionTimer = 5;
        g_McActionState = MC_ACTION_LOAD_INITIAL_DELAY;
        break;

    case MC_ACTION_LOAD_INITIAL_DELAY: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionTick(&action, g_McMenuRowCount - 1);
        ApplyMemoryCardAction(&action);
        break;
    }

    {
        register s32 stateValue asm("$3");

    case MC_ACTION_LOAD_ACCESS_PREPARE: {
        tmp = 0xF;
        stateValue = 1;
        g_McMenuPhase = tmp;
        g_McActionTimer = tmp;
        g_McActionBusy = stateValue;
        g_McActionState = MC_ACTION_LOAD_ACCESS_DELAY;
        break;
    }

    case MC_ACTION_LOAD_ACCESS_DELAY: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionTick(&action, g_McMenuRowCount - 1);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_LOAD_READ: {
        PerformMemoryCardLoadRead();
        break;
    }
    }

    case MC_ACTION_LOAD_SETTLE_PREPARE:
        g_McActionTimer = 5;
        g_McActionState = MC_ACTION_LOAD_SETTLE_DELAY;
        break;

    case MC_ACTION_LOAD_SETTLE_DELAY: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionTick(&action, g_McMenuRowCount - 1);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_LOAD_WAIT_CARD: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionObserveCard(&action, PollMemoryCardStatus(0, 0));
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_LOAD_SHOW_RESULT: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionShowResult(&action, 0, g_McActionOk);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_LOAD_RESULT_DELAY: {
        MemoryCardActionSession action = CurrentMemoryCardAction();
        MemoryCardActionTick(&action, g_McMenuRowCount - 1);
        ApplyMemoryCardAction(&action);
        break;
    }

    case MC_ACTION_NO_FILE:
        g_McMenuPhase = MC_PROMPT_NO_FILE;
        if (!((PollMenuConfirmInput() & 0xFFFF) != 0)) {
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        }
        g_McMenuPage = MC_PAGE_MODE_SELECT;
        g_McActionState = MC_ACTION_IDLE;
        break;

    default:
    }
    break;

    default:
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
    g_McMenuSubState = 1;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    switch (g_McActionState) {
    case MC_READ_WAIT_SCENE:
        {
            u32 sceneFrame = g_SceneTimer;
            if (sceneFrame < 0x1F) break;
            wtmp = MC_READ_WAIT_CARD;
            g_McCardOkFrames = 0;
            g_McActionElapsed = 0;
            g_McActionState = wtmp;
        }
        break;
    case MC_READ_WAIT_CARD:
        g_McActionBusy = 0;
        {
            s32 t = g_McActionElapsed + 1;
            g_McActionElapsed = t;
            if (!((g_GameInput.pressed & PAD_CANCEL) == 0)) {
            if (!(t < 0x79)) {
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        if (!(fadeBusy != 0)) {
        PlaySoundCue(3);
        StartMenuExitFade();
        }
            }
            }
        }
        if (g_McCardStatus != 1) break;
        g_McCardOkFrames += 1;
        if (g_McCardOkFrames < 2) break;
        wtmp = MC_READ_PREPARE;
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        g_McActionState = wtmp;
        break;
    case MC_READ_PREPARE:
        g_McActionBusy = 1;
        g_McActionTimer = 5;
        g_McActionState = MC_READ_DELAY;
        break;
    case MC_READ_DELAY:
        {
            s32 t = g_McActionTimer;
            g_McActionTimer = t - 1;
            if (g_McActionTimer != 0) break;
        }
        g_McActionState = MC_READ_REFRESH;
        break;
    case MC_READ_REFRESH:
        {
            s32 x = RefreshMemoryCardSaveStatus(1, g_McSaveHeaders);
            s32 w;
            g_McSlotUsedMask = x;
            if (!(x == 0)) {
                x = x & 7;
                if (x != 0) {
                    x = 2;
                } else {
                    x = 0xE;
                }
            } else {
                x = 0xC;
            }
            g_McMenuSubState = x;
            w = GameMenuLoadPhase;
            g_McActionState = MC_READ_POST_REFRESH;
            g_McSavedLoadPhase = w;
        }
        break;
    case MC_READ_POST_REFRESH:
        g_McActionTimer = 5;
        g_McActionState = MC_READ_SETTLE_PREPARE;
        break;
    case MC_READ_SETTLE_PREPARE:
        {
            s32 t = g_McActionTimer;
            g_McActionTimer = t - 1;
            if (g_McActionTimer != 0) break;
        }
        g_McActionTimer = 5;
        g_McActionBusy = 0;
        g_McActionState = MC_READ_SETTLE_DELAY;
        break;
    case MC_READ_SETTLE_DELAY:
        {
            s32 t = g_McActionTimer;
            g_McActionTimer = t - 1;
            if (g_McActionTimer != 0) break;
        }
        g_McActionState = MC_READ_COMPLETE;
        break;
    case MC_READ_COMPLETE:
        if (g_McMenuSelection != 1) break;
        g_McMenuState = g_McMenuSelection;
        break;
    default:
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
    mst = g_McActionState;
    switch (mst) {
    case MC_NO_CARD_PREPARE:
    g_McActionTimer = 5;
    g_McSlotUsedMask = 0;
    ClearSaveHeaderRows(g_McSaveHeaders);
    g_McLastSlot = 0;
    g_McActionState = MC_NO_CARD_DELAY;
    break;

    case MC_NO_CARD_DELAY:
    g_McActionTimer -= 1;
    if (!(g_McActionTimer != 0)) {
    g_McActionState = MC_NO_CARD_INPUT;
    }
    break;

    case MC_NO_CARD_INPUT:
    {
        s32 mph = g_McMenuPage;
        if (!(mph == MC_PAGE_MODE_SELECT)) {
        if (mph == MC_PAGE_SLOT_ACTION) goto L_b1280;
        break;
        }
    }

    {
        s32 *mp = &g_McMenuRowCursor;
        AdjustMenuSelectionHorizontal(mp, 0, g_McMenuRowCount - 1);
        if (!(PollMenuConfirmInput() == 0)) {
        if (!(*mp != g_McMenuRowCount - 1)) {
    if (fadeBusy != 0) break;
    g_McActionState = MC_NO_CARD_PREPARE;
    PlaySoundCue(2);
    StartMenuExitFade();
    break;
        }

    PlaySoundCue(5);
    break;
        }
    }

    if (!((g_GameInput.pressed & PAD_CANCEL) == 0)) {
    if (!(fadeBusy != 0)) {
    g_McActionState = MC_NO_CARD_PREPARE;
    PlaySoundCue(3);
    StartMenuExitFade();
    break;

L_b1280:
    if (!((g_GameInput.pressed & PAD_CANCEL) == 0)) {
    if (!(fadeBusy != 0)) {
    PlaySoundCue(3);
    StartMenuExitFade();
    /* fall through */
    }
    }
    }
    }

    break;

    default:
    break;
    }
    ResolveMemoryCardMenuTransition(&session);

    if (!(g_McMenuState == -1)) {
    g_McActionState = MC_NO_CARD_PREPARE;
    }
    break;
    case MC_MENU_NEW_CARD:
    switch (g_McMenuPage) {
    case MC_PAGE_MODE_SELECT:
    {
        s32 *p = &g_McMenuRowCursor;
        g_McMenuSubState = 0xB;
        g_McMenuPhase = MC_PROMPT_NONE;
        AdjustMenuSelectionHorizontal(p, 0, g_McMenuRowCount - 1);
        pad = g_GameInput.pressed;
        if (!((pad & 0x860) == 0)) {
        if (!(*p != 0)) {
        PlaySoundCue(2);
        g_McMenuPage = MC_PAGE_SLOT_ACTION;
        g_McConfirmChoice = 0;
        g_McSaveMode = *p;
        break;
        }
        if (!(*p != g_McMenuRowCount - 1)) {
        if (fadeBusy) break;
        PlaySoundCue(2);
    g_McActionBusy = 0;
    StartMenuExitFade();
    break;
        }
        PlaySoundCue(5);
        g_McMenuPage = MC_PAGE_SLOT_ACTION;
        g_McSaveMode = *p;
        break;
        }
        if ((pad & 0x90) == 0 || fadeBusy) break;
        PlaySoundCue(3);
    }

    g_McActionBusy = 0;
    StartMenuExitFade();
    break;

    case MC_PAGE_SLOT_ACTION:
    switch (g_McActionState) {
    case MC_FORMAT_IDLE:
        if (!(g_McSaveMode == 0)) {
        g_McMenuPhase = MC_PROMPT_NO_DATA;
    L1447:
        { u16 p = PollMenuConfirmInput(); if (!(p)) {
    L_b1452:
        { u16 q = PollMenuBackInput(); if (q == 0) break; }
        } }
        g_McMenuPage = MC_PAGE_MODE_SELECT;
        g_McActionState = MC_FORMAT_IDLE;
        break;
        }
        g_McMenuPhase = MC_PROMPT_NEW_CARD;
        { u16 p = PollMenuConfirmInput(); if (p == 0) goto L_b1452; }
        g_McActionState = MC_FORMAT_CONFIRM;
        break;
    case MC_FORMAT_CONFIRM:
        g_McMenuPhase = g_McConfirmChoice + 7;
        SetMenuBinaryChoiceVertical(&g_McConfirmChoice);
        if (g_McConfirmChoice == 0) goto L1447;
        { u16 p = PollMenuConfirmInput();
        if (p != 0) {
        g_McActionState = MC_FORMAT_PREPARE;
        break;
        } }
        if (g_McConfirmChoice == 0) goto L1447;
        goto L_b1452;
    case MC_FORMAT_PREPARE:
        g_McActionBusy = 1;
        g_McActionTimer = 0x14;
        g_McActionState = MC_FORMAT_DELAY;
        break;
    case MC_FORMAT_DELAY:
        g_McActionTimer -= 1;
        if (g_McActionTimer == 0) {
            g_McActionState = MC_FORMAT_EXECUTE;
        }
        break;
    case MC_FORMAT_EXECUTE:
        g_McActionResult = FormatMemoryCard(0, 0);
        if (g_McActionResult == 1) {
            g_McActionState = MC_FORMAT_SUCCESS_DELAY;
            g_McActionTimer = 0x3C;
        } else {
            g_McActionState = MC_FORMAT_ERROR;
        }
        break;
    case MC_FORMAT_SUCCESS_DELAY:
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        g_McActionTimer -= 1;
        if (g_McActionTimer == 0) {
            g_McActionBusy = 0;
            g_McActionState = MC_FORMAT_SUCCESS;
        }
        break;
    case MC_FORMAT_SUCCESS:
        {
        u16 lpad = g_GameInput.pressed;
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        if ((lpad & 0x90) == 0) break;
        }
        g_McActionBusy = 0;
        g_McActionState = MC_FORMAT_IDLE;
        g_McActionResult = 0;
        g_McConfirmChoice = 0;
        g_McActionTimer = 0;
        if (fadeBusy) break;
        PlaySoundCue(3);
        StartMenuExitFade();
        break;
    case MC_FORMAT_ERROR:
        g_McMenuSubState = 0x12;
        g_McMenuPhase = MC_PROMPT_CARD_ERROR;
        g_McActionBusy = 0;
        { u16 p = PollMenuConfirmInput(); if (!(p)) {
        { u16 q = PollMenuBackInput(); if (q == 0) break; }
        } }
        g_McActionState = MC_FORMAT_IDLE;
    default:
    }
    break;

    default:
    break;
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
