#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"

#include <stdio.h>

s32 g_AssetLoadState;
s32 g_BgmChangeDelay;
s32 g_BgmSelectCdTrack;
s32 g_BgmSelectCursor;
s32 g_BgmSelectShowUi;
BgmSelectStep g_BgmSelectStep;
s32 g_BgmSelectTrack;
s32 g_CameraCarIndex;
s32 g_CdTrackEnded;
s32 g_FadeLevel;
s32 g_FadeStep;
s32 g_FrameSyncThreshold;
s32 g_SceneId;
s32 g_SceneTimer;
char g_TextNowLoading[] = "NOW LOADING";

static s32 s_courseInstalls;
static s32 s_dataRequests;
static s32 s_displayMask;
static s32 s_displaySetups;
static s32 s_fadeCalls;
static s32 s_lastFade;
static s32 s_textCalls;
static s32 s_trackInits;

void SetDispMask(s32 enabled) { s_displayMask = enabled; }
void SetupDisplay240(s32 r, s32 g, s32 b) {
    (void)r;
    (void)g;
    (void)b;
    s_displaySetups++;
}
void InstallCourseAssets(void) { s_courseInstalls++; }
s32 RequestTrackDataAssets(void) {
    s_dataRequests++;
    return 1;
}
void InitTrackScene(void) { s_trackInits++; }
void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    (void)tpage;
    s_lastFade = color;
    s_fadeCalls++;
}
void DrawProportionalText(s32 x, s32 y, const char *str, s32 clutIndex) {
    (void)x;
    (void)y;
    (void)str;
    (void)clutIndex;
    s_textCalls++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetCalls(void) {
    s_courseInstalls = 0;
    s_dataRequests = 0;
    s_displayMask = -1;
    s_displaySetups = 0;
    s_fadeCalls = 0;
    s_lastFade = -1;
    s_textCalls = 0;
    s_trackInits = 0;
}

int main(void) {
    ResetCalls();
    EnterBgmSelectScreen();
    CHECK(s_displayMask == 0 && s_displaySetups == 1);
    CHECK(g_FrameSyncThreshold == 0x80 && g_FadeLevel == 0x13C);
    CHECK(g_FadeStep == -4 && g_SceneId == 0x1C);
    CHECK(g_BgmSelectStep == BGM_SELECT_STEP_LOAD_ASSETS);
    CHECK(g_BgmSelectCursor == 1 && g_BgmSelectShowUi == 1);
    CHECK(g_BgmSelectCdTrack == 3 && g_BgmSelectTrack == 0);
    CHECK(g_BgmChangeDelay == 0x1E && g_CameraCarIndex == 0);

    ResetCalls();
    g_AssetLoadState = 1;
    g_FadeLevel = 2;
    g_FadeStep = -4;
    UpdateBgmSelectLoad();
    CHECK(s_courseInstalls == 0 && s_dataRequests == 0);
    CHECK(g_FadeLevel == 0 && g_FadeStep == 0);
    CHECK(s_fadeCalls == 1 && s_lastFade == 0 && s_textCalls == 1);

    ResetCalls();
    g_AssetLoadState = 0;
    g_FadeLevel = 0;
    g_FadeStep = 0;
    UpdateBgmSelectLoad();
    CHECK(s_courseInstalls == 1 && s_dataRequests == 1);
    CHECK(g_BgmSelectStep == BGM_SELECT_STEP_FADE_IN);

    ResetCalls();
    g_AssetLoadState = 0;
    g_FadeLevel = 254;
    g_FadeStep = 0;
    UpdateBgmSelectFadeIn();
    CHECK(s_fadeCalls == 1 && s_lastFade == 258);
    CHECK(s_displayMask == 0 && s_trackInits == 1);
    CHECK(g_FadeLevel == 0 && g_FadeStep == 0);
    CHECK(g_BgmSelectStep == BGM_SELECT_STEP_ACTIVE);

    ResetCalls();
    g_AssetLoadState = 1;
    g_SceneTimer = 0xF;
    g_FadeStep = 0;
    UpdateBgmSelectFadeIn();
    CHECK(s_displayMask == 1 && s_fadeCalls == 0 && s_textCalls == 1);

    ResetCalls();
    g_AssetLoadState = 1;
    g_FadeLevel = 2;
    g_FadeStep = -4;
    g_SceneId = 0x1C;
    ExitBgmSelect();
    CHECK(g_FadeLevel == 0 && g_FadeStep == 0);
    CHECK(g_SceneId == 0x1C);
    CHECK(s_fadeCalls == 1 && s_lastFade == 0 && s_textCalls == 1);

    ResetCalls();
    g_AssetLoadState = 0;
    g_FadeLevel = 254;
    g_FadeStep = 0;
    ExitBgmSelect();
    CHECK(g_FadeLevel == 258 && g_FadeStep == 4);
    CHECK(g_SceneId == 0x16);
    CHECK(s_fadeCalls == 1 && s_lastFade == 258 && s_textCalls == 1);

    puts("BGM select transition tests passed");
    return 0;
}
