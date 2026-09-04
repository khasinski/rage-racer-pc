#include "game/race_internal.h"
#include "game/scene.h"
#include "game/state.h"

enum {
    RACE_END_MAX_BRIGHTNESS = 127,
    RACE_END_SKIP_MIN_TIMER = 261,
    SCREEN_FADE_STEP = 2,
    LOST_RACE_RETRY_DIGIT_COUNT = 6,
};

s32 RaceEndBrightness(s32 level) {
    if (level <= 0) {
        return 0;
    }
    if (level >= RACE_END_SCREEN_FADE_COMPLETE) {
        return RACE_END_MAX_BRIGHTNESS;
    }
    return level >> 1;
}

s32 UpdateLostRaceChoice(s32 choice, u16 pressedButtons) {
    s32 previousChoice;

    if (choice != 0 && choice != 1) {
        choice = 0;
    }
    previousChoice = choice;

    if ((pressedButtons & PAD_UP) && previousChoice == 1) {
        choice = 0;
    }
    if ((pressedButtons & PAD_DOWN) && choice == 0) {
        choice = 1;
    }
    return choice;
}

s32 LostRaceRetryDigitIndex(s32 retriesRemaining) {
    if (retriesRemaining < 0) {
        return 0;
    }
    if (retriesRemaining >= LOST_RACE_RETRY_DIGIT_COUNT) {
        return LOST_RACE_RETRY_DIGIT_COUNT - 1;
    }
    return retriesRemaining;
}

s32 LostRaceExitScene(s32 choice) {
    return choice != 0 ? GAME_SCENE_INIT_MENU : GAME_SCENE_ENTER_RACE;
}

s32 CanSkipRaceEndScreen(s32 timer, u16 pressedButtons) {
    return timer >= RACE_END_SKIP_MIN_TIMER &&
        (pressedButtons & PAD_CONFIRM) != 0;
}

s32 NextLostRaceFadeTimer(s32 timer) {
    if (timer < 0) {
        return 0;
    }
    return timer >= RACE_END_SCREEN_FADE_COMPLETE - SCREEN_FADE_STEP
               ? RACE_END_SCREEN_FADE_COMPLETE
               : timer + SCREEN_FADE_STEP;
}

s32 NextRaceEndScreenTimer(s32 timer) {
    if (timer <= 0) {
        return 0;
    }
    if (timer > RACE_END_SCREEN_INITIAL_TIMER) {
        timer = RACE_END_SCREEN_INITIAL_TIMER;
    }
    return timer - 1;
}
