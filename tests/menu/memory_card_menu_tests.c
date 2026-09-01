/*
 * The memory card menu, swept.
 *
 * This was one 987-line function and nothing tested it: the whole suite's
 * coverage of it was a single line of smoke output. It is a state machine over
 * about thirty globals, so instead of picking a handful of cases by hand, this
 * walks the state space: every card state crossed with every action state,
 * page, prompt and button, and it records what each step did.
 *
 * The record is the point. Running the same sweep against the code before a
 * change and after it says whether the change moved anything, which is the
 * only claim worth making about a menu nobody can reach from a race.
 */

#include "common.h"
#include "game/memcard.h"
#include "game/menu.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

void UpdateMemoryCardMenu(void);

/* The menu's own state. */
s32 g_McActionBusy;
s32 g_McActionElapsed;
s32 g_McActionResult;
s32 g_McActionState;
s32 g_McActionTimer;
s32 g_McCardOkFrames;
s32 g_McCardStatus;
s32 g_McConfirmChoice;
s32 g_McErrorCountdown;
s32 g_McErrorPending;
s32 g_McErrorTicks;
s32 g_McFadeLevel;
s32 g_McFadeStep;
s32 g_McFreeBlocks;
s32 g_McFromLoadMenu;
s32 g_McLastMenuState;
s32 g_McLastSlot;
s32 g_McMenuPage;
s32 g_McMenuPhase;
s32 g_McMenuRowCount;
s32 g_McMenuRowCursor;
s32 g_McMenuSelection;
s32 g_McMenuState;
s32 g_McNoCardTicks;
GameSaveHeaderRow g_McSaveHeaders[4];
s32 g_McSaveMode;
s32 g_McSettleTicks;
s32 g_McSlotCursor;
s32 g_McSlotUsedMask;
u16 g_PadPressed;
s32 g_SceneId;
s32 g_SceneTimer;
s32 GameMenuLoadPhase;

/*
 * Everything the menu reaches outside itself. The card driver is a stub that
 * answers whatever the sweep tells it to, and every call is counted, because
 * which calls a step makes is as much of its behaviour as which globals it
 * writes.
 */
static s32 s_cardStatusAnswer;
static s32 s_formatAnswer;
static s32 s_loadAnswer;
static s32 s_writeAnswer;
static int s_calls;

s32 PollMemoryCardStatus(s32 a, s32 b) {
    (void)a;
    (void)b;
    s_calls++;
    return s_cardStatusAnswer;
}
s32 FormatMemoryCard(s32 port, s32 slot) {
    (void)port;
    (void)slot;
    s_calls++;
    return s_formatAnswer;
}
s32 LoadMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *header) {
    (void)slot;
    (void)header;
    s_calls++;
    return s_loadAnswer;
}
s32 WriteMemoryCardSaveSlot(s32 slot, GameSaveHeaderRow *header) {
    (void)slot;
    (void)header;
    s_calls++;
    return s_writeAnswer;
}
s32 RefreshMemoryCardSaveStatus(s32 unused, GameSaveHeaderRow *headers) {
    (void)unused;
    (void)headers;
    s_calls++;
    return 0;
}
void ClearSaveHeaderRows(GameSaveHeaderRow *rows) {
    (void)rows;
    s_calls++;
}
void PlaySoundCue(s32 cue) { s_calls += 100 + cue; }
void StartMenuExitFade(void) { s_calls += 1000; }
void SetDispMask(s32 mask) { (void)mask; s_calls++; }
void DrawMenuFadeOverlay(s32 brightness) { (void)brightness; s_calls++; }
void DrawMemoryCardScreen(s32 page, s32 fromLoad, s32 row, s32 slot) {
    (void)page; (void)fromLoad; (void)row; (void)slot;
    s_calls++;
}
void DrawMemoryCardMessage(s32 message) { (void)message; s_calls++; }
void DrawMemoryCardSaveRows(s32 mask, GameSaveHeaderRow *headers) {
    (void)mask;
    (void)headers;
    s_calls++;
}
void AdjustMenuSelectionHorizontal(s32 *value, s32 low, s32 high) {
    /* The real one moves on left/right; this one moves the same way so the
     * cursor walks, without pulling the whole pad layer in. */
    if (g_PadPressed & PAD_LEFT) {
        (*value)--;
    }
    if (g_PadPressed & PAD_RIGHT) {
        (*value)++;
    }
    if (*value < low) {
        *value = high;
    }
    if (*value > high) {
        *value = low;
    }
}
void SetMenuBinaryChoiceVertical(s32 *choice) {
    if (g_PadPressed & (PAD_UP | PAD_DOWN)) {
        *choice = *choice == 0 ? 1 : 0;
    }
}
u16 PollMenuConfirmInput(void) {
    u16 value = g_PadPressed & 0x860;
    if (value != 0) {
        s_calls += 10;
    }
    return value;
}
u16 PollMenuBackInput(void) {
    u16 value = g_PadPressed & 0x90;
    if (value != 0) {
        s_calls += 20;
    }
    return value;
}

/*
 * Everything the step could have changed, rolled into one number so the whole
 * sweep can be compared against a single value, and written out in full when
 * the caller wants to see what moved.
 */
static unsigned long s_digest = 2166136261UL;

static void Record(FILE *out, const char *label) {
    char line[512];

    snprintf(line, sizeof(line),
            "%s state=%d action=%d phase=%d page=%d row=%d slot=%d "
            "sel=%d busy=%d timer=%d elapsed=%d result=%d choice=%d "
            "err=%d/%d/%d fade=%d/%d last=%d/%d mask=%x free=%d ticks=%d/%d/%d "
            "loadphase=%d scene=%d/%d calls=%d\n",
            label, g_McMenuState, g_McActionState,
            g_McMenuPhase, g_McMenuPage, g_McMenuRowCursor, g_McSlotCursor,
            g_McMenuSelection, g_McActionBusy, g_McActionTimer,
            g_McActionElapsed, g_McActionResult,
            g_McConfirmChoice, g_McErrorPending,
            g_McErrorCountdown, g_McErrorTicks, g_McFadeLevel, g_McFadeStep,
            g_McLastMenuState, g_McLastSlot, g_McSlotUsedMask, g_McFreeBlocks,
            g_McNoCardTicks, g_McCardOkFrames, g_McSettleTicks,
            GameMenuLoadPhase, g_SceneId, g_SceneTimer, s_calls);
    {
        const char *p;

        for (p = line; *p != '\0'; p++) {
            s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
            s_digest &= 0xFFFFFFFFUL;
        }
    }
    if (out != NULL) {
        fputs(line, out);
    }
}

static int TestFailedLoadReportsError(void) {
    g_McMenuState = 1;
    g_McMenuSelection = 1;
    g_McCardStatus = 1;
    g_McMenuPage = 1;
    g_McMenuRowCount = 4;
    g_McActionState = 0x22;
    g_McActionBusy = 1;
    g_McSlotCursor = 1;
    g_McFadeLevel = 0;
    g_McFadeStep = 0;
    g_McErrorPending = 0;
    g_SceneTimer = 0x40;
    g_PadPressed = 0;
    s_loadAnswer = 0;

    UpdateMemoryCardMenu();
    if (g_McActionResult != 0) {
        printf("FAIL a failed load returned %d\n", g_McActionResult);
        return 0;
    }

    g_McActionState = 0x26;
    UpdateMemoryCardMenu();
    if (g_McMenuPhase != MC_PROMPT_CARD_ERROR) {
        printf("FAIL a failed load reports prompt %d instead of card error\n",
               g_McMenuPhase);
        return 0;
    }
    return 1;
}

static int TestOverwritePromptResetsChoice(void) {
    g_McMenuState = 1;
    g_McMenuSelection = 1;
    g_McCardStatus = 1;
    g_McMenuPage = 1;
    g_McMenuRowCount = 4;
    g_McActionState = 0;
    g_McActionBusy = 1;
    g_McConfirmChoice = 1;
    g_McSlotCursor = 1;
    g_McSlotUsedMask = 1 << 1;
    g_McFreeBlocks = 1;
    g_McSaveMode = 0;
    g_McFadeLevel = 0;
    g_McFadeStep = 0;
    g_McErrorPending = 0;
    g_SceneTimer = 0x40;
    g_PadPressed = PAD_CONFIRM;

    UpdateMemoryCardMenu();
    if (g_McActionState != 0xA || g_McConfirmChoice != 0) {
        printf("FAIL overwrite prompt starts in action %x with choice %d\n",
               g_McActionState, g_McConfirmChoice);
        return 0;
    }
    return 1;
}

int main(int argc, char **argv) {
    static const s32 states[] = {3, 1, 2, -1, -2, -3, 7};
    static const s32 actions[] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC,
                                  0xD, 0xF, 0x10, 0x11, 0x12, 0x13, 0x14,
                                  0x15, 0x19, 0x1E, 0x1F, 0x20, 0x21, 0x22,
                                  0x23, 0x24, 0x25, 0x26, 0x27, 0x28};
    static const u16 pads[] = {0, 0x800, 0x10, 0x20, 0x40, 0x80, 0x1000,
                               0x2000};
    static const s32 statuses[] = {0, 1, 2, -1, -2, -3};
    /*
     * Every observable field and external call made by each step is folded
     * into this. When it moves, run the test with a file name to write the
     * sweep out and diff the two to see which steps changed. Dead internal
     * bookkeeping is deliberately not part of the contract.
     */
    static const unsigned long expected = 3437069393UL;
    FILE *out = NULL;
    size_t si, ai, pi, ci;
    s32 page, mode, freeBlocks;
    s32 steps = 0;
    char label[64];

    if (!TestFailedLoadReportsError() || !TestOverwritePromptResetsChoice()) {
        return 1;
    }

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            printf("cannot write %s\n", argv[1]);
            return 1;
        }
    }

    for (si = 0; si < sizeof(states) / sizeof(states[0]); si++) {
        for (ai = 0; ai < sizeof(actions) / sizeof(actions[0]); ai++) {
            for (pi = 0; pi < sizeof(pads) / sizeof(pads[0]); pi++) {
                for (ci = 0; ci < sizeof(statuses) / sizeof(statuses[0]); ci++) {
                    for (page = 0; page < 3; page++) {
                        for (mode = 0; mode < 2; mode++) {
                            for (freeBlocks = 0; freeBlocks < 2; freeBlocks++) {
                                s32 mask;

                                for (mask = 0; mask < 8; mask += 3) {
                                    /* The no-card counter has a threshold of
                                     * its own, so the sweep has to arrive at
                                     * it from either side. */
                                    static const s32 noCardTicks[] = {0, 5, 6};
                                    size_t ti;

                                    for (ti = 0; ti < 3; ti++) {
                                    /* A step starts from a clean slate so one
                                     * step's damage cannot hide the next
                                     * one's. */
                                    g_McActionBusy = 0;
                                    g_McActionElapsed = 0;
                                    g_McActionResult = 0;
                                    g_McActionTimer = 3;
                                    g_McCardOkFrames = 0;
                                    g_McCardStatus = statuses[ci];
                                    g_McConfirmChoice = 0;
                                    g_McErrorCountdown = 2;
                                    g_McErrorPending = 0;
                                    g_McErrorTicks = 0;
                                    g_McFadeLevel = 0;
                                    g_McFadeStep = 0;
                                    g_McFromLoadMenu = 0;
                                    g_McLastMenuState = 0;
                                    g_McLastSlot = 0;
                                    g_McMenuRowCount = 4;
                                    g_McMenuRowCursor = 1;
                                    g_McMenuSelection = 0;
                                    g_McNoCardTicks = noCardTicks[ti];
                                    g_McSettleTicks = 0;
                                    g_McSlotCursor = 1;
                                    g_SceneId = 26;
                                    g_SceneTimer = 0x40;
                                    GameMenuLoadPhase = 0;
                                    memset(g_McSaveHeaders, 0,
                                           sizeof(g_McSaveHeaders));

                                    g_McMenuState = states[si];
                                    g_McActionState = actions[ai];
                                    g_McMenuPage = page;
                                    g_McSaveMode = mode;
                                    g_McFreeBlocks = freeBlocks;
                                    g_McSlotUsedMask = mask;
                                    g_PadPressed = pads[pi];
                                    s_cardStatusAnswer = statuses[ci];
                                    s_formatAnswer = ci & 1;
                                    s_loadAnswer = ci & 1;
                                    s_writeAnswer = ci & 1;
                                    s_calls = 0;

                                    UpdateMemoryCardMenu();

                                    sprintf(label,
                                            "s%d/a%02x/p%04x/c%d/g%d/m%d/f%d/"
                                            "k%d/t%d",
                                            states[si], actions[ai], pads[pi],
                                            statuses[ci], page, mode,
                                            freeBlocks, mask, noCardTicks[ti]);
                                    Record(out, label);
                                    steps++;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (out != NULL) {
        fclose(out);
    }

    if (s_digest != expected) {
        printf("FAIL the memory card menu behaves differently: "
               "%d steps digest to %lu, expected %lu\n",
               steps, s_digest, expected);
        return 1;
    }
    printf("the memory card menu takes the same %d steps it always did\n",
           steps);
    return 0;
}
