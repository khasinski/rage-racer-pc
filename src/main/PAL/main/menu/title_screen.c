#include "common.h"
#include "game/prim.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/audio_internal.h"
#include "game/cd.h"
#include "game/car.h"
#include "game/frontend_internal.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/random.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/fmv_internal.h"
#include "game/save_internal.h"
#include "game/scratchpad.h"
#include "game/screens.h"
#include "game/state.h"
#include "psyq/gpu.h"


/* Scene 2: the menu-side entry to the front end. Clears the title/menu
 * state words and hands over to scene 4, UpdateFrontend. */
void EnterFrontend(void) {
    SetDispMask(0);
    CloseLoadedAudioSlots();
    ResetTrackTextureSwap();
    UploadLoadBufferImage();

    g_FrameSyncThreshold = 0x80;
    g_SceneId = 4;
    g_SceneTimer = 0;
    g_FrontendIdleTimer = 0;
    g_TitleFadeLevel = 0;
    g_MainMenuSlide = 0;
    g_TitlePulse = 0;
    g_FrontendState = FRONTEND_STATE_TITLE;
    g_TitleExitTimer = 0;
    g_TitleAttractTimer = -1;

    UpdateBgmTrackCount();
    SetDefaultReverbDepth();
}


void EnterTitleScreen(void) {
    SetupDisplay240(0, 0, 0);
    if (g_StreamReturnScene != 0) {
        g_TitleFadeLevel = 0xFF;
        g_TitleAttractTimer = 0x190;
        g_TitleExitTimer = 0;
    } else {
        SetDispMask(0);
        UploadLoadBufferImage();
        g_TitleFadeLevel = 0;
        g_TitleAttractTimer = 0;
        g_TitleExitTimer = 0x1E;
    }
    g_FrameSyncThreshold = 0x80;
    g_SceneTimer = 0;
    g_SceneId = 4;
    g_FrontendIdleTimer = 0;
    g_MainMenuSlide = 0;
    g_FrontendState = FRONTEND_STATE_TITLE;
    UpdateBgmTrackCount();
    SetDefaultReverbDepth();
    DrawPressStartPrompt();
}


void DrawTitleFadeOverlay(s32 brightness) {
    void *current;
    void **scratch;
    u8 *base;
    void *next;
    s32 color;

    color = (u8)brightness;
    base = (u8 *)GamePrimaryOrderingTable(0);
    scratch = &SCRATCH_PRIM_CURSOR_AS(void);
    current = *scratch;
    next = GameQueueTileTrans(base, current, 0, 0x18, 0x140, 0xC0, color, color, color);
    *scratch = QueueDrawModePrim(base, next, 0x29);
}


void DrawPressStartPrompt(void) {
    void **scratch;
    u8 *base;
    void *next;
    s32 sinValue;
    s32 frame;

    if (g_TitleFadeLevel > 0) {
        DrawTitleFadeOverlay((u8)g_TitleFadeLevel);
        g_TitleFadeLevel -= 2;
    }

    sinValue = rsin(((g_AnimTimer * 3) << 5) & 0xFE0);
    frame = (sinValue / 64) + 0x80;

    scratch = &SCRATCH_PRIM_CURSOR_AS(void);
    base = (u8 *)GamePrimaryOrderingTable(0);
    next = *scratch;
    next = GameQueueShadedSprite(base, next, 0x68, 0xC8, 0x70, 0x10, 0x70, 0xA0, 0x7E84, frame);
    *scratch = QueueDrawModePrim(base, next, 0x39);
}


void UpdateTitleScreen(void) {
    if (g_PadPressed & PAD_START) {
        PlaySoundCue(2);
        g_FrontendState = FRONTEND_STATE_MENU_OPENING;
        g_FrontendIdleTimer = 0;
        g_TitleMenuSelection = 0;
        if (g_TitleAttractTimer > 0) {
            g_TitleAttractTimer = 0;
            StartCdVolumeFade(1);
        }
    }
    DrawPressStartPrompt();
}


void DrawMainMenuRows(void) {
    void *scratch;
    u8 *base;
    s32 row;
    s32 i;
    s32 one;
    s32 width;
    s32 y;

    base = (u8 *)GamePrimaryOrderingTable(0);
    scratch = SCRATCH_PRIM_CURSOR_AS(void);
    row = 0;
    i = 0;
    one = 1;
    width = 0x70;
    y = 0x64;

    while (i < 5) {
        s32 code;
        s32 frame;
        s32 delta;

        code = 0x7E85;

        if ((g_ExtraGrandPrixUnlocked == 0) && (i == one)) {
            i = 2;
        }

        if (i == g_TitleMenuSelection) {
            code = 0x7E86;
        }

        if (g_FrontendState == one) {
            code = 0x7E85;
        }

        if ((g_TitlePulse & 2) != 0) {
            code = 0x7E85;
        }

        delta = g_MainMenuSlide - (row * 8);
        if (delta >= 0) {
            frame = delta;
            if (frame >= 0x11) {
                frame = 0x10;
            }
        } else {
            frame = 0;
        }

        scratch = GameQueueTexturedRect(base, scratch, 0x68, y, width, frame, 0, (i * 16) + 0xA0, width, 0x10, code, 0x39);
        y += 0x18;
        i++;
        row++;
    }

    SCRATCH_PRIM_CURSOR_AS(void) = scratch;
}

void UpdateMainMenuOpen(void) {
    if (++g_MainMenuSlide == 0x30) {
        g_FrontendState = FRONTEND_STATE_MENU_INPUT;
    }

    DrawMainMenuRows();
}


/* Refills g_BgmShuffleOrder with a random permutation of the
 * g_BgmTrackCount tracks and rewinds g_BgmShuffleIndex. */
void ShuffleBgmOrder(void) {
    s32 i;
    s32 count;
    s32 j;
    s32 remaining;

    for (i = 0; i < g_BgmTrackCount; i++) {
        g_BgmShuffleOrder[i] = 0xFF;
    }

    for (i = 0; i < g_BgmTrackCount; i++) {
        count = 0;
        for (j = 0; j < g_BgmTrackCount; j++) {
            if (g_BgmShuffleOrder[j] == 0xFF) {
                count++;
            }
        }

        remaining = ((Random15() & 0xFFF) % count) + 1;
        j = 0;
        while (remaining != 0) {
            if (g_BgmShuffleOrder[j] == 0xFF) {
                remaining--;
            }
            j++;
        }
        g_BgmShuffleOrder[j - 1] = i;
    }

    g_BgmShuffleIndex = 0;
}


void UpdateMainMenuInput(void) {
    volatile u16 *flagp = &g_PadPressed;
    s32 idx;
    u16 flags;

    if (*flagp != 0) {
        g_FrontendIdleTimer = 0;
    }
    flags = *flagp;
    idx = g_TitleMenuSelection;

    if (flags & 0x1000) {
        s32 n = idx - 1;
        g_TitleMenuSelection = n;
        if (g_ExtraGrandPrixUnlocked == 0 && n == 1) {
            g_TitleMenuSelection = idx - 2;
        }
    } else if (flags & 0x4000) {
        s32 n = idx + 1;
        g_TitleMenuSelection = n;
        if (g_ExtraGrandPrixUnlocked == 0 && n == 1) {
            g_TitleMenuSelection = idx + 2;
        }
    }

    {
        s32 m = g_TitleMenuSelection + 5;
        m = m % 5;
        g_TitleMenuSelection = m;
        if (idx != m) {
            PlaySoundCue(1);
        }
    }

    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        if (g_AssetLoadState != 0) {
            ResetAssetLoader();
        }
        ShuffleBgmOrder();
        switch (g_TitleMenuSelection) {
        case 0:
            g_CarTable = g_GrandPrixCars;
            g_RaceProgress = &g_GrandPrixSave;
            g_CourseProgress = &g_GrandPrixCourseProgress;
            g_SeriesSelection = 0;
            if (g_GrandPrixSave.maxClassReached == -1) {
                g_GrandPrixClass = 0;
                g_CourseIndex = 3;
                RequestTrackLoad();
            } else {
                RequestSelectBgmAssetsNoReset();
            }
            break;
        case 1:
            g_CarTable = g_ExtraGrandPrixCars;
            g_RaceProgress = &g_ExtraGrandPrixSave;
            g_CourseProgress = &g_ExtraGrandPrixCourseProgress;
            g_SeriesSelection = 1;
            if (g_ExtraGrandPrixSaveMaxClass == -1) {
                g_GrandPrixClass = 0;
                g_CourseIndex = 3;
                RequestTrackLoad();
            } else {
                RequestSelectBgmAssetsNoReset();
            }
            break;
        case 2:
            g_CarTable = g_TimeAttackCars;
            g_RaceProgress = &g_TimeAttackSave;
            g_SeriesSelection = 0;
            RequestSelectBgmAssetsNoReset();
            break;
        case 3:
            RequestSaveScreenAssets();
            break;
        case 4:
            RequestOptionScreenAssets();
            g_OptionMenuCursor = 0;
            break;
        }
        g_FrontendState = FRONTEND_STATE_MENU_EXIT;
    }
    DrawMainMenuRows();
}
