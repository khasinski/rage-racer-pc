#include "game/prim.h"
#include "game/audio_internal.h"
#include "game/car.h"
#include "game/cd.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render_internal.h"
#include "game/track.h"

void DrawBgmSelectBar(void) {
    u8 *base;
    s32 tileW;
    s32 tileH;
    s32 temp;
    u8 *next;

    base = (u8 *)GamePrimaryOrderingTable(1);
    next = SCRATCH_PRIM_CURSOR_AS(u8);
    temp = (g_BgmSelectCursor == 0) ? 0x3FEC : 0x3FEF;
    tileW = 0x14;
    tileH = 0x10;

    next = GameQueueSprite(base, next, 0x20, 0xC1, tileW, tileH, 0, 0, temp);
    temp = (g_BgmSelectCursor == 1) ? 0x3FEC : 0x3FEF;
    next = GameQueueSprite(base, next, 0x36, 0xC1, tileW, tileH, tileW, 0, temp);
    temp = (g_BgmSelectCursor == 2) ? 0x3FEC : 0x3FEF;
    next = GameQueueSprite(base, next, 0x4C, 0xC1, tileW, tileH, 0x28, 0, temp);

    if (g_BgmRandomLabelTimer != 0) {
        g_BgmRandomLabelTimer--;
        temp = 0x10;
    } else {
        temp = g_BgmSelectTrack * 12 + 0x1C;
    }

    next = GameQueueSprite(base, next, 0x64, 0xC2, 0xBA, 0xC, 0, temp, 0x3FED);
    next = GameQueueSprite(base, next, 0x62, 0xC0, 0xBE, 0x10, 0x3C, 0, 0x3FEE);
    next = GameQueueTileTrans(base, next, 0x14, 0xB8, 0x118, 0x20, 0, 0, 0);
    SCRATCH_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, next, 0xB);
}

void AdvanceBgmShuffleBag(u32 track) {
    g_BgmShuffleIndex++;
    if (g_BgmShuffleIndex == g_BgmTrackCount) {
        ShuffleBgmOrder();

        if (track == g_BgmShuffleOrder[0]) {
            u8 tmp = g_BgmShuffleOrder[g_BgmTrackCount - 1];
            g_BgmShuffleOrder[0] = tmp;
            g_BgmShuffleOrder[g_BgmTrackCount - 1] = track;
        }
    }
}

void UpdateBgmSelect(void) {
    s32 t;
    if (g_BgmChangeDelay > 0) {
        t = g_BgmChangeDelay - 1;
        g_BgmChangeDelay = t;
        /* The empty t == 4 arm guarded nothing: the two values exclude each
         * other, so reaching the second test already means t is not 4. */
        if (t == 0) {
            if (g_BgmSelectCdTrack == 12) g_BgmSelectCdTrack = 17;
            RequestCdTrack(g_BgmSelectCdTrack);
            StartCdAudio();
            g_CdTrackEnded = 0;
        }
    } else {
        if (g_CdTrackEnded != 0) {
            g_BgmChangeDelay = 6;
            if (g_BgmRandomPlay != 0) {
                g_BgmSelectTrack = g_BgmShuffleOrder[g_BgmShuffleIndex];
                AdvanceBgmShuffleBag(g_BgmSelectTrack);
            } else {
                g_BgmSelectTrack++;
                g_BgmSelectTrack = (g_BgmSelectTrack + g_BgmTrackCount) % g_BgmTrackCount;
            }
            g_BgmSelectCdTrack = g_BgmSelectTrack + 3;
        }
    }

    if (g_SceneTimer == 2) SetDispMask(1);
    if (g_FadeStep == 0) {
    if (g_PadPressed & PAD_LEFT) {
        if (g_BgmSelectCursor > 0) g_BgmSelectCursor = g_BgmSelectCursor - 1;
    }
    if (g_PadPressed & PAD_RIGHT) {
        if (g_BgmSelectCursor < 2) g_BgmSelectCursor = g_BgmSelectCursor + 1;
    }
    if (g_PadPressed & 1) {
        s32 p;
        s32 h0;
        ShuffleBgmOrder();
        h0 = g_BgmShuffleOrder[0];
        p = g_BgmSelectTrack;
        if (p == h0) {
            u8 tmp = g_BgmShuffleOrder[g_BgmTrackCount - 1];
            g_BgmShuffleOrder[0] = tmp;
            g_BgmShuffleOrder[g_BgmTrackCount - 1] = p;
        }
        g_BgmRandomPlay = 1;
        g_BgmRandomLabelTimer = 60;
    }
    {
        u16 f = g_PadPressed;
        if (f & 2) {
            g_BgmRandomPlay = 0;
            g_BgmRandomLabelTimer = 0;
        }
        if (f & 0x860) {
            switch (g_BgmSelectCursor) {
            case 0:
                if (g_BgmRandomPlay == 0) {
                    g_BgmSelectTrack--;
                    g_BgmSelectTrack = (g_BgmSelectTrack + g_BgmTrackCount) % g_BgmTrackCount;
                }
                if (g_BgmChangeDelay == 0) {
                    StartCdVolumeFade(60);
                    g_BgmChangeDelay = 0x40;
                }
                g_BgmSelectCdTrack = g_BgmSelectTrack + 3;
                break;
            case 2:
                if (g_BgmRandomPlay != 0) {
                    g_BgmSelectTrack = g_BgmShuffleOrder[g_BgmShuffleIndex];
                    AdvanceBgmShuffleBag(g_BgmSelectTrack);
                } else {
                    g_BgmSelectTrack++;
                    g_BgmSelectTrack = (g_BgmSelectTrack + g_BgmTrackCount) % g_BgmTrackCount;
                }
                if (g_BgmChangeDelay == 0) {
                    StartCdVolumeFade(60);
                    g_BgmChangeDelay = 0x40;
                }
                g_BgmSelectCdTrack = g_BgmSelectTrack + 3;
                break;
            case 1:
                StartCdVolumeFade(60);
                g_FadeStep = 4;
                break;
            }
        } else if (f & 0x90) {
            StartCdVolumeFade(60);
            g_FadeStep = 4;
        }
    }
    if (g_PadPressed & 4) g_BgmSelectShowUi = 1;
    if (g_PadPressed & 8) g_BgmSelectShowUi = 0;
    } else {
    DrawFullscreenFadeTile(g_FadeLevel, 0x49);
    g_FadeLevel = g_FadeLevel + g_FadeStep;
    if (g_FadeLevel >= 256) {
        RequestOptionScreenAssets();
        g_BgmSelectStep = BGM_SELECT_STEP_EXIT;
        g_FadeLevel = 256;
        g_FadeStep = -4;
    }
    }

    if (g_BgmSelectShowUi != 0) DrawBgmSelectBar();
    g_AnimTimer++;
    g_CameraCarIndex = CycleBgmSelectCameraCar(0xff, g_CameraCarIndex);
    UpdateAttractCars();
    RequestTrackTexturePage(g_Cars[g_CameraCarIndex].trackSection);
    UpdateCamera(g_CameraViewMode, (GameRenderObject *)&g_Cars[g_CameraCarIndex]);
    DrawCars();
    UpdateEnvironment();
    DrawSkyBackground();
    SCRATCH_ENV_MODE4 = g_IsEnvironmentMode4;
    DrawTerrainCellsWide();
    DrawCourseObjects();
    DrawCourseScenery2(g_AnimTimer, 1);
}
