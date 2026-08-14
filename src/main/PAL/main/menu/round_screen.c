#include "common.h"
#include <stdio.h>
#include "game/prim.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/save_internal.h"
#include "game/scratchpad.h"
#include "game/screens.h"
#include "game/state.h"
#include "psyq/gpu.h"
#include "psyq/gte.h"

/* Darkens the scene colour matrix by GetTrackZoneBlend's 0..0x100 track-zone ramp; RestoreColorMatrix puts it back. */
void ApplyZoneLighting(s32 a0, Matrix *mtx) {
    Matrix out;
    s32 s1;

    if (g_TrackZoneCode != 0) {
        s1 = 0x100 - (a0 * 3) / 4;
        out.m[0][0] = g_SceneColorMatrix.m[0][0] * s1 / 256;
        out.m[0][1] = g_SceneColorMatrix.m[0][1] * s1 / 256;
        out.m[0][2] = g_SceneColorMatrix.m[0][2] * s1 / 256;
        out.m[1][0] = g_SceneColorMatrix.m[1][0] * s1 / 256;
        out.m[1][1] = g_SceneColorMatrix.m[1][1] * s1 / 256;
        out.m[1][2] = g_SceneColorMatrix.m[1][2] * s1 / 256;
        out.m[2][0] = g_SceneColorMatrix.m[2][0] * s1 / 256;
        out.m[2][1] = g_SceneColorMatrix.m[2][1] * s1 / 256;
        out.m[2][2] = g_SceneColorMatrix.m[2][2] * s1 / 256;
        SetColorMatrix(&out);
    } else {
        s32 k;
        s32 h;
        s32 kb;
        out.m[0][0] = g_SceneColorMatrix.m[0][0];
        out.m[0][1] = g_SceneColorMatrix.m[0][1];
        out.m[0][2] = g_SceneColorMatrix.m[0][2];
        h = a0 / 2;
        k = 0x100;
        s1 = k - h;
        out.m[1][0] = g_SceneColorMatrix.m[1][0] * s1 / 256;
        out.m[1][1] = g_SceneColorMatrix.m[1][1] * s1 / 256;
        out.m[1][2] = g_SceneColorMatrix.m[1][2] * s1 / 256;
        s1 = k - (a0 * 3) / 4;
        out.m[2][0] = g_SceneColorMatrix.m[2][0] * s1 / 256;
        out.m[2][1] = g_SceneColorMatrix.m[2][1] * s1 / 256;
        out.m[2][2] = g_SceneColorMatrix.m[2][2] * s1 / 256;
        SetColorMatrix(&out);

        kb = k - a0;
        mtx->m[0][0] = mtx->m[0][0] * kb / 256 + g_TrackLightMatrix.m[0][0] * a0 / 256;
        mtx->m[0][1] = mtx->m[0][1] * kb / 256 + g_TrackLightMatrix.m[0][1] * a0 / 256;
        mtx->m[0][2] = mtx->m[0][2] * kb / 256 + g_TrackLightMatrix.m[0][2] * a0 / 256;
        mtx->m[1][0] = mtx->m[1][0] * kb / 256 + g_TrackLightMatrix.m[1][0] * a0 / 256;
        mtx->m[1][1] = mtx->m[1][1] * kb / 256 + g_TrackLightMatrix.m[1][1] * a0 / 256;
        mtx->m[1][2] = mtx->m[1][2] * kb / 256 + g_TrackLightMatrix.m[1][2] * a0 / 256;
        mtx->m[2][0] = mtx->m[2][0] * kb / 256 + g_TrackLightMatrix.m[2][0] * a0 / 256;
        mtx->m[2][1] = mtx->m[2][1] * kb / 256 + g_TrackLightMatrix.m[2][1] * a0 / 256;
        mtx->m[2][2] = mtx->m[2][2] * kb / 256 + g_TrackLightMatrix.m[2][2] * a0 / 256;
    }
}

void RestoreColorMatrix(void) { SetColorMatrix(&g_SceneColorMatrix); }


/* Scene 9: finishes the asset load, relocates the car model and derives g_GrandPrixRound. */
void EnterRoundScreen(void) {
    s32 count;
    CourseProgressByteAddress ptr;
    CourseProgressByteAddress end;

    SetDispMask(0);
    g_FrameSyncThreshold = 0x80;

    if (g_AssetLoadState != 1) {
        CloseLoadedAudioSlots();
        UploadImageAsset(g_ImageBlockBuffer);
        RelocateCarModel();

        g_FrameSyncThreshold = 0x180;
        g_SceneTimer = 0;
        g_SceneId = 10;
        g_FadeLevel = 0;
        count = (g_GrandPrixClass < 2) ? 3 : 4;
        g_GrandPrixRound = 0;

        if (count != 0) {
            ptr.pointer = g_CourseProgress->bestPlace;
            end.value = count + ptr.value;
            do {
                if (*ptr.pointer != 0) {
                    g_GrandPrixRound++;
                }
                ptr.pointer++;
            } while (ptr.value < end.value);
        }

        if (g_CourseProgress->bestPlace[g_CourseIndex] == 0) {
            g_GrandPrixRound++;
        }
    }
}

s32 UpdateRoundScreenFade(s32 stage) {
    s32 value;
    s32 ret;

    if (g_SceneId == 10) {
        value = (g_SceneTimer * 4) - g_RoundScreenFadeDelays[stage];
    } else {
        value = g_FadeLevel;
        if (value > 0) {
            value--;
            g_FadeLevel = value;
        }
        value = g_FadeLevel;
    }

    if (value >= 0) {
        ret = value;
        if (ret < 0x80) {
            return ret;
        }
        ret = 0x7F;
    } else {
        ret = 0;
    }
    return ret;
}


/* The ROUND screen: course name, round number and either the prize lines or the best times. */
void DrawRoundScreen(void) {
    char buf[88];
    s32 col;
    s32 y0;
    void *ot = GamePrimaryOrderingTable(0);

    col = UpdateRoundScreenFade(0);
    DrawSprite(ot, 0x74, 0x14, 0x58, 0x38, 0xa8, 0xa8, col, col, col, 0x1f, 0, 1, 0x29);
    DrawSprite(ot, 0x44, 0x50, 0xb8, 0x14, 0x48, 0xe8, col, col, col, 0x80, 0, 1, 0x29);

    col = UpdateRoundScreenFade(1);
    if (g_GrandPrixMode != 0) {
        sprintf(buf, g_FmtRound, g_GrandPrixRound);
        GameDrawProportionalTextShaded(0x5e, 0x68, buf, 0x7812, col);
        y0 = 0x78;
    } else {
        y0 = 0x68;
    }
    DrawSprite(ot, 0x5e, y0, 0x84, 0xc, 0, g_CourseIndex * 12 + 156, col, col, col, 0x12, 0, 1, 0x29);

    col = UpdateRoundScreenFade(2);
    if (g_GrandPrixMode != 0) {
        GameDrawProportionalTextShaded(0x80, 0x88, g_CaptionPrizeMoney2, 0x7812, col);
        sprintf(buf, g_FmtPrize1st, g_PrizeMoney.values[g_CourseIndex][g_GrandPrixClass][0]);
        GameDrawProportionalTextShaded(0x56, 0x98, buf, 0x7812, col);
        sprintf(buf, g_FmtPrize2nd, g_PrizeMoney.values[g_CourseIndex][g_GrandPrixClass][1]);
        GameDrawProportionalTextShaded(0x56, 0xa4, buf, 0x7812, col);
        sprintf(buf, g_FmtPrize3rd, g_PrizeMoney.values[g_CourseIndex][g_GrandPrixClass][2]);
        GameDrawProportionalTextShaded(0x56, 0xb0, buf, 0x7812, col);
    } else {
        GameDrawProportionalTextShaded(0x62, 0x7c, g_CaptionBestTotalTime, 0x7812, col);
        FormatLapTime(buf, g_BestTotalTimes[g_GrandPrixSeries][g_CourseIndex][g_GrandPrixMode]);
        GameDrawProportionalTextShaded(0x6a, 0x8c, buf, 0x7812, col);
        GameDrawProportionalTextShaded(0x6a, 0x9c, g_CaptionBestLapTime, 0x7812, col);
        FormatLapTime(buf, g_BestLapTimes[g_GrandPrixSeries][g_CourseIndex][g_GrandPrixMode]);
        GameDrawProportionalTextShaded(0x6a, 0xac, buf, 0x7812, col);
    }
}


/* The BGM row: the selection number and the track title from g_BgmTrackNames. */
void DrawBgmSelector(void) {
    s32 x;
    char buf[88];
    u8 **scr = &SCRATCH_PRIM_CURSOR_AS(u8);
    u8 *p;
    void *ot = GamePrimaryOrderingTable(1);

    p = *scr;
    p = GameQueueSprite(ot, p, 0x14, 0xce, 0x58, 8, 0xa8, 0xe0, 0x7812);
    x = (g_BgmSelection == 0xa) ? 0x6c : 0x70;
    p = GameQueueSprite(ot, p, x, 0xce, 8, 8, 0x84, 0xc4, 0x7812);
    p = GameQueueSprite(ot, p, (g_BgmSelection == 0xa) ? 0x84 : 0x80, 0xce, 8, 8, 0x8c, 0xc4, 0x7812);
    p = QueueDrawModePrim(ot, p, 0x29);
    p = AddTilePrim(ot, p, 0x10, 0xcc, 0x5b, 0xc, 0x85, 0x15, 0xe);
    p = AddTilePrim(ot, p, 0x6c, 0xcc, 0x1f, 0xc, 0x40, 0x40, 0x40);
    p = AddTilePrim(ot, p, 0x8c, 0xcc, 0xa4, 0xc, 0, 0, 0);
    p = AddTilePrim(ot, p, 0xf, 0xcb, 0x122, 0xe, 0xff, 0xff, 0xff);
    *scr = p;

    sprintf(buf, g_FmtBgmNumber, g_BgmSelection);
    x = (g_BgmSelection == 0xa) ? 0x74 : 0x78;
    DrawText8x8(x, 0xce, buf, 0x78cc);
    DrawText8x8(0x90, 0xce, g_BgmTrackNames[g_BgmSelection], 0x78cc);
}


/* Scene 10: draws the ROUND screen, takes the BGM choice and starts the race at frame 121. */
void UpdateRoundScreen(void) {
    {
        u32 sceneTime = g_SceneTimer;

        if (sceneTime < 10000) {
            g_SceneTimer = g_SceneTimer + 1;
        }
    }
    if (g_SceneTimer == 0xf) {
        SetDispMask(1);
    }
    if (g_SceneTimer == 1) {
        SetupDisplay240(0, 0, 0);
    }
    DrawRoundScreen();
    if (g_SceneTimer == 0x20) {
        PlaySoundCue(0x19);
    }
    if (g_FadeLevel == 0) {
        if (RequestRaceAssets() == 0) {
            g_FadeLevel = 0x80;
        }
    } else {
        u32 sceneTime = g_SceneTimer;
        if (sceneTime >= 121) {
            g_SceneId = 0xb;
            if ((ReadStablePadHeld() & (PAD_START | PAD_R1 | PAD_L1)) == 0x80c) {
                g_MirrorMode = 1;
            } else {
                g_MirrorMode = 0;
            }
            if (g_BgmSelection == 0) {
                s32 idx = g_BgmShuffleIndex;
                u8 val = g_BgmShuffleOrder[idx];
                g_BgmShuffleIndex = idx + 1;
                g_BgmTrack = val;
                if (g_BgmShuffleIndex == g_BgmTrackCount) {
                    g_BgmShuffleIndex = 0;
                }
            } else {
                g_BgmTrack = g_BgmSelection - 1;
            }
            if (g_BgmTrack == 9) {
                g_BgmTrack = 0xe;
            }
        }
    }
    if (g_SceneId == 0xa) {
        u16 flags = g_PadPressed;
        if (flags & 0x8000) {
            g_BgmSelection = g_BgmSelection - 1;
        } else if (flags & 0x2000) {
            g_BgmSelection = g_BgmSelection + 1;
        }
        g_BgmSelection = (g_BgmSelection + g_BgmTrackCount + 1) % (g_BgmTrackCount + 1);
        DrawBgmSelector();
    }
}

/* Installs the track colour/light matrices, back and far colours and the fog near distance. */
void InitTrackLighting(void) {
    g_SceneColorMatrix = g_TrackColorMatrix;
    g_SceneLightMatrix = g_TrackLightMatrix;
    SetColorMatrix(&g_SceneColorMatrix);
    SetLightMatrix(&g_SceneLightMatrix);
    SetBackColor(0x20, 0x20, 0x20);
    SetFogNear(0x1770, 0x140);
    SetFarColor(0x80, 0x80, 0x80);
}
