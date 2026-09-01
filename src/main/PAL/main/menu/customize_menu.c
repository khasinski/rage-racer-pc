#include "game/menu.h"

s32 DrawCustomizeScreen(s32 step) {
    s32 engineSpecStep = 0;

    if (step == 0) {
        g_CustomizeFadeAccum = 0;
        return 0;
    }

    g_CustomizeFadeAccum += step;
    if (step > 0) {
        if (g_CustomizeFadeAccum >= MENU_FADE_COMPLETE) {
            g_CustomizeFadeAccum = MENU_FADE_MAX;
        }
    } else {
        s32 fadeRemaining;

        if (g_CustomizeFadeAccum < 0) {
            g_CustomizeFadeAccum = 0;
        }
        fadeRemaining = MENU_FADE_MAX - g_CustomizeFadeAccum;
        engineSpecStep = fadeRemaining * fadeRemaining / 2048;
    }

    DrawCarEngineSpec(engineSpecStep, (u8)(g_CustomizeFadeAccum / 4));
    return g_CustomizeFadeAccum;
}
