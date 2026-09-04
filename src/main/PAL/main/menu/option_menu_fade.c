#include "game/menu.h"

#include <stdint.h>

enum {
    OPTION_FADE_OPAQUE = 0x100,
    OPTION_FADE_EXIT_STEP = 8,
};

/* Arms the fade-out that leaves the setup menu for scene `scene`. */
void StartOptionMenuExit(GameSceneId scene) {
    g_OptionMenuExitScene = scene;
    g_GameMode = OPTION_MODE_FADE;
    g_FadeStep = OPTION_FADE_EXIT_STEP;
}

/* OPTION_MODE_FADE: integrates the fade, then opens the root menu or leaves
 * for g_OptionMenuExitScene. */
void UpdateOptionMenuFade(void) {
    int64_t nextLevel = (int64_t)g_FadeLevel + g_FadeStep;

    if (nextLevel < 0) {
        g_FadeLevel = 0;
        g_FadeStep = 0;
        g_GameMode = OPTION_MODE_ROOT;
    } else if (nextLevel > OPTION_FADE_OPAQUE) {
        g_FadeLevel = OPTION_FADE_OPAQUE;
        g_SceneId = g_OptionMenuExitScene;
    } else {
        g_FadeLevel = (s32)nextLevel;
    }

    DrawFullscreenFadeTile480(g_FadeLevel, 0x49);
    DrawOptionRootMenu();
}
