#include "game/race_pause.h"
#include "game/state.h"

static int failures;
#define EXPECT_EQ(expected, actual) do {                                      \
    if ((int)(expected) != (int)(actual)) failures++;                         \
} while (0)

static RacePauseState DefaultState(void) {
    RacePauseState state = {300, 0, 0, 2, 0, 1, 9, 2, 0};
    return state;
}

static void test_pause_and_navigation(void) {
    RacePauseState state = DefaultState();
    RacePauseCommands commands;
    RacePauseStep(&state, PAD_START, &commands);
    EXPECT_EQ(1, state.paused);
    EXPECT_EQ(299, state.sceneTimer);
    EXPECT_EQ(1, commands.pauseCd);
    EXPECT_EQ(0, commands.effectVoicesEnabled);
    EXPECT_EQ(2, commands.soundCues[0]);

    state.debounce = 0;
    RacePauseStep(&state, PAD_DOWN, &commands);
    EXPECT_EQ(1, state.optionCursor);
    EXPECT_EQ(1, commands.soundCues[0]);
}

static void test_grand_prix_retire(void) {
    RacePauseState state = DefaultState();
    RacePauseCommands commands;
    state.paused = 1;
    state.optionCursor = 1;
    RacePauseStep(&state, PAD_START, &commands);
    EXPECT_EQ(0, state.paused);
    EXPECT_EQ(5, state.phase);
    EXPECT_EQ(1, state.retireCameraActive);
    EXPECT_EQ(8, commands.startCdFadeFrames);
    EXPECT_EQ(0x3D, commands.soundCues[0]);
}

static void test_time_attack_exit(void) {
    RacePauseState state = DefaultState();
    RacePauseCommands commands;
    state.grandPrixMode = 0;
    state.paused = 1;
    state.optionCursor = 1;
    RacePauseStep(&state, PAD_START, &commands);
    EXPECT_EQ(8, state.phase);
    EXPECT_EQ(0xB, commands.exitRaceScene);
}

int main(void) {
    test_pause_and_navigation();
    test_grand_prix_retire();
    test_time_attack_exit();
    return failures != 0;
}
