#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/audio_internal.h"
#include "game/menu.h"
#include "game/prim.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/round_screen_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"
#include "game/scene.h"

#include <stdio.h>
#include <string.h>

s32 g_AssetLoadFailed;
s32 g_AssetLoadState;
s32 g_BestLapTimes[2][4][2];
s32 g_BestTotalTimes[2][4][2];
s32 g_BgmSelection;
s32 g_BgmShuffleIndex;
u8 g_BgmShuffleOrder[BGM_SHUFFLE_CAPACITY];
s32 g_BgmTrack;
s32 g_BgmTrackCount;
char *g_BgmTrackNames[11];
char g_CaptionBestLapTime[] = "BEST LAP";
char g_CaptionBestTotalTime[] = "BEST TOTAL";
char g_CaptionPrizeMoney2[] = "PRIZE";
s32 g_CourseIndex;
CourseProgressState *g_CourseProgress;
s32 g_FadeLevel;
char g_FmtBgmNumber[] = "%d";
char g_FmtPrize1st[] = "%d";
char g_FmtPrize2nd[] = "%d";
char g_FmtPrize3rd[] = "%d";
char g_FmtRound[] = "%d";
s32 g_FrameSyncThreshold;
s32 g_GrandPrixClass;
s16 g_GrandPrixMode;
s32 g_GrandPrixRound;
s16 g_GrandPrixSeries;
u8 *g_ImageBlockBuffer;
size_t g_ImageBlockSize;
s32 g_MirrorMode;
u16 g_PadHeld;
u16 g_PadPressed;
RagePrizeMoneyStorage g_PrizeMoneyState;
GameRenderState g_RenderState;
s16 g_RoundScreenFadeDelays[ROUND_SCREEN_FADE_DELAY_STORAGE_COUNT];
s32 g_SceneId;
s32 g_SceneTimer;

static GameFrameContext s_frame;
GameFrameContext *g_DrawBuffer = &s_frame;

static s32 s_closeAudioCalls;
static s32 s_displayMask;
static s32 s_relocateResult;
static s32 s_requestAssetsResult;
static s32 s_setupDisplayCalls;
static s32 s_soundCue;
static s32 s_uploadResult;

void SetDispMask(int enabled) {
    s_displayMask = enabled;
}

void CloseLoadedAudioSlots(void) { s_closeAudioCalls++; }

s32 UploadImageAsset(const GameImageAssetHeaderWord *asset, size_t size) {
    (void)asset;
    (void)size;
    return s_uploadResult;
}

s32 RelocateCarModel(void) { return s_relocateResult; }
s32 RequestRaceAssets(void) { return s_requestAssetsResult; }

void SetupDisplay240(s32 red, s32 green, s32 blue) {
    (void)red;
    (void)green;
    (void)blue;
    s_setupDisplayCalls++;
}

void PlaySoundCue(s32 cue) { s_soundCue = cue; }

void DrawSprite(GameOrderingTableEntry *ot, s16 x, s16 y, s16 width,
                u16 height, u16 u, u16 v, u8 red, u8 green, u8 blue,
                u16 clut, s32 shadeTex, s32 semiTrans, u32 flags) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)red;
    (void)green;
    (void)blue;
    (void)clut;
    (void)shadeTex;
    (void)semiTrans;
    (void)flags;
}

void GameDrawProportionalTextShaded(s32 x, s32 y, const char *text,
                                    s32 clut, s32 intensity) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
    (void)intensity;
}

void FormatLapTime(char *text, s32 milliseconds) {
    snprintf(text, LAP_TIME_TEXT_CAPACITY, "%d", milliseconds);
}

void DrawText8x8(s32 x, s32 y, const char *text, s32 clut) {
    (void)x;
    (void)y;
    (void)text;
    (void)clut;
}

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                    s32 width, s32 height, s32 u, s32 v, s32 clut) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)u;
    (void)v;
    (void)clut;
    return packet + sizeof(SPRT);
}

u8 *QueueDrawModePrim(GameOrderingTableEntry *ot, u8 *packet, s32 tpage) {
    (void)ot;
    (void)tpage;
    return packet + sizeof(DR_MODE);
}

u8 *AddTilePrim(GameOrderingTableEntry *ot, u8 *packet, s32 x, s32 y,
                s32 width, s32 height, s32 red, s32 green, s32 blue) {
    (void)ot;
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)red;
    (void)green;
    (void)blue;
    return packet + sizeof(TILE);
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
    static CourseProgressState progress;
    static u8 image[4];
    static u8 packets[512];
    s32 index;

    memset(&progress, 0, sizeof(progress));
    memset(&s_frame, 0, sizeof(s_frame));
    memset(&g_RenderState, 0, sizeof(g_RenderState));
    memset(g_RoundScreenFadeDelays, 0, sizeof(g_RoundScreenFadeDelays));
    g_CourseProgress = &progress;
    g_ImageBlockBuffer = image;
    g_ImageBlockSize = sizeof(image);
    g_RenderState.packetCursor = packets;
    g_AssetLoadFailed = 0;
    g_AssetLoadState = 0;
    g_CourseIndex = 0;
    g_GrandPrixClass = 0;
    g_GrandPrixMode = 1;
    g_GrandPrixSeries = 0;
    g_BgmSelection = 0;
    g_BgmTrackCount = 2;
    g_BgmShuffleIndex = 0;
    g_BgmShuffleOrder[0] = 1;
    g_BgmShuffleOrder[1] = 0;
    for (index = 0; index < 11; index++) {
        g_BgmTrackNames[index] = "TRACK";
    }
    s_closeAudioCalls = 0;
    s_displayMask = -1;
    s_relocateResult = 1;
    s_requestAssetsResult = 1;
    s_setupDisplayCalls = 0;
    s_soundCue = -1;
    s_uploadResult = 1;
}

int main(void) {
    ResetState();
    g_AssetLoadState = 1;
    g_SceneId = GAME_SCENE_ENTER_ROUND;
    EnterRoundScreen();
    CHECK(s_displayMask == 0 && g_FrameSyncThreshold == 0x80);
    CHECK(s_closeAudioCalls == 0 && g_SceneId == GAME_SCENE_ENTER_ROUND);

    ResetState();
    s_uploadResult = 0;
    g_SceneId = GAME_SCENE_ENTER_ROUND;
    EnterRoundScreen();
    CHECK(s_closeAudioCalls == 1 && g_SceneId == GAME_SCENE_ENTER_ROUND);

    ResetState();
    g_SceneId = GAME_SCENE_ENTER_ROUND;
    EnterRoundScreen();
    CHECK(g_SceneId == GAME_SCENE_ROUND && g_SceneTimer == 0);
    CHECK(g_FrameSyncThreshold == 0x180 && g_GrandPrixRound == 1);

    g_SceneTimer = 0;
    g_FadeLevel = 0;
    s_requestAssetsResult = 0;
    UpdateRoundScreen();
    CHECK(g_SceneTimer == 1 && s_setupDisplayCalls == 1);
    CHECK(g_FadeLevel == 0x80 && g_SceneId == GAME_SCENE_ROUND);

    g_SceneTimer = 120;
    g_FadeLevel = 1;
    g_PadHeld = PAD_START | PAD_R1 | PAD_L1;
    UpdateRoundScreen();
    CHECK(g_SceneId == GAME_SCENE_ENTER_RACE && g_MirrorMode == 1);
    CHECK(g_BgmTrack == 1 && g_BgmShuffleIndex == 1);

    ResetState();
    g_SceneId = GAME_SCENE_ROUND;
    g_SceneTimer = 31;
    g_FadeLevel = 1;
    UpdateRoundScreen();
    CHECK(s_soundCue == 0x19);

    puts("round screen gates assets and advances into the selected race");
    return 0;
}
