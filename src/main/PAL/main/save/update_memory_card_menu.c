#include "game/memcard.h"
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/audio.h"

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
            g_SceneId = 2;
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

/* The screen itself, drawn whatever the state machine decided this frame. */
static void DrawMemoryCardMenu(void) {
    if (g_McDrawEnabled == 0) {
        return;
    }
    DrawMemoryCardScreen(g_McMenuPage, g_McFromLoadMenu, g_McMenuRowCursor,
                         g_McSlotCursor);
    if (g_McMenuPhase != 0) {
        DrawMemoryCardMessage(g_McMenuPhase - 1);
    }
    DrawMemoryCardSaveRows(g_McSlotUsedMask, g_McSaveHeaders);
}

/*
 * The card is mid-operation. Nothing to choose here; the cancel button
 * is the only way out, and only once the fade has finished.
 */
static void RunCardBusyState(s32 fadeBusy) {
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
}

/*
 * A card the game can use. This is the save and load menu itself, and
 * every prompt that hangs off picking a slot.
 */
/*
 * The list of things the player can do with a readable card. The last row
 * is the way out.
 */
static void RunCardMenuRows(s32 fadeBusy) {
    u16 pad;
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
            return;
        }
        if (fadeBusy) return;
        PlaySoundCue(2);
        } else {
        if ((pad & 0x90) == 0 || fadeBusy) return;
        PlaySoundCue(3);
        }
        g_McActionBusy = 0;
        StartMenuExitFade();
    }
}

/*
 * Picking a slot, and every step that follows from it: the prompt, the
 * confirmation, the read or write itself, and how it reports what happened.
 */
static void RunCardSlotActions(s32 fadeBusy) {
    s32 tmp;
    switch (g_McActionState) {
    case 0x00: {
        s32 *s0 = &g_McSlotCursor;
        /*
         * Picking a slot. Which prompt the player sees, and what confirming
         * does, depends on whether this is a save or a load, whether the card
         * has room, and whether the slot under the cursor already holds a
         * file. Retail asks the back button twice on the card-full path, once
         * inside the branch and once on the way out, and each ask plays its
         * own cue, so both stay.
         */
        AdjustMenuSelectionHorizontal(s0, 0, 2);
        if (g_McSaveMode != 0) {
            if ((g_McSlotUsedMask % 8) != 0) {
                g_McMenuPhase = MC_PROMPT_SELECT_LOAD;
                if (g_PadPressed & PAD_CONFIRM) {
                    if (((g_McSlotUsedMask >> *s0) & 1) != 0) {
                        PlaySoundCue(2);
                        g_McConfirmChoice = 0;
                        g_McActionState = 0x1E;
                    } else {
                        PlaySoundCue(5);
                        g_McActionState = 0x28;
                    }
                }
            } else {
                g_McMenuPhase = MC_PROMPT_NO_DATA;
                if (g_PadPressed & PAD_CONFIRM) {
                    PlaySoundCue(5);
                    g_McMenuPage = 0;
                }
            }
        } else if (g_McFreeBlocks != 0) {
            g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
            if (g_PadPressed & PAD_CONFIRM) {
                if (((g_McSlotUsedMask >> *s0) & 1) != 0) {
                    PlaySoundCue(2);
                    g_McConfirmChoice_v = 0;
                    g_McActionState = 0xA;
                } else {
                    PlaySoundCue(2);
                    g_McActionTimer = 0x1E;
                    g_McActionState = 0xB;
                }
            }
        } else if ((g_McSlotUsedMask % 8) != 0) {
            g_McMenuPhase = MC_PROMPT_SELECT_SAVE;
            if (g_PadPressed & PAD_CONFIRM) {
                if (((g_McSlotUsedMask >> *s0) & 1) != 0) {
                    PlaySoundCue(2);
                    g_McConfirmChoice = 0;
                    g_McActionState = 0xA;
                } else {
                    PlaySoundCue(2);
                    g_McActionState = 0x19;
                }
            }
        } else {
            g_McMenuPhase = MC_PROMPT_CARD_FULL;
            if (g_PadPressed & PAD_CONFIRM) {
                PlaySoundCue(5);
                g_McMenuPage = 0;
            } else if ((PollMenuBackInput() & 0xFFFF) != 0) {
                g_McMenuPage = 0;
            }
        }
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
        if (g_McConfirmChoice != 0) {
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
        if (x != 0) {
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
            if (r != 0) {
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
        cm1--;
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
        s32 stateValue;

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
        if (g_McActionResult != 0) {
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
        cm1--;
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
            cm1--;
            g_McMenuRowCursor = cm1;
        }
        break;

    default:
        break;
    }
}

static void RunCardReadyState(s32 fadeBusy) {
    /* Page 0 is the list of things to do with the card, page 1 is picking a
     * slot; any other page is not one this screen has, so it goes back. */
    if (g_McMenuPage == 0) {
        RunCardMenuRows(fadeBusy);
    } else if (g_McMenuPage == 1) {
        RunCardSlotActions(fadeBusy);
    } else {
            {
                s32 cm1 = g_McMenuRowCount;
                g_McMenuPage = 0;
                g_McSlotCursor = 0;
                g_McActionState = 0;
                g_McActionBusy = 0;
                g_McActionResult = 0;
                g_McConfirmChoice = 0;
                g_McActionTimer = 0;
                cm1--;
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
    if (g_McMenuState != 1) {
    g_McActionState = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
    g_McActionBusy = 0;
    }
}

/*
 * A format or a save running, stepping through its own stages while the
 * screen says it is busy.
 */
static void RunCardWorkingState(s32 fadeBusy) {
    s32 wtmp;

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
            if (t >= 0x79) {
        g_McCardOkFrames = 0;
        g_McActionElapsed = 0;
        if (fadeBusy == 0) {
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
            if (x != 0) {
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
        break;
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

    if (g_McMenuState == 2) return;
    g_McMenuSubState = 1;
    g_McMenuPhase = MC_PROMPT_ACCESSING;
    g_McActionState = 0;
    g_McActionResult = 0;
    g_McConfirmChoice = 0;
}

/*
 * Nothing in the slot.
 */
static void RunNoCardState(s32 fadeBusy) {
    s32 mslot;
    s32 mst;

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
    if (g_McActionTimer == 0) {
    g_McActionState = 3;
    }
    break;

    case 3:
        if (g_McMenuPage == 0) {
            s32 *cursor = &g_McMenuRowCursor;

            AdjustMenuSelectionHorizontal(cursor, 0, g_McMenuRowCount - 1);
            if (PollMenuConfirmInput() != 0) {
                if (*cursor != g_McMenuRowCount - 1) {
                    PlaySoundCue(5);
                    break;
                }
                if (fadeBusy != 0) {
                    break;
                }
                g_McActionState = 0;
                PlaySoundCue(2);
                StartMenuExitFade();
                break;
            }
            if ((g_PadPressed & PAD_CANCEL) != 0 && fadeBusy == 0) {
                g_McActionState = 0;
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
        break;
    }

    if (g_McMenuState != -1) {
    g_McActionState = 0;
    }
}

/*
 * A card the game cannot read: offer to format it, then report how that
 * went.
 */
static void RunUnformattedCardState(s32 fadeBusy) {
    u16 pad;

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
        if (g_McSaveMode != 0) {
            /* Nothing to load: either button closes the prompt. */
            g_McMenuPhase = MC_PROMPT_NO_DATA;
            if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
                g_McMenuPage = 0;
                g_McActionState = 0;
            }
            break;
        }
        /* An unformatted card: confirming starts the format, backing out
         * closes the prompt. */
        g_McMenuPhase = MC_PROMPT_NEW_CARD;
        if (PollMenuConfirmInput() != 0) {
            g_McActionState = 1;
        } else if (PollMenuBackInput() != 0) {
            g_McMenuPage = 0;
            g_McActionState = 0;
        }
        break;
    case 1:
        g_McMenuPhase = g_McConfirmChoice + 7;
        SetMenuBinaryChoiceVertical(&g_McConfirmChoice);
        if (g_McConfirmChoice == 0) {
            /* Resting on "no": either button closes the prompt. Retail asked
             * whether the choice was still zero a second time further down,
             * which it always was, because nothing between the two changes
             * it. */
            if (PollMenuConfirmInput() != 0 || PollMenuBackInput() != 0) {
                g_McMenuPage = 0;
                g_McActionState = 0;
            }
            break;
        }
        if (PollMenuConfirmInput() != 0) {
            g_McActionState = 2;
        } else if (PollMenuBackInput() != 0) {
            g_McMenuPage = 0;
            g_McActionState = 0;
        }
        break;
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
        break;
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

        if (g_McMenuState != -2) {
            g_McActionState = 0;
            g_McActionBusy = 0;
            g_McActionResult = 0;
            g_McConfirmChoice = 0;
        }
        return;

    /*
     * Retail parked these two arms inside the arm above, after its break, so
     * the only way in was the switch jumping over the `if` they sat in. They
     * are arms of this switch and now sit where the others do.
     */
}

/*
 * The card answered with something the menu has no name for.
 */
static void RunCardErrorState(s32 fadeBusy) {
    s32 tmp;

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

            if (sel == -3) {
                return;
            }
            if (sel == 3) {
                g_McLastMenuState = g_McMenuState;
            }
            tmp = g_McStateChangeCount;
            g_McMenuState = sel;
            g_McStateChangeCount = tmp + 1;
            if (g_McErrorPending != 0) {
                g_McErrorPending = 0;
                g_McErrorCountdown = 3;
            }
        }
}

void UpdateMemoryCardMenu(void) {
    s32 fadeBusy;

    fadeBusy = UpdateMemoryCardFade();
    if (!AdvanceMemoryCardMenuStartup()) {
        DrawMemoryCardMenu();
        return;
    }
    /* An action already under way owns the card, so its status is not asked
     * again until it reports an error. */
    if (g_McActionBusy == 0 || g_McErrorPending != 0) {
        s32 status = PollMemoryCardStatus(0, 0);

        g_McCardStatus = status;
        if (status == 0) {
            /* No card. Six frames of that in a row before the screen says so,
             * so a card being reseated does not flash the message. */
            s32 ticks = g_McNoCardTicks;

            g_McNoCardTicks = ticks + 1;
            if (ticks >= 6) {
                g_McMenuSelection = 3;
            }
        } else {
            s32 substate;

            switch (status) {
            case 1:
                substate = 2;
                break;
            case 2:
                substate = 1;
                break;
            case -1:
                substate = 0xA;
                break;
            case -2:
                substate = 0xB;
                break;
            case -3:
                substate = 0x11;
                break;
            default:
                substate = 0x11;
                break;
            }
            g_McNoCardTicks = 0;
            g_McMenuSubState = substate;
            g_McMenuSelection = g_McCardStatus;
        }
    }

    /*
     * What the menu does this frame is decided by what the card is: each of
     * these owns one state and nothing else.
     */
    switch (g_McMenuState) {
    case 3:
        RunCardBusyState(fadeBusy);
        break;
    case 1:
        RunCardReadyState(fadeBusy);
        break;
    case 2:
        RunCardWorkingState(fadeBusy);
        break;
    case -1:
        RunNoCardState(fadeBusy);
        break;
    case -2:
        RunUnformattedCardState(fadeBusy);
        break;
    case -3:
    default:
        RunCardErrorState(fadeBusy);
        break;
    }
    DrawMemoryCardMenu();
}
