#include <stdio.h>

#include "game/asset.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"
#include "game/state.h"

GameRenderState g_RenderState;
static GameFrameContext s_FrameContext;
GameFrameContext *g_DrawBuffer = &s_FrameContext;
s32 g_FrameSyncThreshold;
s32 g_SceneId;
s32 g_SceneTimer;
s32 g_LostRaceChoice;
s32 g_GrandPrixClass;
u16 g_PadPressed;
static CourseProgressState s_CourseProgress;
CourseProgressState *g_CourseProgress = &s_CourseProgress;

char g_CaptionLostRace[] = "LOST";
char g_TextTryAgain[] = "TRY";
char g_TextEndRace[] = "END";
char g_TextChance[] = "CHANCE";
char g_TextPressStart[] = "START";
char g_ChanceDigits[6][2] = {"0", "1", "2", "3", "4", "5"};

static s32 s_ReverbCalls;
static s32 s_LastSoundCue;
static s32 s_AssetRequests;
static s32 s_FadeLevel;
static s32 s_AudioFadeFrames;
static s32 s_ResetProgressCalls;
static s32 s_ResetProgressClass;
static s32 s_BannerDraws;

void SetReverbDepth(s32 left, s32 right) {
    if (left == 0x28 && right == 0x28) s_ReverbCalls++;
}
void PlaySoundCue(s32 cue) { s_LastSoundCue = cue; }
s32 RequestSelectBgmAssets(void) {
    s_AssetRequests++;
    return 1;
}
void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    if (tpage == 0x49) s_FadeLevel = color;
}
void StartCdVolumeFade(s32 frames) { s_AudioFadeFrames = frames; }
void ResetCourseProgress(s32 classIndex) {
    s_ResetProgressCalls++;
    s_ResetProgressClass = classIndex;
}
void GameDrawProportionalTextShaded(s32 x, s32 y, const char *str,
                                    s32 clutIndex, s32 intensity) {
    (void)x;
    (void)y;
    (void)str;
    (void)clutIndex;
    (void)intensity;
}
void DrawProportionalText(s32 x, s32 y, const char *str, s32 clutIndex) {
    (void)x;
    (void)y;
    (void)str;
    (void)clutIndex;
}
void DrawText8x8(s32 x, s32 y, const char *str, s32 clutIndex) {
    (void)x;
    (void)y;
    (void)str;
    (void)clutIndex;
}
void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1,
                u16 y1, u16 u0, u16 v0, u8 r, u8 g, u8 b, u16 clutX,
                s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x0;
    (void)y0;
    (void)x1;
    (void)y1;
    (void)u0;
    (void)v0;
    (void)r;
    (void)g;
    (void)b;
    (void)clutX;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
    s_BannerDraws++;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void ResetState(void) {
    g_FrameSyncThreshold = 0;
    g_SceneId = -1;
    g_SceneTimer = 0;
    g_LostRaceChoice = 0;
    g_GrandPrixClass = 2;
    g_PadPressed = 0;
    s_CourseProgress.retriesRemaining = 3;
    s_ReverbCalls = 0;
    s_LastSoundCue = -1;
    s_AssetRequests = 0;
    s_FadeLevel = -1;
    s_AudioFadeFrames = -1;
    s_ResetProgressCalls = 0;
    s_ResetProgressClass = -1;
    s_BannerDraws = 0;
}

static int TestLostRaceRetry(void) {
    ResetState();
    EnterLostRaceScreen();
    CHECK(g_FrameSyncThreshold == 0x80);
    CHECK(g_SceneId == 14 && g_SceneTimer == -1);
    CHECK(g_LostRaceChoice == 0 && s_ReverbCalls == 1);

    g_PadPressed = PAD_START;
    UpdateLostRaceScreen();
    CHECK(s_LastSoundCue == 2 && g_SceneTimer == 0);
    CHECK(s_CourseProgress.retriesRemaining == 2 && s_AssetRequests == 0);

    g_PadPressed = 0;
    g_SceneTimer = 254;
    UpdateLostRaceScreen();
    CHECK(g_SceneTimer == 256 && s_FadeLevel == 256);
    CHECK(g_SceneId == 11);
    return 0;
}

static int TestLostRaceExit(void) {
    ResetState();
    EnterLostRaceScreen();
    g_PadPressed = PAD_DOWN;
    UpdateLostRaceScreen();
    CHECK(g_LostRaceChoice == 1 && s_LastSoundCue == 1);

    g_PadPressed = PAD_START;
    UpdateLostRaceScreen();
    CHECK(s_AssetRequests == 1 && s_CourseProgress.retriesRemaining == 2);

    g_PadPressed = 0;
    g_SceneTimer = 254;
    UpdateLostRaceScreen();
    CHECK(g_SceneId == 6);
    return 0;
}

static int TestRaceEndScreen(void) {
    ResetState();
    EnterRaceEndScreen();
    CHECK(g_FrameSyncThreshold == 0x80);
    CHECK(g_SceneId == 16 && g_SceneTimer == 555 && s_BannerDraws == 1);

    g_PadPressed = PAD_CONFIRM;
    UpdateRaceEndScreen();
    CHECK(g_SceneTimer == 255 && s_AudioFadeFrames == 250);

    g_PadPressed = 0;
    g_SceneTimer = 1;
    UpdateRaceEndScreen();
    CHECK(g_SceneTimer == 0 && g_SceneId == 6 && s_AssetRequests == 1);
    CHECK(s_ResetProgressCalls == 1 && s_ResetProgressClass == 2);
    return 0;
}

int main(void) {
    if (TestLostRaceRetry() != 0) return 1;
    if (TestLostRaceExit() != 0) return 1;
    if (TestRaceEndScreen() != 0) return 1;
    puts("race end screen transitions passed");
    return 0;
}
