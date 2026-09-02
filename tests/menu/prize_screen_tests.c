/*
 * The prize screen counts money into the save block, and the save block is the
 * one piece of a race that outlives it.
 *
 * verify_grand_prix_results.py already drives a whole race into this screen,
 * but it only watches the scene numbers go by; nothing checked that the money
 * arithmetic lands. The screen is a state machine over globals, so it can be
 * stepped a frame at a time here and asked what the player was actually paid.
 */

#include "common.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/sound.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

void UpdatePrizeMoneyScreen(void);

/* The screen's own state. */
PrizeScreenState g_PrizeScreenState;
s32 g_PrizeAmount;
s32 g_PromotionBonus;
s32 g_PrizeCountStep;
s32 g_BonusCountStep;
s32 g_ClassClearFanfareTimer;
s32 g_ClassCompleted;
s32 g_SeriesCleared;
s32 g_SceneTimer;
GameRaceProgress *g_RaceProgress;
u16 g_PadPressed;
u16 g_PadHeld;

/* The screen only draws and plays cues through these; what it draws is not
 * this test's subject, but whether it asked to is. */
static int s_bgmRequested;
static int s_classAdvanced;
static int s_fanfareTicks;

void DrawFullscreenFadeTile(s32 step, s32 clut) { (void)step; (void)clut; }
void DrawGrandPrixIntro(void) {}
void DrawPrizeMoneyPanel(s32 step) { (void)step; }
void DrawRaceTimePanel(s32 step) { (void)step; }
void PlaySoundCue(s32 cue) { (void)cue; }
s32 RequestSelectBgmAssets(void) {
    s_bgmRequested++;
    return 0;
}
void AdvanceGrandPrixClass(void) { s_classAdvanced++; }
void TickClassClearFanfare(void) { s_fanfareTicks++; }

static GameRaceProgress s_progress;
static int s_failures;

static void Check(int condition, const char *what, s32 got, s32 wanted) {
    if (condition) return;
    printf("FAIL %s: got %d, expected %d\n", what, got, wanted);
    s_failures++;
}

static void Reset(s32 prize, s32 bonus) {
    memset(&s_progress, 0, sizeof(s_progress));
    g_RaceProgress = &s_progress;
    g_PrizeScreenState = PRIZE_SCREEN_STATE_INTRO_FADE_IN;
    g_PrizeAmount = prize;
    g_PromotionBonus = bonus;
    g_PrizeCountStep = 100;
    g_BonusCountStep = 50;
    g_ClassClearFanfareTimer = 0;
    g_ClassCompleted = 0;
    g_SeriesCleared = 0;
    g_SceneTimer = 0x100;
    g_PadPressed = 0;
    g_PadHeld = 0;
    s_bgmRequested = 0;
    s_classAdvanced = 0;
    s_fanfareTicks = 0;
}

/*
 * Run the screen until it stops changing state or the budget runs out, holding
 * confirm as told and pressing it on the first frame of each new state so the
 * waits move on.
 */
static int RunToEnd(int hold, int budget) {
    PrizeScreenState previous = g_PrizeScreenState;
    int frames = 0;

    while (frames < budget) {
        g_PadHeld = hold ? PAD_CONFIRM : 0;
        g_PadPressed = (g_PrizeScreenState != previous || frames == 0)
                           ? PAD_CONFIRM
                           : 0;
        previous = g_PrizeScreenState;
        UpdatePrizeMoneyScreen();
        frames++;
        if (g_PrizeScreenState == PRIZE_SCREEN_STATE_FADE_OUT) {
            break;
        }
    }
    return frames;
}

int main(void) {
    /* Everything owed reaches the player, prize first and then the bonus. */
    Reset(1000, 400);
    RunToEnd(0, 4000);
    Check(s_progress.money.value == 1400, "prize and bonus both paid",
          s_progress.money.value, 1400);
    Check(g_PrizeAmount == 0, "nothing left owed on the prize", g_PrizeAmount,
          0);
    Check(g_PromotionBonus == 0, "nothing left owed on the bonus",
          g_PromotionBonus, 0);
    Check(g_PrizeScreenState == PRIZE_SCREEN_STATE_FADE_OUT,
          "screen finishes on the fade", g_PrizeScreenState,
          PRIZE_SCREEN_STATE_FADE_OUT);

    /* Paying a prize cannot overflow the save's advertised money limit. */
    Reset(1000, 400);
    s_progress.money.value = RACE_MAX_PRIZE_MONEY - 50;
    RunToEnd(0, 4000);
    Check(s_progress.money.value == RACE_MAX_PRIZE_MONEY,
          "prize money saturates at the save limit", s_progress.money.value,
          RACE_MAX_PRIZE_MONEY);
    Check(g_PrizeAmount == 0 && g_PromotionBonus == 0,
          "capped money still finishes both counters", g_PrizeAmount,
          0);

    /* The counter waits out the panel before it starts, and the wait is
     * counted in frames rather than eyeballed. */
    {
        int i;

        Reset(1000, 0);
        g_PrizeScreenState = PRIZE_SCREEN_STATE_COUNT_PRIZE;
        g_SceneTimer = 0;
        for (i = 0; i < 120; i++) {
            UpdatePrizeMoneyScreen();
        }
        Check(s_progress.money.value == 0, "nothing is paid during the wait",
              s_progress.money.value, 0);
        UpdatePrizeMoneyScreen();
        Check(s_progress.money.value == 100, "the first step lands on frame 121",
              s_progress.money.value, 100);
    }

    /* An amount that is not a whole number of steps still pays out exactly,
     * rather than overshooting on the last one. */
    Reset(1050, 425);
    RunToEnd(0, 4000);
    Check(s_progress.money.value == 1475, "part of a step still pays exactly",
          s_progress.money.value, 1475);

    /* No bonus at all skips straight past the bonus states. */
    Reset(300, 0);
    RunToEnd(0, 4000);
    Check(s_progress.money.value == 300, "prize alone", s_progress.money.value,
          300);
    Check(s_classAdvanced == 0, "the class only advances after the fade",
          s_classAdvanced, 0);

    /* Nothing owed at all still walks the screen to its end. */
    Reset(0, 0);
    RunToEnd(0, 4000);
    Check(s_progress.money.value == 0, "nothing owed pays nothing",
          s_progress.money.value, 0);
    Check(g_PrizeScreenState == PRIZE_SCREEN_STATE_FADE_OUT,
          "nothing owed still reaches the fade", g_PrizeScreenState,
          PRIZE_SCREEN_STATE_FADE_OUT);

    /* Holding confirm counts four times as fast and pays the same total. */
    {
        int slow, fast;

        Reset(4000, 2000);
        slow = RunToEnd(0, 20000);
        Check(s_progress.money.value == 6000, "unhurried total",
              s_progress.money.value, 6000);

        Reset(4000, 2000);
        fast = RunToEnd(1, 20000);
        Check(s_progress.money.value == 6000, "hurried total",
              s_progress.money.value, 6000);
        if (fast >= slow) {
            printf("FAIL holding confirm did not speed the count up: "
                   "%d frames held against %d\n", fast, slow);
            s_failures++;
        }
    }

    /* The fanfare has to finish before the screen will leave, however hard
     * the player presses. */
    Reset(0, 0);
    g_PrizeScreenState = PRIZE_SCREEN_STATE_WAIT_TO_FINISH;
    g_ClassClearFanfareTimer = 1;
    {
        int i;
        for (i = 0; i < 100; i++) {
            g_PadPressed = PAD_CONFIRM;
            UpdatePrizeMoneyScreen();
        }
    }
    Check(g_PrizeScreenState == PRIZE_SCREEN_STATE_WAIT_TO_FINISH,
          "the fanfare holds the screen", g_PrizeScreenState,
          PRIZE_SCREEN_STATE_WAIT_TO_FINISH);
    Check(s_fanfareTicks == 100, "and keeps being ticked", s_fanfareTicks, 100);

    /* Leaving asks for the menu music back only when the class is unfinished. */
    Reset(0, 0);
    g_PrizeScreenState = PRIZE_SCREEN_STATE_WAIT_TO_FINISH;
    g_PadPressed = PAD_CONFIRM;
    UpdatePrizeMoneyScreen();
    Check(s_bgmRequested == 1, "unfinished class asks for the menu music",
          s_bgmRequested, 1);

    Reset(0, 0);
    g_PrizeScreenState = PRIZE_SCREEN_STATE_WAIT_TO_FINISH;
    g_ClassCompleted = 1;
    g_PadPressed = PAD_CONFIRM;
    UpdatePrizeMoneyScreen();
    Check(s_bgmRequested == 0, "a finished class keeps its own music",
          s_bgmRequested, 0);

    /* The fade runs slower once the whole series is done, and hands over at
     * the end either way. */
    {
        int i;

        Reset(0, 0);
        g_PrizeScreenState = PRIZE_SCREEN_STATE_FADE_OUT;
        g_SceneTimer = 0;
        for (i = 0; i < 300; i++) {
            UpdatePrizeMoneyScreen();
        }
        Check(s_classAdvanced > 0, "the fade hands over to the next class",
              s_classAdvanced, 1);

        Reset(0, 0);
        g_PrizeScreenState = PRIZE_SCREEN_STATE_FADE_OUT;
        g_SeriesCleared = 1;
        g_SceneTimer = 0;
        for (i = 0; i < 128; i++) {
            UpdatePrizeMoneyScreen();
        }
        Check(s_classAdvanced == 0, "a cleared series fades at half the speed",
              s_classAdvanced, 0);
    }

    if (s_failures != 0) {
        printf("%d prize screen checks failed\n", s_failures);
        return 1;
    }
    printf("the prize screen pays out everything it owes\n");
    return 0;
}
