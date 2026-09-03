#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/replay_internal.h"
#include "game/state.h"
#include "game/track_internal.h"

enum {
    REPLAY_INITIAL_FADE = 255,
    REPLAY_FADE_IN_STEP = -4,
    GRAND_PRIX_ENVIRONMENT_REWIND = 1800,
    TIME_ATTACK_ENVIRONMENT_REWIND = 3000,
};

static s32 WrappedReplayStartCursor(s32 writeCursor) {
    u32 nextSample;

    if (writeCursor < 0 || g_ReplayFrameCount <= 0) {
        return 0;
    }
    nextSample = ((u32)writeCursor &
                  ~(u32)(REPLAY_SUBFRAMES_PER_SAMPLE - 1)) +
                 REPLAY_SUBFRAMES_PER_SAMPLE;
    return nextSample < (u32)g_ReplayFrameCount ? (s32)nextSample : 0;
}

void BeginReplay(void) {
    g_FadeLevel = REPLAY_INITIAL_FADE;
    g_SceneTimer = 0;
    g_FadeStep = REPLAY_FADE_IN_STEP;

    if (g_ReplayBufferWrapped != 0) {
        g_ReplayReadCursor = WrappedReplayStartCursor(g_ReplayWriteCursor);
    } else {
        g_ReplayReadCursor = 0;
        /* The last sample straddles the transition out of the live race. */
        g_ReplayFrameCount = g_ReplayWriteCursor > REPLAY_SUBFRAMES_PER_SAMPLE
            ? g_ReplayWriteCursor - REPLAY_SUBFRAMES_PER_SAMPLE
            : 0;
    }

    if (g_GrandPrixClass != GRAND_PRIX_FINAL_CLASS_INDEX) {
        const s32 rewindFrames = g_GrandPrixMode != 0
            ? GRAND_PRIX_ENVIRONMENT_REWIND
            : TIME_ATTACK_ENVIRONMENT_REWIND;

        SeekEnvironmentScript(g_EnvScriptClock - rewindFrames);
    }

    SeedReplayCars();
}
