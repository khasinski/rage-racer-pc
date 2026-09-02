#include "game/race_internal.h"
#include "game/state.h"

enum {
    SCENE_TITLE = 6,
    SCENE_RACE = 11,
    RACE_END_MAX_BRIGHTNESS = 127,
    RACE_END_SKIP_MIN_TIMER = 261,
};

s32 RaceEndBrightness(s32 level) {
    if (level <= 0) {
        return 0;
    }
    if (level >= 256) {
        return RACE_END_MAX_BRIGHTNESS;
    }
    return level >> 1;
}

s32 UpdateLostRaceChoice(s32 choice, u16 pressedButtons) {
    s32 previousChoice = choice;

    if ((pressedButtons & PAD_UP) && previousChoice == 1) {
        choice = 0;
    }
    if ((pressedButtons & PAD_DOWN) && choice == 0) {
        choice = 1;
    }
    return choice;
}

s32 LostRaceExitScene(s32 choice) {
    return choice != 0 ? SCENE_TITLE : SCENE_RACE;
}

s32 CanSkipRaceEndScreen(s32 timer, u16 pressedButtons) {
    return timer >= RACE_END_SKIP_MIN_TIMER &&
        (pressedButtons & PAD_CONFIRM) != 0;
}
