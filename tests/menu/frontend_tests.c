#include "common.h"
#include "game/asset.h"
#include "game/fmv.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/render_state.h"
#include "game/screens.h"
#include "game/state.h"

#include <stdio.h>

static Frontend s_frontend;
#include <string.h>

s32 g_AnimTimer;
s32 g_AssetLoadState;
static s32 s_assetLoadFailed;
ScoreRecord g_ClassRecords[CLASS_RECORD_COUNT];
s32 g_CourseIndex;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
GameRaceProgress *g_RaceProgress;
s32 g_SceneId;
s32 g_SceneTimer;
void (*g_FrontendDrawHandlers[FRONTEND_STATE_COUNT])(void);
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

s32 AssetLoadCompletedSuccessfully(void) {
    return g_AssetLoadState == 0 && !s_assetLoadFailed;
}

static void DrawHandler(void) { s_drawHandlerCalls++; }

s32 Random15(void) {
    s32 index = s_randomIndex++;
    return s_randomValues[index < 4 ? index : 3];
}

s32 RandomIndex(s32 count) {
    return count > 0 ? (Random15() & 0xFFF) % count : 0;
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
s32 RequestCourseTextureAssets(void) {
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
    memset(g_ClassRecords, 0, sizeof(g_ClassRecords));
    memset(s_packets, 0, sizeof(s_packets));
    g_DrawBuffer = &s_frame;
    g_RenderState.draw.packetCursor = s_packets;
    g_RaceProgress = &s_progress;
    s_frontend.state = FRONTEND_STATE_TITLE;
    g_FrontendDrawHandlers[FRONTEND_STATE_TITLE] = DrawHandler;
    g_SceneTimer = 0;
    s_frontend.idleTimer = 0;
    s_frontend.attractCycle = 0;
    s_frontend.attractTimer = 1;
    s_frontend.exitTimer = 0;
    s_frontend.menuSlide = 0;
    g_AssetLoadState = 0;
    s_assetLoadFailed = 0;
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
    for (s32 cls = 0; cls < 5; ++cls) {
        for (s32 course = 0; course < 4; ++course) {
            Reset();
            g_SceneTimer = 0x1CC;
            s_randomValues[1] = cls;
            s_randomValues[2] = course;
            s_randomValues[3] = 2;
            UpdateFrontend();
            s32 reroll = cls < 2 && course == 3;
            CHECK(g_GrandPrixClass == cls);
            CHECK(g_CourseIndex == (reroll ? 2 : course));
            CHECK(s_randomIndex == (reroll ? 4 : 3));
            CHECK(s_trackRequests == 1 && g_SceneTimer == 0x1CD);
        }
    }
    Reset();
    s_frontend.pulse = 0x80;
    s_frontend.selection = 0;
    s_progress.maxClassReached = -1;
    UpdateMainMenuExit();
    CHECK(s_frontend.pulse == 0x81 && s_fadeColor == 0x102);
    CHECK(s_progress.maxClassReached == 0 && g_GrandPrixMode == 1);
    CHECK(g_GrandPrixSeries == 0 && g_SceneId == 0x1F);

    Reset();
    s_frontend.pulse = 0x80;
    s_frontend.selection = TITLE_MENU_CUSTOM;
    g_GrandPrixMode = 1;
    UpdateMainMenuExit();
    CHECK(s_frontend.pulse == 0x81);
    CHECK(g_GrandPrixMode == 0 && g_SceneId == GAME_SCENE_INIT_MENU);

    Reset();
    s_frontend.exitTimer = 1;
    UpdateFrontend();
    CHECK(g_SceneTimer == 1 && s_setupCalls == 1);
    CHECK(s_frontend.exitTimer == 0 && s_lastCue == 0x1A);

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
    s_assetLoadFailed = 1;
    UpdateFrontend();
    CHECK(g_SceneTimer == 0x1CE);
    s_assetLoadFailed = 0;
    UpdateFrontend();
    CHECK(g_SceneTimer == 0x1CF);

    Reset();
    g_SceneTimer = 0x1CF;
    s_frontend.idleTimer = 900;
    s_frontend.attractCycle = 0;
    UpdateFrontend();
    CHECK(g_SceneId == 0x1D && g_GrandPrixMode == 1);
    CHECK(s_frontend.attractCycle == 1);

    Reset();
    g_SceneTimer = 0x1CF;
    s_frontend.idleTimer = 900;
    s_frontend.attractCycle = 1;
    UpdateFrontend();
    CHECK(s_fmvCalls == 1 && s_frontend.attractCycle == 2);

    Reset();
    s_frontend.menuSlide = 100;
    UpdateTitleAttract();
    CHECK(s_lastAlpha == 0x30);
    s_frontend.menuSlide = -10;
    for (s32 i = 0; i < CLASS_RECORD_COUNT; i++) {
        g_ClassRecords[i].place = 1;
    }
    UpdateTitleAttract();
    CHECK(s_lastAlpha == 0x7F && s_lastPanelClut == 0x7D80);

    Reset();
    s_frontend.state = FRONTEND_STATE_INVALID;
    UpdateFrontend();
    CHECK(s_frontend.state == FRONTEND_STATE_TITLE &&
          s_drawHandlerCalls == 1);

    Reset();
    s_frontend.state = FRONTEND_STATE_COUNT;
    UpdateFrontend();
    CHECK(s_frontend.state == FRONTEND_STATE_TITLE &&
          s_drawHandlerCalls == 1);

    puts("frontend transitions, attract loading and overlays are preserved");
    return 0;
}

Frontend *MenuFrontend(void) { return &s_frontend; }
