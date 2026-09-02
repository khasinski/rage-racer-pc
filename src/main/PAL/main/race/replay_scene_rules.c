#include "game/race_internal.h"

enum {
    REPLAY_ENDING_WASH_FRAMES = 600,
    REPLAY_EXIT_FADE_FRAMES = 68,
    MAX_WASH_LEVEL = 255,
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

s32 NextReplayReadCursor(s32 cursor, s32 frameCount) {
    const s32 next = cursor + 1;
    return next < frameCount ? next : 0;
}
