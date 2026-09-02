#include "common.h"
#include "game/asset.h"
#include "game/cd.h"
#include "game/fmv.h"
#include "game/state.h"

#include <stdio.h>

FmvPlaybackState g_FmvState;
s32 g_SceneId;
s32 g_StreamReturnScene;

static s32 s_closeCalls;
static s32 s_resetCalls;
static s32 s_cdSyncCalls;
static long s_cdSyncMode;
static long s_cdCommand;
static s32 s_startCalls;
static s32 s_decodeCalls;
static s32 s_endCalls;
static s32 s_failures;

void CloseLoadedAudioSlots(void) {
    s_closeCalls++;
}
void ResetCdAudioState(void) { s_resetCalls++; }
long CdSync(long mode, u8 *result) {
    (void)result;
    s_cdSyncCalls++;
    s_cdSyncMode = mode;
    return 0;
}
long CdControl(long command, void *parameter, u8 *result) {
    (void)parameter;
    (void)result;
    s_cdCommand = command;
    return 1;
}
void StartFmvPlayback(void) {
    s_startCalls++;
}
void DecodeFmvFrame(void) { s_decodeCalls++; }
void EndFmv(void) { s_endCalls++; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestBeginFmv(void) {
    BeginFmv(27);

    Check(s_closeCalls == 1 && s_resetCalls == 1,
          "FMV start releases game audio");
    Check(g_FmvState == FMV_PLAYBACK_START && g_StreamReturnScene == 27 &&
              g_SceneId == 5,
          "FMV start records playback and return states");
    Check(s_cdSyncCalls == 1 && s_cdSyncMode == CD_SYNC_WAIT &&
              s_cdCommand == CD_DRIVE_PAUSE,
          "FMV start pauses the current CD operation");
}

static void TestUpdateFmv(void) {
    g_FmvState = FMV_PLAYBACK_INVALID;
    UpdateFmv();
    Check(s_startCalls == 0 && s_decodeCalls == 0 && s_endCalls == 0,
          "invalid FMV state does nothing");

    g_FmvState = FMV_PLAYBACK_START;
    UpdateFmv();
    Check(s_startCalls == 1 && s_decodeCalls == 1,
          "FMV start initializes and decodes in the same frame");

    g_FmvState = FMV_PLAYBACK_DECODE;
    UpdateFmv();
    Check(s_startCalls == 1 && s_decodeCalls == 2,
          "FMV decode state advances one frame");

    g_FmvState = FMV_PLAYBACK_FINISH;
    UpdateFmv();
    Check(s_endCalls == 1, "FMV finish state ends playback");
}

int main(void) {
    TestBeginFmv();
    TestUpdateFmv();

    if (s_failures != 0) return 1;
    puts("FMV scene initializes and dispatches every playback state");
    return 0;
}
