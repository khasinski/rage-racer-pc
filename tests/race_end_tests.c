#include "game/race_end.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    RaceEndState state = {5, 0, 10, 0};
    RaceEndCommands commands;

    RaceEndStep(&state, &commands);
    EXPECT_EQ(15, commands.requestCdTrack);
    EXPECT_EQ(1, commands.startCd);
    EXPECT_EQ(-1, commands.exitScene);
    EXPECT_EQ(11, state.fadeTimer);

    state.fadeTimer = 21;
    RaceEndStep(&state, &commands);
    EXPECT_EQ(3, commands.drawEndBannerIntensity);
    EXPECT_EQ(3, commands.drawFadeIntensity);

    state.fadeTimer = 101;
    RaceEndStep(&state, &commands);
    EXPECT_EQ(15, commands.exitScene);

    state = (RaceEndState){5, 1, 4, 2};
    RaceEndStep(&state, &commands);
    EXPECT_EQ(8, commands.drawLostCaptionIntensity);
    EXPECT_EQ(8, commands.drawFadeIntensity);
    EXPECT_EQ(-1, commands.exitScene);
    state.fadeTimer = 126;
    RaceEndStep(&state, &commands);
    EXPECT_EQ(13, commands.exitScene);

    state = (RaceEndState){7, 1, 0, 0};
    RaceEndStep(&state, &commands);
    EXPECT_EQ(6, commands.exitScene);
    EXPECT_EQ(0, state.fadeTimer);
    return 0;
}
