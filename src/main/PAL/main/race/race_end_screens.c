#include "common.h"
#include "game/screens.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/cd.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/save_internal.h"

void UpdateBgmTrackCount(void) {
    s32 index;
    s32 current;
    s32 one;
    s32 value;

    g_ClassWinCount = 0;
    one = 1;
    index = 0;
    do {
        current = g_ClassRecords[index].place;
        index++;
        if (current == one) {
            g_ClassWinCount = g_ClassWinCount + 1;
        }
    } while (index < 11);

    value = g_ClassWinCount;
    value = value < 5;
    if (value) {
        value = 9;
    } else {
        value = 10;
    }
    g_BgmTrackCount = value;
}

void DrawLostRaceCaption(s32 level) {
    if (level >= 0x100) {
        level = 0xFF;
    }
    level >>= 1;
    GameDrawProportionalTextShaded(0x28, 0x40, &g_CaptionLostRace, 0x7812, level);
}

void EnterLostRaceScreen(void) {
    g_FrameSyncThreshold = 0x80;
    SetReverbDepth(0x28, 0x28);
    g_SceneId = 0xE;
    g_LostRaceChoice = 0;
    g_SceneTimer = -1;
    DrawLostRaceCaption(0xFF);
}

void DrawRaceEndPrompt(void) {
    s32 color = 0x7812;
    s32 drawColor;
    s16 index;

    if (g_SceneTimer & 4) {
        color = 0x784C;
    }

    drawColor = 0x7812;
    if (g_LostRaceChoice == 0) {
        drawColor = color;
    }
    DrawProportionalText(0x6A, 0x68, g_TextTryAgain, drawColor);

    drawColor = 0x7812;
    if (g_LostRaceChoice != 0) {
        drawColor = color;
    }
    DrawProportionalText(0x70, 0x78, g_TextEndRace, drawColor);

    DrawProportionalText(0x76, 0xB8, g_TextChance, 0x7812);

    index = g_CourseProgress->retriesRemaining;
    DrawProportionalText(0xBE, 0xB8, g_ChanceDigits[index], 0x7812);

    DrawText8x8(0x58, 0xD0, g_TextPressStart, 0x78CC);
    DrawLostRaceCaption(0xFF);
}

void UpdateLostRaceScreen(void) {
    s32 timer;
    s32 old;
    s32 current;
    CourseProgressState *ptr;
    u16 value;

    timer = g_SceneTimer;
    if (timer == -1) {
        old = g_LostRaceChoice;
        if ((g_PadPressed & PAD_UP) && (old == 1)) {
            g_LostRaceChoice = 0;
        }
        if ((g_PadPressed & PAD_DOWN) && (g_LostRaceChoice == 0)) {
            g_LostRaceChoice = 1;
        }
        current = g_LostRaceChoice;
        if (old != current) {
            PlaySoundCue(1);
        }
        if (g_PadPressed & PAD_START) {
            PlaySoundCue(2);
            if (g_LostRaceChoice != 0) {
                RequestSelectBgmAssets();
            }
            ptr = g_CourseProgress;
            value = ptr->retriesRemaining;
            g_SceneTimer = 0;
            ptr->retriesRemaining = value - 1;
        }
    } else {
        timer += 2;
        g_SceneTimer = timer;
        DrawFullscreenFadeTile(timer, 0x49);
        if (g_SceneTimer == 0x100) {
            if (g_LostRaceChoice != 0) {
                g_SceneId = 6;
            } else {
                g_SceneId = 0xB;
            }
        }
    }

    DrawRaceEndPrompt();
}

void DrawRaceEndBanner(s32 level) {
    if (level >= 256) {
        level = 0xFF;
    }
    level >>= 1;
    DrawSprite(GamePrimaryOrderingTable(0), 0x50, 0x6C, 0xA0, 0x18, 0, 0x28, level, level, level, 0xC, 0, 1, 0x29);
}

void EnterRaceEndScreen(void) {
    g_FrameSyncThreshold = 0x80;
    g_SceneId = 0x10;
    g_SceneTimer = 0x22B;
    DrawRaceEndBanner(0x22B);
}

void UpdateRaceEndScreen(void) {
    u32 v = g_SceneTimer - 1;
    g_SceneTimer = v;
    if ((g_PadPressed & PAD_CONFIRM) && v >= 261) {
        StartCdVolumeFade(0xFA);
        g_SceneTimer = 0xFF;
    }
    if (g_SceneTimer == 0) {
        RequestSelectBgmAssets();
        ResetCourseProgress(g_GrandPrixClass);
        g_SceneId = 6;
    }
    DrawRaceEndBanner(g_SceneTimer);
}
