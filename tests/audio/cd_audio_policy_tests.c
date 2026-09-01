#include "game/cd.h"

#define EXPECT_TRUE(value) do { if (!(value)) return 1; } while (0)
#define EXPECT_FALSE(value) EXPECT_TRUE(!(value))

int main(void) {
    EXPECT_TRUE(CdAudioRequestsIdle(-1, CD_COMMAND_NONE));
    EXPECT_FALSE(CdAudioRequestsIdle(3, CD_COMMAND_NONE));
    EXPECT_FALSE(CdAudioRequestsIdle(-1, CD_COMMAND_PLAY));
    EXPECT_FALSE(CdAudioRequestsIdle(3, CD_COMMAND_PLAY));

    EXPECT_FALSE(CdTrackHasLoopPoint(100, 100));
    EXPECT_FALSE(CdTrackHasLoopPoint(100, 99));
    EXPECT_TRUE(CdTrackHasLoopPoint(100, 101));

    EXPECT_FALSE(CdPlaybackPassedLoopPoint(100, 100, 200));
    EXPECT_FALSE(CdPlaybackPassedLoopPoint(100, 150, 149));
    EXPECT_TRUE(CdPlaybackPassedLoopPoint(100, 150, 150));
    EXPECT_TRUE(CdPlaybackPassedLoopPoint(100, 150, 151));
    return 0;
}
