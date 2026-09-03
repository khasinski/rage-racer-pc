#include <stdio.h>
#include "game/prim.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/round_screen_internal.h"
#include "game/save_internal.h"
#include "game/screens.h"

enum {
    ROUND_SCREEN_SCENE = 10,
    ROUND_SCREEN_RACE_SCENE = 11,
    ROUND_SCREEN_SETUP_FRAME = 1,
    ROUND_SCREEN_DISPLAY_FRAME = 15,
    ROUND_SCREEN_CUE_FRAME = 32,
    ROUND_SCREEN_CUE = 0x19,
    ROUND_SCREEN_RACE_FRAME = 121,
    ROUND_SCREEN_FADE_READY = 0x80,
    ROUND_SCREEN_TEXT_CAPACITY = 88,
};

/* Scene 9: finishes the asset load, relocates the car model and derives g_GrandPrixRound. */
void EnterRoundScreen(void) {
    SetDispMask(0);
    g_FrameSyncThreshold = 0x80;

    if (!IsRoundScreenAssetLoadComplete(g_AssetLoadState,
                                        g_AssetLoadFailed)) {
        return;
    }
    CloseLoadedAudioSlots();
    if (!UploadImageAsset(GetImageAssetHeaderWords(g_ImageBlockBuffer),
                          g_ImageBlockSize)) {
        return;
    }
    if (!RelocateCarModel()) {
        return;
    }

    g_FrameSyncThreshold = 0x180;
    g_SceneTimer = 0;
    g_SceneId = ROUND_SCREEN_SCENE;
    g_FadeLevel = 0;
    g_GrandPrixRound = DetermineGrandPrixRound(
        g_CourseProgress != NULL ? g_CourseProgress->bestPlace : NULL,
        g_GrandPrixClass,
        SeriesCourseIndex());
}

static s32 NextRoundScreenFade(s32 stage) {
    s32 value;

    if (g_SceneId == ROUND_SCREEN_SCENE) {
        return RoundScreenFadeFromTimer(
            g_SceneTimer, g_RoundScreenFadeDelays[stage]);
    }
    if (g_FadeLevel > 0) {
        g_FadeLevel--;
    }
    value = g_FadeLevel;
    return ClampRoundScreenFade(value);
}


/* The ROUND screen: course name, round number and either the prize lines or the best times. */
void DrawRoundScreen(void) {
    char text[ROUND_SCREEN_TEXT_CAPACITY];
    s32 col;
    s32 y0;
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);

    col = NextRoundScreenFade(0);
    DrawSprite(ot, 0x74, 0x14, 0x58, 0x38, 0xa8, 0xa8, col, col, col,
               0x1f, 0, 1, 0x29);
    DrawSprite(ot, 0x44, 0x50, 0xb8, 0x14, 0x48, 0xe8, col, col, col,
               0x80, 0, 1, 0x29);

    col = NextRoundScreenFade(1);
    if (g_GrandPrixMode != 0) {
        snprintf(text, sizeof(text), g_FmtRound, g_GrandPrixRound);
        GameDrawProportionalTextShaded(0x5e, 0x68, text, 0x7812, col);
        y0 = 0x78;
    } else {
        y0 = 0x68;
    }
    DrawSprite(ot, 0x5e, y0, 0x84, 0xc, 0,
               g_CourseIndex * 12 + 156, col, col, col, 0x12, 0, 1, 0x29);

    col = NextRoundScreenFade(2);
    if (g_GrandPrixMode != 0) {
        GameDrawProportionalTextShaded(0x80, 0x88, g_CaptionPrizeMoney2,
                                      0x7812, col);
        const s32 *prizes;

        if (!RoundScreenTableIndicesValid(g_GrandPrixSeries,
                                          g_GrandPrixClass,
                                          g_GrandPrixMode)) {
            return;
        }
        prizes = g_PrizeMoney.values[SeriesCourseIndex()][g_GrandPrixClass];

        snprintf(text, sizeof(text), g_FmtPrize1st,
                 prizes[PRIZE_PLACE_FIRST]);
        GameDrawProportionalTextShaded(0x56, 0x98, text, 0x7812, col);
        snprintf(text, sizeof(text), g_FmtPrize2nd,
                 prizes[PRIZE_PLACE_SECOND]);
        GameDrawProportionalTextShaded(0x56, 0xa4, text, 0x7812, col);
        snprintf(text, sizeof(text), g_FmtPrize3rd,
                 prizes[PRIZE_PLACE_THIRD]);
        GameDrawProportionalTextShaded(0x56, 0xb0, text, 0x7812, col);
    } else {
        s32 course = SeriesCourseIndex();
        s32 recordMode = RaceRecordMode(g_GrandPrixMode);

        GameDrawProportionalTextShaded(0x62, 0x7c, g_CaptionBestTotalTime,
                                      0x7812, col);
        if (!RoundScreenTableIndicesValid(g_GrandPrixSeries,
                                          g_GrandPrixClass,
                                          g_GrandPrixMode)) {
            return;
        }
        FormatLapTime(
            text,
            g_BestTotalTimes[g_GrandPrixSeries][course][recordMode]);
        GameDrawProportionalTextShaded(0x6a, 0x8c, text, 0x7812, col);
        GameDrawProportionalTextShaded(0x6a, 0x9c, g_CaptionBestLapTime,
                                      0x7812, col);
        FormatLapTime(
            text, g_BestLapTimes[g_GrandPrixSeries][course][recordMode]);
        GameDrawProportionalTextShaded(0x6a, 0xac, text, 0x7812, col);
    }
}


/* The BGM row: the selection number and the track title from g_BgmTrackNames. */
static void DrawBgmSelector(void) {
    s32 x;
    char text[ROUND_SCREEN_TEXT_CAPACITY];
    u8 *p;
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(1);

    g_BgmSelection = WrapRoundBgmSelection(
        g_BgmSelection, ClampRoundBgmTrackCount(g_BgmTrackCount));
    p = RENDER_PRIM_CURSOR_AS(u8);
    p = GameQueueSprite(ot, p, 0x14, 0xce, 0x58, 8, 0xa8, 0xe0, 0x7812);
    x = (g_BgmSelection == 0xa) ? 0x6c : 0x70;
    p = GameQueueSprite(ot, p, x, 0xce, 8, 8, 0x84, 0xc4, 0x7812);
    p = GameQueueSprite(ot, p, (g_BgmSelection == 0xa) ? 0x84 : 0x80,
                        0xce, 8, 8, 0x8c, 0xc4, 0x7812);
    p = QueueDrawModePrim(ot, p, 0x29);
    p = AddTilePrim(ot, p, 0x10, 0xcc, 0x5b, 0xc, 0x85, 0x15, 0xe);
    p = AddTilePrim(ot, p, 0x6c, 0xcc, 0x1f, 0xc, 0x40, 0x40, 0x40);
    p = AddTilePrim(ot, p, 0x8c, 0xcc, 0xa4, 0xc, 0, 0, 0);
    p = AddTilePrim(ot, p, 0xf, 0xcb, 0x122, 0xe, 0xff, 0xff, 0xff);
    g_RenderState.packetCursor = p;

    snprintf(text, sizeof(text), g_FmtBgmNumber, g_BgmSelection);
    x = (g_BgmSelection == 0xa) ? 0x74 : 0x78;
    DrawText8x8(x, 0xce, text, 0x78cc);
    DrawText8x8(0x90, 0xce, g_BgmTrackNames[g_BgmSelection], 0x78cc);
}


/* Scene 10: draws the ROUND screen, takes the BGM choice and starts the race at frame 121. */
void UpdateRoundScreen(void) {
    RoundBgmChoice bgm;
    const s32 trackCount = ClampRoundBgmTrackCount(g_BgmTrackCount);

    g_SceneTimer = NextRoundScreenTimer(g_SceneTimer);
    if (g_SceneTimer == ROUND_SCREEN_DISPLAY_FRAME) {
        SetDispMask(1);
    }
    if (g_SceneTimer == ROUND_SCREEN_SETUP_FRAME) {
        SetupDisplay240(0, 0, 0);
    }
    DrawRoundScreen();
    if (g_SceneTimer == ROUND_SCREEN_CUE_FRAME) {
        PlaySoundCue(ROUND_SCREEN_CUE);
    }
    if (g_FadeLevel == 0) {
        if (RequestRaceAssets() == 0) {
            g_FadeLevel = ROUND_SCREEN_FADE_READY;
        }
    } else if ((u32)g_SceneTimer >= ROUND_SCREEN_RACE_FRAME) {
        g_SceneId = ROUND_SCREEN_RACE_SCENE;
        g_MirrorMode = IsRoundMirrorMode(g_PadHeld);
        bgm = ChooseRoundBgm(g_BgmSelection, g_BgmShuffleOrder,
                             trackCount, g_BgmShuffleIndex);
        g_BgmTrack = bgm.track;
        g_BgmShuffleIndex = bgm.shuffleIndex;
    }
    if (g_SceneId == ROUND_SCREEN_SCENE) {
        u16 flags = g_PadPressed;
        if (flags & PAD_LEFT) {
            g_BgmSelection--;
        } else if (flags & PAD_RIGHT) {
            g_BgmSelection++;
        }
        g_BgmSelection =
            WrapRoundBgmSelection(g_BgmSelection, trackCount);
        DrawBgmSelector();
    }
}
