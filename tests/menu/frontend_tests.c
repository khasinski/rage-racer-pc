#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/render_state.h"
#include "game/screens.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 g_AnimTimer;
s32 g_AssetLoadState;
s32 g_AttractCycleCount;
s32 g_ClassWinCount;
s32 g_CourseIndex;
FrontendState g_FrontendState;
u32 g_FrontendIdleTimer;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
s32 g_MainMenuSlide;
GameRaceProgress *g_RaceProgress;
s32 g_SceneId;
s32 g_SceneTimer;
s32 g_TitleAttractTimer;
s32 g_TitleExitTimer;
s32 g_TitleMenuSelection;
s32 g_TitlePulse;
void (*g_FrontendDrawHandlers[4])(void);
GameFrameContext *g_DrawBuffer;
GameRenderState g_RenderState;

static GameFrameContext s_frame;
static GameRaceProgress s_progress;
static u8 s_packets[64];
static s32 s_drawHandlerCalls;
static s32 s_fadeColor;
static s32 s_fmvCalls;
static s32 s_lastCue;
static s32 s_raceRequests;
static s32 s_setupCalls;
static s32 s_trackRequests;
static s32 s_spriteCalls;
static s32 s_lastAlpha;
static s32 s_lastPanelClut;
static s32 s_randomValues[4];
static s32 s_randomIndex;

static void DrawHandler(void) { s_drawHandlerCalls++; }

s32 Random15(void) {
    s32 index = s_randomIndex++;
    return s_randomValues[index < 4 ? index : 3];
}

void DrawFullscreenFadeTile(s32 color, s32 tpage) {
    (void)tpage;
    s_fadeColor = color;
}
void DrawMainMenuRows(void) { s_drawHandlerCalls++; }
void PlaySoundCue(s32 cue) { s_lastCue = cue; }
void BeginIntroFmv(s32 scene) {
    (void)scene;
    s_fmvCalls++;
}
s32 RequestTrackLoad(void) {
    s_trackRequests++;
    return 0;
}
s32 RequestRaceStart(void) {
    s_raceRequests++;
    return 0;
}
void SetupDisplay240(s32 r, s32 g, s32 b) {
    (void)r;
    (void)g;
    (void)b;
    s_setupCalls++;
}
s32 CdControl(u_char command, u_char *parameter, u_char *result) {
    (void)command;
    (void)parameter;
    (void)result;
    return 0;
}
void SetDispMask(s32 enabled) {
    (void)enabled;
}

u8 *GameQueueShadedSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                          s32 height, s32 u, s32 v, s32 clut, s32 shade) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    s_spriteCalls++;
    s_lastPanelClut = clut;
    s_lastAlpha = shade;
    return prim + 1;
}

u8 *GameQueueShadedTexturedRect(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 width,
                                s32 height, s32 u, s32 v, s32 clut,
                                s32 tpage, s32 shade) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)tpage;
    s_spriteCalls++;
    s_lastPanelClut = clut;
    s_lastAlpha = shade;
    return prim + 1;
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *prim, s32 tpage) {
    (void)ot;
    (void)tpage;
    return prim + 1;
}

#define CHECK(condition)                                                       \
    do {                                                                       \
        if (!(condition)) {                                                    \
            fprintf(stderr, "check failed at line %d: %s\n", __LINE__,         \
                    #condition);                                               \
            return 1;                                                          \
        }                                                                      \
    } while (0)

static void Reset(void) {
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&s_progress, 0, sizeof(s_progress));
    memset(s_packets, 0, sizeof(s_packets));
    g_DrawBuffer = &s_frame;
    RENDER_PRIM_CURSOR_AS(u8) = s_packets;
    g_RaceProgress = &s_progress;
    g_FrontendState = FRONTEND_STATE_TITLE;
    g_FrontendDrawHandlers[FRONTEND_STATE_TITLE] = DrawHandler;
    g_SceneTimer = 0;
    g_FrontendIdleTimer = 0;
    g_AttractCycleCount = 0;
    g_TitleAttractTimer = 1;
    g_TitleExitTimer = 0;
    g_MainMenuSlide = 0;
    g_ClassWinCount = 0;
    g_AssetLoadState = 0;
    s_drawHandlerCalls = 0;
    s_fmvCalls = 0;
    s_lastCue = 0;
    s_raceRequests = 0;
    s_setupCalls = 0;
    s_trackRequests = 0;
    s_spriteCalls = 0;
    s_randomIndex = 0;
    s_randomValues[0] = 0;
    s_randomValues[1] = 3;
    s_randomValues[2] = 3;
    s_randomValues[3] = 1;
}

int main(void) {
    Reset();
    g_TitlePulse = 0x80;
    g_TitleMenuSelection = 0;
    s_progress.maxClassReached = -1;
    UpdateMainMenuExit();
    CHECK(g_TitlePulse == 0x81 && s_fadeColor == 0x102);
    CHECK(s_progress.maxClassReached == 0 && g_GrandPrixMode == 1);
    CHECK(g_GrandPrixSeries == 0 && g_SceneId == 0x1F);

    Reset();
    g_TitleExitTimer = 1;
    UpdateFrontend();
    CHECK(g_SceneTimer == 1 && s_setupCalls == 1);
    CHECK(g_TitleExitTimer == 0 && s_lastCue == 0x1A);

    Reset();
    g_SceneTimer = 0x1CC;
    UpdateFrontend();
    CHECK(g_SceneTimer == 0x1CD && s_trackRequests == 1);
    CHECK(g_GrandPrixClass == 3 && g_CourseIndex == 3);
    CHECK(s_drawHandlerCalls == 1 && s_spriteCalls == 5);

    g_AssetLoadState = 1;
    UpdateFrontend();
    CHECK(g_SceneTimer == 0x1CD && s_raceRequests == 0);
    g_AssetLoadState = 0;
    UpdateFrontend();
    CHECK(g_SceneTimer == 0x1CE && s_raceRequests == 1);
    UpdateFrontend();
    CHECK(g_SceneTimer == 0x1CF);

    Reset();
    g_SceneTimer = 0x1CF;
    g_FrontendIdleTimer = 900;
    g_AttractCycleCount = 0;
    UpdateFrontend();
    CHECK(g_SceneId == 0x1D && g_GrandPrixMode == 1);
    CHECK(g_AttractCycleCount == 1);

    Reset();
    g_SceneTimer = 0x1CF;
    g_FrontendIdleTimer = 900;
    g_AttractCycleCount = 1;
    UpdateFrontend();
    CHECK(s_fmvCalls == 1 && g_AttractCycleCount == 2);

    Reset();
    g_MainMenuSlide = 100;
    UpdateTitleAttract();
    CHECK(s_lastAlpha == 0x30);
    g_MainMenuSlide = -10;
    g_ClassWinCount = 11;
    UpdateTitleAttract();
    CHECK(s_lastAlpha == 0x7F && s_lastPanelClut == 0x7D80);

    puts("frontend transitions, attract loading and overlays are preserved");
    return 0;
}
