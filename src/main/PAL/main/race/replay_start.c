#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/track_internal.h"

#include <stdint.h>

enum {
    REPLAY_INITIAL_FADE = 255,
    REPLAY_FADE_IN_STEP = -4,
    GRAND_PRIX_ENVIRONMENT_REWIND = 1800,
    TIME_ATTACK_ENVIRONMENT_REWIND = 3000,
};

static s32 WrappedReplayStartCursor(s32 writeCursor, s32 frameCount) {
    u32 nextSample;

    if (frameCount <= 0 || writeCursor < 0 || writeCursor >= frameCount) {
        return 0;
    }
    nextSample = ((u32)writeCursor &
                  ~(u32)(REPLAY_SUBFRAMES_PER_SAMPLE - 1)) +
                 REPLAY_SUBFRAMES_PER_SAMPLE;
    return nextSample < (u32)frameCount ? (s32)nextSample : 0;
}

s32 ClampReplayFrameCount(s32 frameCount, s32 grandPrixMode) {
    const s32 capacity = ReplayFrameCapacity(grandPrixMode);

    if (frameCount <= 0) {
        return 0;
    }
    return frameCount < capacity ? frameCount : capacity;
}

s32 ReplayEnvironmentRewindTarget(s32 clock, s32 grandPrixMode) {
    const s32 rewindFrames = grandPrixMode != 0
        ? GRAND_PRIX_ENVIRONMENT_REWIND
        : TIME_ATTACK_ENVIRONMENT_REWIND;
    const int64_t target = (int64_t)clock - rewindFrames;

    return target < INT32_MIN ? INT32_MIN : (s32)target;
}

void BeginReplay(void) {
    g_FadeLevel = REPLAY_INITIAL_FADE;
    g_SceneTimer = 0;
    g_FadeStep = REPLAY_FADE_IN_STEP;

    if (g_ReplayBufferWrapped != 0) {
        g_ReplayFrameCount = ClampReplayFrameCount(
            g_ReplayFrameCount, g_GrandPrixMode);
        g_ReplayReadCursor = WrappedReplayStartCursor(
            g_ReplayWriteCursor, g_ReplayFrameCount);
    } else {
        g_ReplayReadCursor = 0;
        /* The last sample straddles the transition out of the live race. */
        g_ReplayFrameCount = ClampReplayFrameCount(
            g_ReplayWriteCursor > REPLAY_SUBFRAMES_PER_SAMPLE
                ? g_ReplayWriteCursor - REPLAY_SUBFRAMES_PER_SAMPLE
                : 0,
            g_GrandPrixMode);
    }

    if (g_GrandPrixClass != GRAND_PRIX_FINAL_CLASS_INDEX) {
        SeekEnvironmentScript(ReplayEnvironmentRewindTarget(
            g_EnvScriptClock, g_GrandPrixMode));
    }

    SeedReplayCars();
}
