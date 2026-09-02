#include "common.h"
#include "game/cd.h"
#include "game/fmv.h"
#include "game/state.h"
#include "psyq/cd.h"

#include <stdio.h>

s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_FrameSyncThreshold;
s32 g_SceneId;
s32 g_SceneTimer;

static s32 s_assetRequests;
static s32 s_cdCommand;
static s32 s_cdSyncCalls;
static s32 s_displayBlue;
static s32 s_displayCalls;
static s32 s_displayGreen;
static s32 s_displayRed;
static s32 s_displayMask = -1;
static s32 s_failures;

long CdSync(long mode, u8 *result) {
    (void)mode;
    (void)result;
    s_cdSyncCalls++;
    return 0;
}

long CdControl(long command, void *parameter, u8 *result) {
    (void)parameter;
    (void)result;
    s_cdCommand = (s32)command;
    return 1;
}

void RequestSelectBgmAssets(void) { s_assetRequests++; }
void SetDispMask(s32 enabled) { s_displayMask = enabled; }

void SetupDisplay240(s32 red, s32 green, s32 blue) {
    s_displayCalls++;
    s_displayRed = red;
    s_displayGreen = green;
    s_displayBlue = blue;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void ResetCalls(void) {
    s_cdSyncCalls = 0;
    s_cdCommand = -1;
    s_assetRequests = 0;
    s_displayCalls = 0;
    s_displayMask = -1;
}

static void TestClassFmvReturn(void) {
    ResetCalls();
    g_SceneId = -1;

    ReturnFromClassFmv();

    Check(s_cdSyncCalls == 1 && s_cdCommand == CD_DRIVE_PAUSE,
          "class FMV return pauses disc playback");
    Check(g_SceneId == 6 && s_assetRequests == 1,
          "class FMV return enters BGM select");
    Check(s_displayCalls == 0,
          "class FMV return preserves the current display mode");
}

static void TestEndingFmvReturn(void) {
    ResetCalls();
    g_FrameSyncThreshold = 0;
    g_FadeStep = 0;
    g_FadeLevel = 99;
    g_SceneId = -1;
    g_SceneTimer = 99;

    ReturnFromEndingFmv();

    Check(s_cdSyncCalls == 1 && s_cdCommand == CD_DRIVE_PAUSE,
          "ending FMV return pauses disc playback");
    Check(s_displayMask == 0 && s_displayCalls == 1 && s_displayRed == 0 &&
              s_displayGreen == 0 && s_displayBlue == 0,
          "ending FMV return restores a black 240-line display");
    Check(g_FrameSyncThreshold == 0x80 && g_FadeStep == 4 &&
              g_FadeLevel == 0 && g_SceneId == 0x22 && g_SceneTimer == 0,
          "ending FMV return initializes the ending scene");
    Check(s_assetRequests == 0,
          "ending FMV return does not request BGM select assets");
}

int main(void) {
    TestClassFmvReturn();
    TestEndingFmvReturn();

    if (s_failures != 0) {
        return 1;
    }
    puts("FMV return paths restore their destination scenes");
    return 0;
}
