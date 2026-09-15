#include "game/asset.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/render_internal.h"
#include "game/cd.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/save_internal.h"
#include "game/screens.h"
#include "game/scene.h"
#include "game/scene_runtime.h"

enum {
    LOST_RACE_INPUT_TIMER = -1,
};

void DrawLostRaceCaption(s32 level) {
    GameDrawProportionalTextShaded(0x28, 0x40, "h L O S T  R A C E i", 0x7812,
                                  RaceEndBrightness(level));
}

void EnterLostRaceScreen(void) {
    LostRace *state = SceneRuntimeLostRace();

    g_FrameSyncThreshold = 0x80;
    SetReverbDepth(0x28, 0x28);
    g_SceneId = GAME_SCENE_LOST_RACE;
    state->choice = 0;
    g_SceneTimer = LOST_RACE_INPUT_TIMER;
    DrawLostRaceCaption(0xFF);
}

static void DrawRaceEndPrompt(const LostRace *state) {
    char chance[2];
    s32 color = 0x7812;
    s32 drawColor;
    s32 index;

    if (g_SceneTimer & 4) {
        color = 0x784C;
    }

    drawColor = 0x7812;
    if (state->choice == 0) {
        drawColor = color;
    }
    DrawProportionalText(0x6A, 0x68, "TRY AGAIN", drawColor);

    drawColor = 0x7812;
    if (state->choice != 0) {
        drawColor = color;
    }
    DrawProportionalText(0x70, 0x78, "END RACE", drawColor);

    DrawProportionalText(0x76, 0xB8, "CHANCE", 0x7812);

    index = LostRaceRetryDigitIndex(
        g_CourseProgress != NULL ? g_CourseProgress->retriesRemaining : 0);
    chance[0] = (char)('0' + index);
    chance[1] = '\0';
    DrawProportionalText(0xBE, 0xB8, chance, 0x7812);

    DrawText8x8(0x58, 0xD0, "PRESS START BUTTON", 0x78CC);
    DrawLostRaceCaption(0xFF);
}

void UpdateLostRaceScreen(void) {
    LostRace *state = SceneRuntimeLostRace();
    s32 timer = g_SceneTimer;

    if (timer == LOST_RACE_INPUT_TIMER) {
        s32 previousChoice = state->choice;

        state->choice =
            UpdateLostRaceChoice(previousChoice, g_PadPressed);
        if (previousChoice != state->choice) {
            PlaySoundCue(1);
        }
        if (g_PadPressed & PAD_START) {
            PlaySoundCue(2);
            if (state->choice != 0) {
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
        if (g_SceneTimer >= RACE_END_SCREEN_FADE_COMPLETE) {
            g_SceneId = LostRaceExitScene(state->choice);
        }
    }

    DrawRaceEndPrompt(state);
}

void DrawRaceEndBanner(s32 level) {
    s32 brightness = RaceEndBrightness(level);

    DrawSprite(GamePrimaryOrderingTable(0), 0x50, 0x6C, 0xA0, 0x18, 0,
               0x28, brightness, brightness, brightness, 0xC, 0, 1, 0x29);
}

void EnterRaceEndScreen(void) {
    g_FrameSyncThreshold = 0x80;
    g_SceneId = GAME_SCENE_RACE_END;
    g_SceneTimer = RACE_END_SCREEN_INITIAL_TIMER;
    DrawRaceEndBanner(RACE_END_SCREEN_INITIAL_TIMER);
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
        if (g_CourseProgress != NULL) {
            ResetCourseProgressState(g_CourseProgress, g_GrandPrixClass);
        }
        g_SceneId = GAME_SCENE_INIT_MENU;
    }
    DrawRaceEndBanner(g_SceneTimer);
}
