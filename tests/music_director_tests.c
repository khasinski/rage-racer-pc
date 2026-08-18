#include "game/music_director.h"

#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) return 1; } while (0)

int main(void) {
    MusicDirectorState state = {7, 9, CD_COMMAND_PAUSE, 3, 5, 0};

    MusicDirectorRequestTrack(&state, 260);
    EXPECT_EQ(4, state.trackPending);
    EXPECT_EQ(0, state.trackStep);
    EXPECT_EQ(CD_COMMAND_NONE, state.commandPending);
    EXPECT_EQ(0, state.commandStep);

    MusicDirectorRequestPause(&state);
    EXPECT_EQ(CD_COMMAND_PAUSE, state.commandPending);
    MusicDirectorRequestResume(&state);
    EXPECT_EQ(CD_COMMAND_RESUME, state.commandPending);

    state.currentTrack = 9;
    state.restartOnResume = 1;
    MusicDirectorRequestResume(&state);
    EXPECT_EQ(9, state.trackPending);
    EXPECT_EQ(4, state.trackStep);
    EXPECT_EQ(0, state.restartOnResume);
    EXPECT_EQ(CD_COMMAND_PLAY, state.commandPending);

    state.currentTrack = 6;
    MusicDirectorLoopCurrent(&state);
    EXPECT_EQ(6, state.trackPending);
    EXPECT_EQ(4, state.trackStep);
    EXPECT_EQ(CD_COMMAND_PLAY, state.commandPending);

    MusicDirectorReset(&state);
    EXPECT_EQ(-1, state.trackPending);
    EXPECT_EQ(2, state.currentTrack);
    EXPECT_EQ(CD_COMMAND_NONE, state.commandPending);
    return 0;
}
