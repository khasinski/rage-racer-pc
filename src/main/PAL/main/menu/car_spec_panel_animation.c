#include "game/menu.h"
#include "game/menu_internal.h"

s32 AdvanceCarSpecPanel(s32 *progress, s32 step) {
    s32 engineSpecStep = 0;

    if (step == 0) {
        *progress = 0;
        return 0;
    }

    *progress += step;
    if (step > 0) {
        if (*progress >= MENU_FADE_COMPLETE) {
            *progress = MENU_FADE_MAX;
        }
    } else {
        s32 fadeRemaining;

        if (*progress < 0) {
            *progress = 0;
        }
        fadeRemaining = MENU_FADE_MAX - *progress;
        engineSpecStep = fadeRemaining * fadeRemaining / 2048;
    }

    DrawCarEngineSpec(engineSpecStep, (u8)(*progress / 4));
    return *progress;
}
