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
#include "game/memcard_internal.h"
#include "game/menu.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

/* The menu's own state. */
MemoryCardSession s_memoryCard;

MemoryCardSession *SceneRuntimeMemoryCard(void) { return &s_memoryCard; }
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
static int s_useRealCardDriver;
s32 FixturePollMemoryCardStatus(MemoryCardPoll *poll, s32 port, s32 slot);
void FixtureResetMemoryCardStatus(void);
static s32 s_formatAnswer;
static s32 s_loadAnswer;
static s32 s_writeAnswer;
static int s_calls;

s32 PollMemoryCardStatus(MemoryCardPoll *poll, s32 a, s32 b) {
    if (s_useRealCardDriver) return FixturePollMemoryCardStatus(poll, a, b);
    (void)poll;
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
s32 RefreshMemoryCardSaveStatus(GameSaveHeaderRow *headers, s32 *freeBlocks) {
    (void)headers;
    (void)freeBlocks;
    s_calls++;
    return 0;
}
void ClearSaveHeaderRows(GameSaveHeaderRow *rows) {
    (void)rows;
    s_calls++;
}
void PlaySoundCue(s32 cue) { s_calls += 100 + cue; }
void StartMenuExitFade(MemoryCardSession *memoryCard) {
    (void)memoryCard;
    s_calls += 1000;
}
void SetDispMask(s32 mask) { (void)mask; s_calls++; }
void DrawFullscreenFadeTile480(s32 brightness, s32 tpage) {
    (void)brightness;
    (void)tpage;
    s_calls++;
}
void DrawMemoryCardScreen(s32 page, s32 fromLoad, s32 row, s32 slot) {
    (void)page; (void)fromLoad; (void)row; (void)slot;
    s_calls++;
}
void DrawMemoryCardMessage(s32 message) { (void)message; s_calls++; }
void DrawMemoryCardSaveRows(s32 mask, const GameSaveHeaderRow *headers,
                            s32 freeBlocks, s32 menuPage, s32 menuRow) {
    (void)mask;
    (void)headers;
    (void)freeBlocks;
    (void)menuPage;
    (void)menuRow;
    s_calls++;
}
void AdjustMenuSelectionVertical(s32 *value, s32 low, s32 high) {
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
void SetMenuBinaryChoiceHorizontal(s32 *choice) {
    if (g_PadPressed & (PAD_UP | PAD_DOWN)) {
        *choice = *choice == 0 ? 1 : 0;
    }
}
u16 PollMenuConfirmInput(void) {
    u16 value = g_PadPressed & PAD_CONFIRM;
    if (value != 0) {
        s_calls += 10;
    }
    return value;
}
u16 PollMenuBackInput(void) {
    u16 value = g_PadPressed & PAD_CANCEL;
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
            "sel=%d busy=%d timer=%d result=%d choice=%d "
            "err=%d/%d/%d fade=%d/%d last=%d/%d mask=%x free=%d ticks=%d/%d "
            "loadphase=%d scene=%d/%d calls=%d\n",
            label, s_memoryCard.menuState, s_memoryCard.action.state,
            s_memoryCard.phase, s_memoryCard.menuPage, s_memoryCard.menuRow, s_memoryCard.slot,
            s_memoryCard.menuSelection, s_memoryCard.action.busy, s_memoryCard.action.timer,
            s_memoryCard.action.result,
            s_memoryCard.action.confirmChoice, s_memoryCard.errorPending,
            s_memoryCard.errorCountdown, s_memoryCard.errorTicks,
            s_memoryCard.fadeLevel, s_memoryCard.fadeStep,
            s_memoryCard.lastMenuState, s_memoryCard.slots.lastSlot,
            s_memoryCard.slots.usedMask, s_memoryCard.freeBlocks,
            s_memoryCard.noCardTicks, s_memoryCard.settleTicks,
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
    s_memoryCard.menuState = 1;
    s_memoryCard.menuSelection = 1;
    s_memoryCard.cardStatus = 1;
    s_memoryCard.menuPage = 1;
    s_memoryCard.fromLoadMenu = 1;
    s_memoryCard.action.state = 0x21;
    s_memoryCard.action.timer = 0;
    s_memoryCard.action.busy = 1;
    s_memoryCard.slot = 1;
    s_memoryCard.fadeLevel = 0;
    s_memoryCard.fadeStep = 0;
    s_memoryCard.errorPending = 0;
    g_SceneTimer = 0x40;
    g_PadPressed = 0;
    s_loadAnswer = 0;

    UpdateMemoryCardMenu();
    if (s_memoryCard.action.result != 0) {
        printf("FAIL a failed load returned %d\n", s_memoryCard.action.result);
        return 0;
    }

    s_memoryCard.action.state = 0x25;
    s_memoryCard.settleTicks = 3;
    s_cardStatusAnswer = MC_MENU_STATE_READY;
    RunCardSlotActions(&s_memoryCard);
    if (s_memoryCard.phase != MC_PROMPT_CARD_ERROR) {
        printf("FAIL a failed load reports prompt %d instead of card error\n",
               s_memoryCard.phase);
        return 0;
    }
    return 1;
}

static int TestOverwritePromptResetsChoice(void) {
    s_memoryCard.menuState = 1;
    s_memoryCard.menuSelection = 1;
    s_memoryCard.cardStatus = 1;
    s_memoryCard.menuPage = 1;
    s_memoryCard.fromLoadMenu = 1;
    s_memoryCard.action.state = 0;
    s_memoryCard.action.busy = 1;
    s_memoryCard.action.confirmChoice = 1;
    s_memoryCard.slot = 1;
    s_memoryCard.slots.usedMask = 1 << 1;
    s_memoryCard.freeBlocks = 1;
    s_memoryCard.saveMode = 0;
    s_memoryCard.fadeLevel = 0;
    s_memoryCard.fadeStep = 0;
    s_memoryCard.errorPending = 0;
    g_SceneTimer = 0x40;
    g_PadPressed = PAD_CONFIRM;

    UpdateMemoryCardMenu();
    if (s_memoryCard.action.state != 0xA || s_memoryCard.action.confirmChoice != 0) {
        printf("FAIL overwrite prompt starts in action %x with choice %d\n",
               s_memoryCard.action.state, s_memoryCard.action.confirmChoice);
        return 0;
    }
    return 1;
}

static void PrepareFormatOperation(s32 formatAnswer) {
    s_memoryCard.menuState = MC_MENU_STATE_UNFORMATTED;
    s_memoryCard.menuSelection = MC_MENU_STATE_UNFORMATTED;
    s_memoryCard.cardStatus = MC_MENU_STATE_UNFORMATTED;
    s_memoryCard.menuPage = 1;
    s_memoryCard.fromLoadMenu = 0;
    s_memoryCard.action.state = 5;
    s_memoryCard.action.busy = 1;
    s_memoryCard.action.result = 0;
    s_memoryCard.fadeLevel = 0;
    s_memoryCard.fadeStep = 0;
    s_memoryCard.errorPending = 0;
    g_SceneTimer = 0x40;
    g_PadPressed = 0;
    s_cardStatusAnswer = MC_MENU_STATE_UNFORMATTED;
    s_formatAnswer = formatAnswer;
}

static int TestFormatOperationReportsItsResult(void) {
    PrepareFormatOperation(1);
    UpdateMemoryCardMenu();
    if (s_memoryCard.action.result != 1 || s_memoryCard.action.state != 7 ||
        s_memoryCard.action.timer != 60) {
        printf("FAIL successful format result=%d action=%x timer=%d\n",
               s_memoryCard.action.result, s_memoryCard.action.state, s_memoryCard.action.timer);
        return 0;
    }

    PrepareFormatOperation(0);
    UpdateMemoryCardMenu();
    if (s_memoryCard.action.result != 0 || s_memoryCard.action.state != 0xA) {
        printf("FAIL failed format result=%d action=%x\n",
               s_memoryCard.action.result, s_memoryCard.action.state);
        return 0;
    }
    return 1;
}

static int TestCardSettleRequiresConsecutiveReadyPolls(void) {
    int i;

    s_memoryCard.action.state = 0x25;
    s_memoryCard.settleTicks = 2;

    s_cardStatusAnswer = MC_MENU_STATE_READY;
    RunCardSlotActions(&s_memoryCard);
    if (s_memoryCard.settleTicks != 3 || s_memoryCard.action.state != 0x25) {
        printf("FAIL settle did not accept the third ready poll\n");
        return 0;
    }

    s_cardStatusAnswer = MC_MENU_STATE_NO_CARD;
    RunCardSlotActions(&s_memoryCard);
    if (s_memoryCard.settleTicks != 0 || s_memoryCard.action.state != 0x25) {
        printf("FAIL interrupted settle kept %d ready polls in action %x\n",
               s_memoryCard.settleTicks, s_memoryCard.action.state);
        return 0;
    }

    s_cardStatusAnswer = MC_MENU_STATE_READY;
    for (i = 0; i < 3; i++) {
        RunCardSlotActions(&s_memoryCard);
    }
    if (s_memoryCard.action.state != 0x25) {
        printf("FAIL settle completed after only three consecutive polls\n");
        return 0;
    }
    RunCardSlotActions(&s_memoryCard);
    if (s_memoryCard.action.state != 0x27) {
        printf("FAIL settle did not complete after four consecutive polls\n");
        return 0;
    }
    return 1;
}

static int TestRealCardDriverSettlesSaveAndLoad(void) {
    static const s32 actions[] = {0x13, 0x25};
    static const s32 completed[] = {0x15, 0x27};
    for (unsigned i = 0; i < 2; ++i) {
        FixtureResetMemoryCardStatus();
        s_useRealCardDriver = 1;
        s_memoryCard.action.state = actions[i];
        s_memoryCard.settleTicks = 0;
        for (int frame = 0; frame < 60 && s_memoryCard.action.state == actions[i]; ++frame)
            RunCardSlotActions(&s_memoryCard);
        s_useRealCardDriver = 0;
        if (s_memoryCard.action.state != completed[i]) {
            printf("FAIL real card driver stuck settling action %x\n", actions[i]);
            return 0;
        }
    }
    return 1;
}

int main(int argc, char **argv) {
    static const s32 states[] = {3, 1, 2, -1, -2, -3, 7};
    static const s32 actions[] = {0, 1, 2, 3, 5, 6, 7, 8, 9, 0xA, 0xC,
                                  0x12, 0x13, 0x15, 0x19, 0x1F, 0x21,
                                  0x24, 0x25, 0x27, 0x28};
    static const u16 pads[] = {0, 0x800, 0x10, 0x20, 0x40, 0x80, 0x1000,
                               0x2000};
    static const s32 statuses[] = {0, 1, 2, -1, -2, -3};
    /*
     * Every observable field and external call made by each step is folded
     * into this. When it moves, run the test with a file name to write the
     * sweep out and diff the two to see which steps changed. Dead internal
     * bookkeeping is deliberately not part of the contract.
     */
    static const unsigned long expected = 1163284757UL;
    FILE *out = NULL;
    size_t si, ai, pi, ci;
    s32 page, mode, freeBlocks;
    s32 steps = 0;
    char label[64];

    if (!TestFailedLoadReportsError() || !TestOverwritePromptResetsChoice() ||
        !TestFormatOperationReportsItsResult() ||
        !TestCardSettleRequiresConsecutiveReadyPolls() ||
        !TestRealCardDriverSettlesSaveAndLoad()) {
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
                                    s_memoryCard.action.busy = 0;
                                    s_memoryCard.action.result = 0;
                                    s_memoryCard.action.timer = 3;
                                    s_memoryCard.cardStatus = statuses[ci];
                                    s_memoryCard.action.confirmChoice = 0;
                                    s_memoryCard.errorCountdown = 2;
                                    s_memoryCard.errorPending = 0;
                                    s_memoryCard.errorTicks = 0;
                                    s_memoryCard.fadeLevel = 0;
                                    s_memoryCard.fadeStep = 0;
                                    s_memoryCard.fromLoadMenu = 0;
                                    s_memoryCard.lastMenuState = 0;
                                    s_memoryCard.slots.lastSlot = 0;
                                    s_memoryCard.menuRow = 1;
                                    s_memoryCard.menuSelection = 0;
                                    s_memoryCard.noCardTicks = noCardTicks[ti];
                                    s_memoryCard.settleTicks = 0;
                                    s_memoryCard.slot = 1;
                                    g_SceneId = 26;
                                    g_SceneTimer = 0x40;
                                    GameMenuLoadPhase = 0;
                                    memset(s_memoryCard.slots.headers, 0,
                                           sizeof(s_memoryCard.slots.headers));

                                    s_memoryCard.menuState = states[si];
                                    s_memoryCard.action.state = actions[ai];
                                    s_memoryCard.menuPage = page;
                                    s_memoryCard.saveMode = mode;
                                    s_memoryCard.freeBlocks = freeBlocks;
                                    s_memoryCard.slots.usedMask = mask;
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
