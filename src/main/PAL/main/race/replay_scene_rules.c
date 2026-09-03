#include "game/race_internal.h"

enum {
    REPLAY_ENDING_WASH_FRAMES = 600,
    REPLAY_EXIT_FADE_FRAMES = 68,
    MAX_WASH_LEVEL = 255,
    REPLAY_RESULT_CUE_FRAME = 60,
    REPLAY_WIN_CUE = 0x40,
    REPLAY_LOSS_CUE = 0x41,
    NO_REPLAY_RESULT_CUE = -1,
};

s32 ReplayEndingWashActive(s32 sceneTimer, s32 frameCount) {
    return frameCount >= REPLAY_ENDING_WASH_FRAMES &&
           sceneTimer > frameCount - REPLAY_ENDING_WASH_FRAMES;
}

s32 ReplayEndingWashLevel(s32 sceneTimer, s32 frameCount) {
    s32 level;

    if (!ReplayEndingWashActive(sceneTimer, frameCount)) {
        return 0;
    }

    level = sceneTimer + REPLAY_ENDING_WASH_FRAMES - frameCount;
    return level > MAX_WASH_LEVEL ? MAX_WASH_LEVEL : level;
}

s32 ShouldStartReplayExitFade(s32 sceneTimer, s32 frameCount) {
    return frameCount >= REPLAY_EXIT_FADE_FRAMES &&
           sceneTimer == frameCount - REPLAY_EXIT_FADE_FRAMES;
}

s32 ReplayBadgeVisible(s32 sceneTimer, s32 seriesCleared) {
    return (sceneTimer & 16) != 0 && seriesCleared == 0;
}

s32 ReplayResultCue(s32 sceneTimer, s32 grandPrixMode, s32 seriesCleared,
                    s32 racePosition) {
    if (sceneTimer != REPLAY_RESULT_CUE_FRAME || grandPrixMode == 0 ||
        seriesCleared != 0) {
        return NO_REPLAY_RESULT_CUE;
    }
    return racePosition == 1 ? REPLAY_WIN_CUE : REPLAY_LOSS_CUE;
}

s32 NextReplayReadCursor(s32 cursor, s32 frameCount) {
    if (frameCount <= 0 || cursor < 0 || cursor >= frameCount - 1) {
        return 0;
    }
    return cursor + 1;
}
