#include "game/cd.h"

#define EXPECT_TRUE(value) do { if (!(value)) return 1; } while (0)
#define EXPECT_FALSE(value) EXPECT_TRUE(!(value))

int main(void) {
    EXPECT_TRUE(CdAudioRequestsIdle(-1, CD_COMMAND_NONE));
    EXPECT_FALSE(CdAudioRequestsIdle(3, CD_COMMAND_NONE));
    EXPECT_FALSE(CdAudioRequestsIdle(-1, CD_COMMAND_PLAY));
    EXPECT_FALSE(CdAudioRequestsIdle(3, CD_COMMAND_PLAY));
    return 0;
}
