#include "game/asset.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/cd.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/save_internal.h"
#include "game/screens.h"

enum {
    SCENE_TITLE = 6,
    SCENE_LOST_RACE = 14,
    SCENE_RACE_END = 16,
    LOST_RACE_INPUT_TIMER = -1,
    SCREEN_FADE_COMPLETE = 256,
    RACE_END_INITIAL_TIMER = 555,
};

void DrawLostRaceCaption(s32 level) {
    GameDrawProportionalTextShaded(0x28, 0x40, g_CaptionLostRace, 0x7812,
                                  RaceEndBrightness(level));
}

void EnterLostRaceScreen(void) {
    g_FrameSyncThreshold = 0x80;
    SetReverbDepth(0x28, 0x28);
    g_SceneId = SCENE_LOST_RACE;
    g_LostRaceChoice = 0;
    g_SceneTimer = LOST_RACE_INPUT_TIMER;
    DrawLostRaceCaption(0xFF);
}

static void DrawRaceEndPrompt(void) {
    s32 color = 0x7812;
    s32 drawColor;
    s32 index;

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

    index = LostRaceRetryDigitIndex(
        g_CourseProgress != NULL ? g_CourseProgress->retriesRemaining : 0);
    DrawProportionalText(0xBE, 0xB8, g_ChanceDigits[index], 0x7812);

    DrawText8x8(0x58, 0xD0, g_TextPressStart, 0x78CC);
    DrawLostRaceCaption(0xFF);
}

void UpdateLostRaceScreen(void) {
    s32 timer = g_SceneTimer;

    if (timer == LOST_RACE_INPUT_TIMER) {
        s32 previousChoice = g_LostRaceChoice;

        g_LostRaceChoice =
            UpdateLostRaceChoice(previousChoice, g_PadPressed);
        if (previousChoice != g_LostRaceChoice) {
            PlaySoundCue(1);
        }
        if (g_PadPressed & PAD_START) {
            PlaySoundCue(2);
            if (g_LostRaceChoice != 0) {
                RequestSelectBgmAssets();
            }
            g_SceneTimer = 0;
            if (g_CourseProgress != NULL &&
                g_CourseProgress->retriesRemaining > 0) {
                g_CourseProgress->retriesRemaining--;
            }
        }
    } else {
        timer = NextLostRaceFadeTimer(timer);
        g_SceneTimer = timer;
        DrawFullscreenFadeTile(timer, 0x49);
        if (g_SceneTimer >= SCREEN_FADE_COMPLETE) {
            g_SceneId = LostRaceExitScene(g_LostRaceChoice);
        }
    }

    DrawRaceEndPrompt();
}

void DrawRaceEndBanner(s32 level) {
    s32 brightness = RaceEndBrightness(level);

    DrawSprite(GamePrimaryOrderingTable(0), 0x50, 0x6C, 0xA0, 0x18, 0,
               0x28, brightness, brightness, brightness, 0xC, 0, 1, 0x29);
}

void EnterRaceEndScreen(void) {
    g_FrameSyncThreshold = 0x80;
    g_SceneId = SCENE_RACE_END;
    g_SceneTimer = RACE_END_INITIAL_TIMER;
    DrawRaceEndBanner(RACE_END_INITIAL_TIMER);
}

void UpdateRaceEndScreen(void) {
    s32 timer = NextRaceEndScreenTimer(g_SceneTimer);

    g_SceneTimer = timer;
    if (CanSkipRaceEndScreen(timer, g_PadPressed)) {
        StartCdVolumeFade(0xFA);
        g_SceneTimer = 0xFF;
    }
    if (g_SceneTimer == 0) {
        RequestSelectBgmAssets();
        ResetCourseProgress(g_GrandPrixClass);
        g_SceneId = SCENE_TITLE;
    }
    DrawRaceEndBanner(g_SceneTimer);
}
