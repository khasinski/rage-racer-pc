#include "game/menu.h"
#include "game/menu_internal.h"

s32 AdvanceCarSpecPanel(s32 *progress, s32 step) {
    s32 engineSpecStep = 0;
    int64_t updated;

    if (progress == NULL) {
        return 0;
    }
    if (step == 0) {
        *progress = 0;
        return 0;
    }

    updated = (int64_t)*progress + step;
    if (updated < 0) {
        updated = 0;
    } else if (updated >= MENU_FADE_COMPLETE) {
        updated = MENU_FADE_MAX;
    }
    *progress = (s32)updated;

    if (step < 0) {
        s32 fadeRemaining;

        fadeRemaining = MENU_FADE_MAX - *progress;
        engineSpecStep = fadeRemaining * fadeRemaining / 2048;
    }

    DrawCarEngineSpec(engineSpecStep, (u8)(*progress / 4));
    return *progress;
}
