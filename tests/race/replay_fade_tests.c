#include <assert.h>

#include "game/race.h"
#include "game/race_internal.h"
#include "game/render.h"
#include "game/replay_internal.h"
#include "game/state.h"

s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_EndingWashLevel;
s32 g_SeriesCleared;
s32 g_SceneTimer;
s32 g_ReplayFrameCount;
s32 g_ReplayBufferWrapped;
u16 g_PadPressed;
s32 g_MirrorMode;
s32 g_SceneId;
s16 g_GrandPrixMode;

static s32 s_AudioFade;
static s32 s_FadeDraws;
static s32 s_FadeColor;
static s32 s_FadeTpage;
static s32 s_WashDraws;
static s32 s_WashProgress;
static s32 s_WashFade;

void StartCdVolumeFade(s32 frames) { s_AudioFade = frames; }
void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    s_FadeDraws++;
    s_FadeColor = color;
    s_FadeTpage = tpage;
}
void DrawSeriesClearedWash(s32 progress, s32 fade) {
    s_WashDraws++;
    s_WashProgress = progress;
    s_WashFade = fade;
}

static void ResetState(void) {
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_EndingWashLevel = 99;
    g_SeriesCleared = 0;
    g_SceneTimer = 0;
    g_ReplayFrameCount = 1000;
    g_ReplayBufferWrapped = 0;
    g_PadPressed = 0;
    g_MirrorMode = 1;
    g_SceneId = 99;
    g_GrandPrixMode = 0;
    s_AudioFade = -1;
    s_FadeDraws = 0;
    s_WashDraws = 0;
}

static void TestFadeInClampsAtClear(void) {
    ResetState();
    g_FadeLevel = 2;
    g_FadeStep = -4;

    UpdateReplayFade();

    assert(g_FadeLevel == 0 && g_FadeStep == 0);
    assert(g_EndingWashLevel == 0);
    assert(s_FadeDraws == 1 && s_FadeColor == 0 && s_FadeTpage == 0x29);
}

static void TestSeriesClearWashProgress(void) {
    ResetState();
    g_SeriesCleared = 1;
    g_SceneTimer = 401;

    UpdateReplayFade();

    assert(g_EndingWashLevel == 1);
    assert(s_WashDraws == 1 && s_WashProgress == 1 && s_WashFade == 0);
    assert(s_FadeDraws == 0);
}

static void TestConfirmStartsAudioFade(void) {
    ResetState();
    g_PadPressed = PAD_CONFIRM;

    UpdateReplayFade();

    assert(g_FadeStep == 4);
    assert(g_FadeLevel == 0);
    assert(s_AudioFade == 60);
}

static void TestWrappedReplayAutoFadeKeepsAudio(void) {
    ResetState();
    g_SceneTimer = 932;
    g_ReplayBufferWrapped = 1;

    UpdateReplayFade();

    assert(g_FadeStep == 4);
    assert(s_AudioFade == -1);
}

static void TestLinearReplayAutoFadeFadesAudio(void) {
    ResetState();
    g_SceneTimer = 932;

    UpdateReplayFade();

    assert(g_FadeStep == 4);
    assert(s_AudioFade == 60);
}

static void TestOpaqueFadeSelectsResultScene(void) {
    ResetState();
    g_FadeLevel = 254;
    g_FadeStep = 4;
    g_GrandPrixMode = 2;

    UpdateReplayFade();

    assert(g_FadeLevel == 258);
    assert(g_MirrorMode == 0);
    assert(g_SceneId == 0x12);
    assert(s_FadeDraws == 1 && s_FadeColor == 258 &&
           s_FadeTpage == 0x49);

    ResetState();
    g_FadeLevel = 254;
    g_FadeStep = 4;

    UpdateReplayFade();

    assert(g_SceneId == 0x14);
}

int main(void) {
    TestFadeInClampsAtClear();
    TestSeriesClearWashProgress();
    TestConfirmStartsAudioFade();
    TestWrappedReplayAutoFadeKeepsAudio();
    TestLinearReplayAutoFadeFadesAudio();
    TestOpaqueFadeSelectsResultScene();
    return 0;
}
