#include "game/menu.h"

s32 DrawEngineerShopScreen(s32 step) {
    s32 engineSpecStep = 0;

    if (step == 0) {
        g_EngineSpecStep = 0;
        return 0;
    }

    g_EngineSpecStep += step;
    if (step > 0) {
        if (g_EngineSpecStep >= MENU_FADE_COMPLETE) {
            g_EngineSpecStep = MENU_FADE_MAX;
        }
    } else {
        s32 fadeRemaining;

        if (g_EngineSpecStep < 0) {
            g_EngineSpecStep = 0;
        }
        fadeRemaining = MENU_FADE_MAX - g_EngineSpecStep;
        engineSpecStep = fadeRemaining * fadeRemaining / 2048;
    }

    DrawCarEngineSpec(engineSpecStep, (u8)(g_EngineSpecStep / 4));
    return g_EngineSpecStep;
}
