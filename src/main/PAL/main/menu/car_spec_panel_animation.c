#include "game/menu.h"
#include "game/menu_internal.h"

s32 AdvanceCarSpecPanel(s32 *progress, s32 step) {
    s32 engineSpecStep = 0;

    if (progress == NULL) {
        return 0;
    }
    if (step == 0) {
        *progress = 0;
        return 0;
    }

    *progress = AddClampedMenuValue(*progress, step, 0, MENU_FADE_MAX);

    if (step < 0) {
        s32 fadeRemaining;

        fadeRemaining = MENU_FADE_MAX - *progress;
        engineSpecStep = fadeRemaining * fadeRemaining / 2048;
    }

    DrawCarEngineSpec(
        engineSpecStep, (u8)(*progress / MENU_FADE_INTENSITY_DIVISOR));
    return *progress;
}
