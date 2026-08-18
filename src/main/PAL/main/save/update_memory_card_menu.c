#include "common.h"
#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/state.h"
#include "game/menu.h"
#include "psyq/gpu.h"
#include "game/audio.h"
#include "game/game_context.h"

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
        g_McLastMenuState = -1;
        g_McMenuPhase = MC_PROMPT_NONE;
        g_McMenuSelection = next;
        g_McMenuState = next;
        g_McActionState = 0;
        g_McActionResult = 0;
        g_McConfirmChoice = 0;
        g_McStateChangeCount = 0;
        g_McActionTimer = 0;
        g_McActionBusy = 0;
        g_McDrawEnabled = 1;
    }
    return 0;
}

void UpdateMemoryCardMenu(void) {
    s32 fadeBusy;
    s32 tmp;
    u16 pad;
    s32 wtmp;
    s32 mst;
    s32 mslot;

    fadeBusy = UpdateMemoryCardFade();
    if (!AdvanceMemoryCardMenuStartup()) goto menu_state_update_done;
    if (!(g_McActionBusy == 0)) {
    if (g_McErrorPending == 0) goto L_sw2;

    }
    {
        s32 st = PollMemoryCardStatus(0, 0);
        s32 c;
        s32 sd;
        g_McCardStatus = st;
        switch (st) {
        case 0: {
            s32 b = g_McNoCardTicks;
            g_McNoCardTicks = b + 1;
            if (b >= 6) {
                g_McMenuSelection = 3;
            }
            goto L_sw2;
        }
        case 1: sd = g_McCardStatus; c = 2; break;
        case 2: sd = g_McCardStatus; c = 1; break;
        case -1: sd = g_McCardStatus; c = 0xA; break;
        case -2: sd = g_McCardStatus; c = 0xB; break;
        case -3: sd = g_McCardStatus; c = 0x11; break;
        default: sd = g_McCardStatus; c = 0x11; break;
        }
        g_McNoCardTicks = 0;
        g_McMenuSubState = c;
        g_McMenuSelection = sd;
    }

L_sw2:
    switch (g_McMenuState) {

    case 3:
    {
    u16 lpad = g_PadPressed;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    g_McActionBusy = 0;
    if ((lpad & 0x90) && !fadeBusy) {
        PlaySoundCue(3);
        StartMenuExitFade();
    }
    }
    switch (g_McMenuSelection) {
    case 1:
    {
        s32 cardStatus = g_McCardStatus;
        if (cardStatus == 1) {
            if (g_McLastMenuState != 2) {
                g_McMenuState = 2;
            } else {
                g_McMenuState = cardStatus;
            }
        }
        break;
    }
    case 2:
        g_McMenuState = 2;
        break;
    case -1:
    case -2:
        g_McMenuState = g_McMenuSelection;
        break;
    case 3:
        break;
    case -3:
    default:
    {
        s32 cardStatus = g_McCardStatus;
        if (cardStatus == -3) {
            s32 r = g_McErrorTicks;
            g_McErrorTicks = r + 1;
            if (r >= 4) {
                g_McMenuState = cardStatus;
            }
        }
    }
    }
    if (g_McMenuState != 3) {
        g_McErrorTicks = 0;
    }
    break;

    case 1:
    switch (g_McMenuPage) {
    case 0:
    {
        s32 *p = &g_McMenuRowCursor;
        g_McMenuPhase = MC_PROMPT_NONE;
        AdjustMenuSelectionHorizontal(p, 0, g_McMenuRowCount - 1);
        pad = g_PadPressed;
        if (!((pad & 0x860) == 0)) {
        if (*p < g_McMenuRowCount - 1) {
            PlaySoundCue(2);
            g_McMenuPage = 1;
            g_McActionState = 0;
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

    case 1:
    switch (g_McActionState) {
    case 0x00: {
        s32 *s0 = &g_McSlotCursor;
        s32 a0;
        s32 nv;
        AdjustMenuSelectionHorizontal(s0, 0, 2);
        if (!(g_McSaveMode == 0)) {
        a0 = g_McSlotUsedMask;
        if (!((a0 % 8) == 0)) {
        g_McMenuPhase = MC_PROMPT_SELECT_LOAD;
        if ((g_PadPressed & PAD_CONFIRM) == 0) goto slot_prompt_done;
        if (!(((a0 >> *s0) & 1) == 0)) {
        PlaySoundCue(2);
        g_McConfirmChoice = 0;
        nv = 0x1E;
        } else {
        PlaySoundCue(5);
        nv = 0x28;
        }
        goto L_b475;
        }
        g_McMenuPhase = MC_PROMPT_NO_DATA;
        if ((g_PadPressed & PAD_CONFIRM) == 0) goto slot_prompt_done;
        } else {
        if (g_McFreeBlocks != 0) goto L_b448;
        a0 = g_McSlotUsedMask;
        if (!((a0 % 8) == 0)) {
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if ((g_PadPressed & PAD_CONFIRM) == 0) goto slot_prompt_done;
        if (!(((a0 >> *s0) & 1) == 0)) {
        PlaySoundCue(2);
        g_McConfirmChoice = 0;
        nv = 0xA;
        } else {
        PlaySoundCue(2);
        nv = 0x19;
        }
        goto L_b475;
        }
        g_McMenuPhase = MC_PROMPT_CARD_FULL;
        if ((g_PadPressed & PAD_CONFIRM) == 0) goto L_b439;
        }
        PlaySoundCue(5);
        g_McMenuPage = 0;
        goto slot_prompt_done;
    L_b439:
        if (!((PollMenuBackInput() & 0xFFFF) == 0)) {
        g_McMenuPage = 0;
        goto slot_prompt_done;
    L_b448:
        g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
        if (!((g_PadPressed & PAD_CONFIRM) == 0)) {
        if (!(((g_McSlotUsedMask >> *s0) & 1) == 0)) {
        PlaySoundCue(2);
        g_McConfirmChoice_v = 0;
        nv = 0xA;
        } else {
        PlaySoundCue(2);
        g_McActionTimer = 0x1E;
        nv = 0xB;
        }
    L_b475:
        g_McActionState = nv;
        }
        }
slot_prompt_done:
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        g_McMenuPage = 0;
        break;
    }

    case 0x0A: {
        s32 *p = &g_McConfirmChoice;
        s32 hi = g_McSlotCursor * 2;
        s32 lo = g_McConfirmChoice + 9;
        g_McMenuPhase = hi + lo;
        SetMenuBinaryChoiceVertical(p);
        if (!(g_McConfirmChoice == 0)) {
        if (!((PollMenuConfirmInput() & 0xFFFF) == 0)) {
        g_McActionState = 0xB;
        break;
        }
        if (g_McConfirmChoice != 0) {
            if ((PollMenuBackInput() & 0xFFFF) == 0) break;
            g_McActionState = 0;
            break;
        }
        }
        if ((PollMenuConfirmInput() & 0xFFFF) != 0) {
            g_McActionState = 0;
            break;
        }
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        g_McActionState = 0;
        break;
    }

    case 0x0B:
        g_McMenuPhase = MC_PROMPT_ACCESSING;
        g_McActionTimer = 0xA;
        g_McActionState = 0xC;
        break;

    case 0x0C: {
        s32 t = g_McActionTimer;
        g_McActionBusy = 1;
        g_McActionTimer = t - 1;
        if (g_McActionTimer != 0) break;
        g_McActionState = 0xD;
        break;
    }

    case 0x0D: {
        s32 a0 = g_McSlotCursor;
        s32 x;
        s32 dp;
        g_McMenuSubState = 5;
        x = WriteMemoryCardSaveSlot(a0, &g_McSaveHeaders[a0]);
        g_McActionResult = x;
        if (!(x == 0)) {
        g_McActionOk = 1;
        x = 6;
        } else {
        x = 0x10;
        g_McActionOk = 0;
        }
        g_McMenuSubState = x;
        dp = GameMenuLoadPhase;
        g_McActionState = 0xF;
        g_McSavedLoadPhase = dp;
        break;
    }

    case 0x0F:
        g_McActionState = 0x10;
        break;

    case 0x10: {
        s32 nv;
        if (g_McActionResult != 0) {
        {
            s32 r = RefreshMemoryCardSaveStatus(0, g_McSaveHeaders);
            g_McSlotUsedMask = r;
            if (!(r == 0)) {
                if ((r & 0xFFFF) == 0) {
                    nv = 0xE;
                    g_McMenuSubState = nv;
                }
            } else {
                nv = 0xC;
                g_McMenuSubState = nv;
            }
        }
        g_McSavedLoadPhase = GameMenuLoadPhase;
        }
        nv = 0x11;
        g_McActionState = nv;
        break;
    }

    case 0x11:
        g_McActionTimer = 5;
        g_McActionState = 0x12;
        break;

    case 0x12: {
        s32 t = g_McActionTimer;
        g_McActionTimer = t - 1;
        if (g_McActionTimer != 0) break;
        g_McSettleTicks = 0;
        g_McActionState = 0x13;
        break;
    }

    case 0x13: {
        s32 t;
        if (PollMemoryCardStatus(0, 0) != 1) break;
        t = g_McSettleTicks + 1;
        g_McSettleTicks = t;
        if (t < 4) break;
        g_McActionState = 0x14;
        break;
    }

    case 0x14: {
        s32 x = g_McActionOk;
        if (x != 0) {
            x = 0x12;
        } else {
            x = 0x10;
        }
        g_McMenuPhase = x;
        g_McActionTimer = 0x3C;
        g_McActionBusy = 0;
        g_McActionState = 0x15;
        break;
    }

    case 0x15: {
        s32 t = g_McActionTimer;
        s32 cm1;
        g_McActionTimer = t - 1;
        if (g_McActionTimer != 0) break;
        cm1 = g_McMenuRowCount;
        g_McMenuPage = 0;
        g_McActionState = 0;
        cm1 = cm1 - 1;
        g_McMenuRowCursor = cm1;
        break;
    }

    case 0x19:
        g_McMenuPhase = MC_PROMPT_CARD_FULL;
        if (!((PollMenuConfirmInput() & 0xFFFF) != 0)) {
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        }
        g_McMenuPage = 0;
        g_McActionState = 0;
        break;

    case 0x1E:
        g_McMenuSubState = 7;
        g_McActionTimer = 5;
        g_McActionState = 0x1F;
        break;

    case 0x1F: {
        s32 t = g_McActionTimer;
        g_McActionTimer = t - 1;
        if (g_McActionTimer != 0) break;
        g_McActionState = 0x20;
        break;
    }

    {
        register s32 stateValue asm("$3");

    case 0x20: {
        tmp = 0xF;
        stateValue = 1;
        g_McMenuPhase = tmp;
        g_McActionTimer = tmp;
        g_McActionBusy = stateValue;
        g_McActionState = 0x21;
        break;
    }

    case 0x21: {
        s32 t = g_McActionTimer;
        g_McActionTimer = t - 1;
        if (g_McActionTimer != 0) break;
        g_McActionState = 0x22;
        break;
    }

    case 0x22: {
        s32 *s0 = &g_McSlotCursor;
        s32 a0 = *s0;
        s32 dp;
        g_McActionResult = LoadMemoryCardSaveSlot(a0, &g_McSaveHeaders[a0]);
        if (!(g_McActionResult == 0)) {
        stateValue = *s0;
        g_McActionOk = 1;
        g_McMenuSubState = 8;
        g_McLastSlot = stateValue;
        } else {
        g_McActionOk = 1;
        g_McMenuSubState = 0xF;
        }
        dp = GameMenuLoadPhase;
        g_McActionTimer = 0x3C;
        g_McActionState = 0x23;
        g_McSavedLoadPhase = dp;
        break;
    }
    }

    case 0x23:
        g_McActionTimer = 5;
        g_McActionState = 0x24;
        break;

    case 0x24: {
        s32 t = g_McActionTimer;
        g_McActionTimer = t - 1;
        if (g_McActionTimer != 0) break;
        g_McSettleTicks = 0;
        g_McActionState = 0x25;
        break;
    }

    case 0x25: {
        s32 t;
        if (PollMemoryCardStatus(0, 0) != 1) break;
        t = g_McSettleTicks + 1;
        g_McSettleTicks = t;
        if (t < 4) break;
        g_McActionState = 0x26;
        break;
    }

    case 0x26: {
        s32 x = g_McActionOk;
        if (x != 0) {
            x = 0x11;
        } else {
            x = 0x10;
        }
        g_McMenuPhase = x;
        g_McActionTimer = 0x3C;
        g_McActionBusy = 0;
        g_McActionState = 0x27;
        break;
    }

    case 0x27: {
        s32 t = g_McActionTimer;
        s32 cm1;
        g_McActionTimer = t - 1;
        if (g_McActionTimer != 0) break;
        cm1 = g_McMenuRowCount;
        g_McMenuPage = 0;
        g_McActionState = 0;
        cm1 = cm1 - 1;
        g_McMenuRowCursor = cm1;
        break;
    }

    case 0x28:
        g_McMenuPhase = MC_PROMPT_NO_FILE;
        if (!((PollMenuConfirmInput() & 0xFFFF) != 0)) {
        if ((PollMenuBackInput() & 0xFFFF) == 0) break;
        }
        g_McMenuPage = 0;
        g_McActionState = 0;
        break;

        {
            s32 cm1 = g_McMenuRowCount;
            g_McMenuPage = 0;
            g_McSlotCursor = 0;
            g_McActionState = 0;
            g_McActionBusy = 0;
            g_McActionResult = 0;
            g_McConfirmChoice = 0;
            g_McActionTimer = 0;
            cm1 = cm1 - 1;
            g_McMenuRowCursor = cm1;
        }
        break;

    default:
    }
    break;

    default:
        {
            s32 cm1 = g_McMenuRowCount;
            g_McMenuPage = 0;
            g_McSlotCursor = 0;
            g_McActionState = 0;
            g_McActionBusy = 0;
            g_McActionResult = 0;
            g_McConfirmChoice = 0;
            g_McActionTimer = 0;
            cm1 = cm1 - 1;
            g_McMenuRowCursor = cm1;
        }
    }
    switch (g_McMenuSelection) {
    case 3:
        g_McLastMenuState = g_McMenuState;
        /* fallthrough */
    case -2:
    case -1:
    case 2:
        g_McMenuState = g_McMenuSelection;
        break;
    case 1:
        if (g_McErrorPending != 0) {
            g_McErrorPending = 0;
            g_McErrorCountdown = 3;
        }
        break;
    case -3:
    default:
        {
            s32 sd = g_McCardStatus;
            g_McErrorPending = 1;
            if (sd == -3) {
                s32 r = g_McErrorCountdown - 1;
                g_McErrorCountdown = r;
                if (r == 0) {
                    g_McMenuState = sd;
                }
            }
        }
    }
    if (!(g_McMenuState == 1)) {
    g_McActionState = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    g_McActionBusy = 0;
    }
    break;

    case 2:
    g_McMenuSubState = 1;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    switch (g_McActionState) {
    case 0:
        {
            u32 sceneFrame = g_SceneTimer;
            if (sceneFrame < 0x1F) break;
            wtmp = 1;
            g_McCardOkFrames = 0;
            g_McActionElapsed = 0;
            g_McActionState = wtmp;
        }
        break;
    case 1:
        g_McActionBusy = 0;
        {
            s32 t = g_McActionElapsed + 1;
            g_McActionElapsed = t;
            if (!((g_PadPressed & PAD_CANCEL) == 0)) {
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
        wtmp = 2;
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        g_McActionState = wtmp;
        break;
    case 2:
        g_McActionBusy = 1;
        g_McActionTimer = 5;
        g_McActionState = 3;
        break;
    case 3:
        {
            s32 t = g_McActionTimer;
            g_McActionTimer = t - 1;
            if (g_McActionTimer != 0) break;
        }
        g_McActionState = 5;
        break;
    case 5:
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
            g_McActionState = 6;
            g_McSavedLoadPhase = w;
        }
        break;
    case 6:
        g_McActionTimer = 5;
        g_McActionState = 7;
        break;
    case 7:
        {
            s32 t = g_McActionTimer;
            g_McActionTimer = t - 1;
            if (g_McActionTimer != 0) break;
        }
        g_McActionTimer = 5;
        g_McActionBusy = 0;
        g_McActionState = 8;
        break;
    case 8:
        {
            s32 t = g_McActionTimer;
            g_McActionTimer = t - 1;
            if (g_McActionTimer != 0) break;
        }
        g_McActionState = 9;
        break;
    case 9:
        if (g_McMenuSelection != 1) break;
        g_McMenuState = g_McMenuSelection;
        break;
    default:
    }

    switch (g_McMenuSelection) {
    case 3:
        g_McLastMenuState = g_McMenuState;
        /* fallthrough */
    case -2:
    case -1:
        g_McMenuState = g_McMenuSelection;
        break;
    case 2:
        if (g_McErrorPending != 0) {
            g_McErrorPending = 0;
            g_McErrorCountdown = 3;
        }
        break;
    case 1:
        break;
    case -3:
    case 0:
    default:
    {
        s32 cardStatus = g_McCardStatus;
        g_McErrorPending = 1;
        if (cardStatus == -3) {
            s32 t = g_McErrorCountdown;
            g_McErrorCountdown = t - 1;
            if (g_McErrorCountdown == 0) {
                g_McMenuState = cardStatus;
            }
        }
    }
    }

    if (g_McMenuState == 2) break;
    g_McMenuSubState = 1;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    g_McActionState = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    break;
    case -1:
    g_McMenuSubState = 0xA;
    g_McMenuPhase = MC_PROMPT_NO_CARD;
    g_McActionBusy = 0;
    mst = g_McActionState;
    switch (mst) {
    case 0:
    g_McActionTimer = 5;
    g_McSlotUsedMask = 0;
    ClearSaveHeaderRows(g_McSaveHeaders);
    g_McLastSlot = 0;
    g_McActionState = 1;
    break;

    case 1:
    g_McActionTimer -= 1;
    if (!(g_McActionTimer != 0)) {
    g_McActionState = 3;
    }
    break;

    case 3:
    {
        s32 mph = g_McMenuPage;
        if (!(mph == 0)) {
        if (mph == 1) goto L_b1280;
        break;
        }
    }

    {
        s32 *mp = &g_McMenuRowCursor;
        AdjustMenuSelectionHorizontal(mp, 0, g_McMenuRowCount - 1);
        if (!(PollMenuConfirmInput() == 0)) {
        if (!(*mp != g_McMenuRowCount - 1)) {
    if (fadeBusy != 0) break;
    g_McActionState = 0;
    PlaySoundCue(2);
    StartMenuExitFade();
    break;
        }

    PlaySoundCue(5);
    break;
        }
    }

    if (!((g_PadPressed & PAD_CANCEL) == 0)) {
    if (!(fadeBusy != 0)) {
    g_McActionState = 0;
    PlaySoundCue(3);
    StartMenuExitFade();
    break;

L_b1280:
    if (!((g_PadPressed & PAD_CANCEL) == 0)) {
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
    switch (g_McMenuSelection) {
    case 1:
    case 2:
        g_McMenuState = 2;
        /* fall through */
    case -1:
        if (g_McErrorPending == 0) break;
        g_McErrorPending = 0;
        g_McErrorCountdown = 3;
        break;
    case -2:
        g_McMenuState = -2;
        break;
    default:
    case -3:
    case 0:
        mslot = g_McCardStatus;
        g_McErrorPending = 1;
        if (mslot != -3) break;
        g_McErrorCountdown -= 1;
        if (g_McErrorCountdown != 0) break;
        g_McMenuState = mslot;
        /* fall through */
    case 3:
        ;
    }

    if (!(g_McMenuState == -1)) {
    g_McActionState = 0;
    }
    break;
    case -2:
    switch (g_McMenuPage) {
    case 0:
    {
        s32 *p = &g_McMenuRowCursor;
        g_McMenuSubState = 0xB;
        g_McMenuPhase = MC_PROMPT_NONE;
        AdjustMenuSelectionHorizontal(p, 0, g_McMenuRowCount - 1);
        pad = g_PadPressed;
        if (!((pad & 0x860) == 0)) {
        if (!(*p != 0)) {
        PlaySoundCue(2);
        g_McMenuPage = 1;
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
        g_McMenuPage = 1;
        g_McSaveMode = *p;
        break;
        }
        if ((pad & 0x90) == 0 || fadeBusy) break;
        PlaySoundCue(3);
    }

    g_McActionBusy = 0;
    StartMenuExitFade();
    break;

    case 1:
    switch (g_McActionState) {
    case 0:
        if (!(g_McSaveMode == 0)) {
        g_McMenuPhase = MC_PROMPT_NO_DATA;
    L1447:
        { u16 p = PollMenuConfirmInput(); if (!(p)) {
    L_b1452:
        { u16 q = PollMenuBackInput(); if (q == 0) break; }
        } }
        g_McMenuPage = 0;
        g_McActionState = 0;
        break;
        }
        g_McMenuPhase = MC_PROMPT_NEW_CARD;
        { u16 p = PollMenuConfirmInput(); if (p == 0) goto L_b1452; }
        g_McActionState = 1;
        break;
    case 1:
        g_McMenuPhase = g_McConfirmChoice + 7;
        SetMenuBinaryChoiceVertical(&g_McConfirmChoice);
        if (g_McConfirmChoice == 0) goto L1447;
        { u16 p = PollMenuConfirmInput();
        if (p != 0) {
        g_McActionState = 2;
        break;
        } }
        if (g_McConfirmChoice == 0) goto L1447;
        goto L_b1452;
    case 2:
        g_McActionBusy = 1;
        g_McActionTimer = 0x14;
        g_McActionState = 3;
        break;
    case 3:
        g_McActionTimer -= 1;
        if (g_McActionTimer == 0) {
            g_McActionState = 5;
        }
        break;
    case 5:
        g_McActionResult = FormatMemoryCard(0, 0);
        if (g_McActionResult == 1) {
            g_McActionState = 7;
            g_McActionTimer = 0x3C;
        } else {
            g_McActionState = 0xA;
        }
        break;
    case 7:
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        g_McActionTimer -= 1;
        if (g_McActionTimer == 0) {
            g_McActionBusy = 0;
            g_McActionState = 8;
        }
        break;
    case 8:
        {
        u16 lpad = g_PadPressed;
        g_McMenuPhase = MC_PROMPT_FORMAT_OK;
        if ((lpad & 0x90) == 0) break;
        }
        g_McActionBusy = 0;
        g_McActionState = 0;
        g_McActionResult = 0;
        g_McConfirmChoice = 0;
        g_McActionTimer = 0;
        if (fadeBusy) break;
        PlaySoundCue(3);
        StartMenuExitFade();
        break;
    case 0xA:
        g_McMenuSubState = 0x12;
        g_McMenuPhase = MC_PROMPT_CARD_ERROR;
        g_McActionBusy = 0;
        { u16 p = PollMenuConfirmInput(); if (!(p)) {
        { u16 q = PollMenuBackInput(); if (q == 0) break; }
        } }
        g_McActionState = 0;
    default:
    }
    break;

    default:
    break;
    }
    switch (g_McMenuSelection) {
    case 1:
    case 2:
        g_McMenuState = 2;
        break;
    case 3:
        g_McLastMenuState = g_McMenuState;
        g_McMenuState = 3;
        break;
    case -1:
        g_McMenuState = -1;
        break;
    case -2:
        if (g_McErrorPending != 0) {
            g_McErrorPending = 0;
            g_McErrorCountdown = 3;
        }
        break;
    case -3:
    default:
        {
            s32 sd = g_McCardStatus;
            g_McErrorPending = 1;
            if (sd == -3) {
                s32 r = g_McErrorCountdown - 1;
                g_McErrorCountdown = r;
                if (r == 0) {
                    g_McMenuState = sd;
                }
            }
        }
    }

    if (!(g_McMenuState == -2)) {
    g_McActionState = 0;
    g_McActionBusy = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    break;
    case -3:
    default:
    g_McMenuSubState = 0x11;
    {
    u16 lpad = g_PadPressed;
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
