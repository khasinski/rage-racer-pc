#include <assert.h>
#include <limits.h>

#include "game/race.h"
#include "game/race_internal.h"
#include "game/replay_internal.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track_internal.h"

Replay g_Replay;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_SceneTimer;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s32 g_EnvScriptClock;

static s32 s_EnvironmentSeek;
static s32 s_SeedCalls;

void SeekEnvironmentScript(s32 frame) { s_EnvironmentSeek = frame; }
void SeedReplayCars(void) { s_SeedCalls++; }

static void ResetState(void) {
    g_FadeLevel = 0;
    g_FadeStep = 0;
    g_SceneTimer = 99;
    g_Replay.wrapped = 0;
    g_Replay.write = 0;
    g_Replay.read = -1;
    g_Replay.count = 0;
    g_GrandPrixClass = 0;
    g_GrandPrixMode = 0;
    g_EnvScriptClock = 5000;
    s_EnvironmentSeek = -1;
    s_SeedCalls = 0;
}

static void TestLinearTimeAttackReplay(void) {
    ResetState();
    g_Replay.write = 102;

    BeginReplay();

    assert(g_FadeLevel == 255 && g_FadeStep == -4 && g_SceneTimer == 0);
    assert(g_Replay.read == 0);
    assert(g_Replay.count == 100);
    assert(s_EnvironmentSeek == 2000);
    assert(s_SeedCalls == 1);
}

static void TestWrappedGrandPrixReplay(void) {
    ResetState();
    g_Replay.wrapped = 1;
    g_Replay.write = 101;
    g_Replay.count = 200;
    g_GrandPrixMode = 2;

    BeginReplay();

    assert(g_Replay.read == 102);
    assert(g_Replay.count == 200);
    assert(s_EnvironmentSeek == 3200);
}

static void TestWrappedCursorAtEndRestartsFromZero(void) {
    ResetState();
    g_Replay.wrapped = 1;
    g_Replay.write = 199;
    g_Replay.count = 200;

    BeginReplay();

    assert(g_Replay.read == 0);
}

static void TestFinalClassKeepsEnvironmentPosition(void) {
    ResetState();
    g_Replay.write = 20;
    g_GrandPrixClass = GRAND_PRIX_FINAL_CLASS_INDEX;

    BeginReplay();

    assert(s_EnvironmentSeek == -1);
    assert(s_SeedCalls == 1);
}

static void TestInvalidAndShortReplayBounds(void) {
    assert(ClampReplayFrameCount(-1, 0) == 0);
    assert(ClampReplayFrameCount(INT_MAX, 0) ==
           TIME_ATTACK_REPLAY_SUBFRAME_COUNT);
    assert(ClampReplayFrameCount(INT_MAX, 1) ==
           GRAND_PRIX_REPLAY_SUBFRAME_COUNT);

    ResetState();
    g_Replay.write = 1;

    BeginReplay();

    assert(g_Replay.read == 0 && g_Replay.count == 0);

    ResetState();
    g_Replay.wrapped = 1;
    g_Replay.write = -1;
    g_Replay.count = 200;
    BeginReplay();
    assert(g_Replay.read == 0);

    g_Replay.write = INT_MAX;
    BeginReplay();
    assert(g_Replay.read == 0);
    assert(g_Replay.count == 200);

    ResetState();
    g_Replay.wrapped = 1;
    g_Replay.write = TIME_ATTACK_REPLAY_SUBFRAME_COUNT;
    g_Replay.count = INT_MAX;
    BeginReplay();
    assert(g_Replay.read == 0);
    assert(g_Replay.count == TIME_ATTACK_REPLAY_SUBFRAME_COUNT);
}

static void TestEnvironmentRewindBounds(void) {
    assert(ReplayEnvironmentRewindTarget(5000, 0) == 2000);
    assert(ReplayEnvironmentRewindTarget(5000, 1) == 3200);
    assert(ReplayEnvironmentRewindTarget(INT_MIN, 0) == INT_MIN);
}

int main(void) {
    TestLinearTimeAttackReplay();
    TestWrappedGrandPrixReplay();
    TestWrappedCursorAtEndRestartsFromZero();
    TestFinalClassKeepsEnvironmentPosition();
    TestInvalidAndShortReplayBounds();
    TestEnvironmentRewindBounds();
    return 0;
}
