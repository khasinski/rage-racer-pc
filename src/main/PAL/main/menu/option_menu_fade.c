#include "game/menu.h"

#include <stdint.h>

enum {
    OPTION_FADE_OPAQUE = 0x100,
    OPTION_FADE_EXIT_STEP = 8,
};

/* Arms the fade-out that leaves the setup menu for scene `scene`. */
void StartOptionMenuExit(GameSceneId scene) {
    MenuOption()->exitScene = scene;
    g_GameMode = OPTION_MODE_FADE;
    g_FadeStep = OPTION_FADE_EXIT_STEP;
}

/* OPTION_MODE_FADE: integrates the fade, then opens the root menu or leaves
 * for the destination stored by StartOptionMenuExit. */
void UpdateOptionMenuFade(void) {
    const s32 step = g_FadeStep;
    const int64_t next = (int64_t)g_FadeLevel + step;

    g_FadeLevel = StepFade(g_FadeLevel, step, OPTION_FADE_OPAQUE);
    if (step < 0 && g_FadeLevel == 0) {
        g_FadeStep = 0;
        g_GameMode = OPTION_MODE_ROOT;
    } else if (step > 0 && next > OPTION_FADE_OPAQUE) {
        g_SceneId = MenuOption()->exitScene;
    }

    DrawFullscreenFadeTile480(g_FadeLevel, 0x49);
    DrawOptionRootMenu();
}
