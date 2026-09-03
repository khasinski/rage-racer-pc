#include "game/audio.h"
#include "game/cd.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/replay_internal.h"
#include "game/state.h"

enum {
    REPLAY_FADE_STEP = 4,
    REPLAY_AUDIO_FADE_FRAMES = 60,
    REPLAY_FADE_IN_TPAGE = 0x29,
    REPLAY_FADE_OUT_TPAGE = 0x49,
    TIME_ATTACK_RESULT_SCENE = 0x14,
    GRAND_PRIX_RESULT_SCENE = 0x12,
};

static void StartReplayExitFade(s32 fadeAudio) {
    g_FadeStep = REPLAY_FADE_STEP;
    if (fadeAudio != 0) {
        StartCdVolumeFade(REPLAY_AUDIO_FADE_FRAMES);
    }
}

void UpdateReplayFade(void) {
    s32 endingWashActive;

    if (g_FadeStep < 0) {
        g_FadeLevel = AdvanceReplayFadeLevel(g_FadeLevel, g_FadeStep);
        if (g_FadeLevel == 0) {
            g_FadeStep = 0;
            g_EndingWashLevel = 0;
        }
        DrawFullscreenFadeTile(g_FadeLevel, REPLAY_FADE_IN_TPAGE);
        return;
    }

    endingWashActive = g_SeriesCleared != 0 &&
                       ReplayEndingWashActive(g_SceneTimer,
                                              g_ReplayFrameCount);
    if (endingWashActive) {
        g_EndingWashLevel = ReplayEndingWashLevel(
            g_SceneTimer, g_ReplayFrameCount);
    }

    if (g_FadeStep == 0) {
        if (g_PadPressed & PAD_CONFIRM) {
            StartReplayExitFade(1);
        } else if (ShouldStartReplayExitFade(
                       g_SceneTimer, g_ReplayFrameCount)) {
            StartReplayExitFade(g_ReplayBufferWrapped == 0);
        }
    } else {
        g_FadeLevel = AdvanceReplayFadeLevel(g_FadeLevel, g_FadeStep);
        if (g_FadeLevel >= REPLAY_OPAQUE_FADE) {
            g_MirrorMode = 0;
            g_SceneId = g_GrandPrixMode == 0
                ? TIME_ATTACK_RESULT_SCENE
                : GRAND_PRIX_RESULT_SCENE;
        }
    }

    if (g_SeriesCleared != 0) {
        if (endingWashActive || g_FadeLevel != 0) {
            DrawSeriesClearedWash(g_EndingWashLevel, g_FadeLevel);
        }
    } else if (g_FadeLevel != 0) {
        DrawFullscreenFadeTile(g_FadeLevel, REPLAY_FADE_OUT_TPAGE);
    }
}
