#include "game/pad.h"
#include "game/race_session.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return __LINE__; } while (0)

int main(void) {
    RaceSession session = {
        {300, 1, 0, 2, 1, 1, 9, 2, 0},
        {2, 1, 9, 2}
    };
    RaceSessionCommands commands;

    RaceSessionStep(&session, PAD_START, &commands);
    EXPECT_EQ(5, session.pause.phase);
    EXPECT_EQ(5, session.end.phase);
    EXPECT_EQ(1, session.pause.fadeTimer);
    EXPECT_EQ(1, commands.end.disableMirror);
    EXPECT_EQ(0, commands.end.drawLostCaptionIntensity);
    EXPECT_EQ(8, commands.pause.startCdFadeFrames);

    session = (RaceSession){
        {300, 1, 0, 2, 2, 0, 9, 0, 0},
        {2, 0, 9, 0}
    };
    RaceSessionStep(&session, PAD_START, &commands);
    EXPECT_EQ(7, session.pause.phase);
    EXPECT_EQ(6, commands.end.exitScene);
    EXPECT_EQ(1, commands.pause.updateTimeAttackRecord);
    return 0;
}
