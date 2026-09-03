#include "game/prim.h"
#include "game/menu.h"
#include "game/render_internal.h"

enum {
    OPTION_FADE_OPAQUE = 0x100,
    OPTION_FADE_EXIT_STEP = 8,
};

/* The 0x140x0x1E0 twin of DrawFullscreenFadeTile, for the 480-line setup scene. */
void DrawFullscreenFadeTile480(s32 color, s32 tpage) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(0);
    u8 *next;

    if (color < 0) {
        color = 0;
    } else if (color >= OPTION_FADE_OPAQUE) {
        color = OPTION_FADE_OPAQUE - 1;
    }

    next = GameQueueTileTrans(ot, RENDER_PRIM_CURSOR_AS(u8), 0, 0,
                              0x140, 0x1E0, color, color, color);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, tpage);
}

/* Arms the fade-out that leaves the setup menu for scene `scene`. */
void StartOptionMenuExit(u32 scene) {
    g_OptionMenuExitScene = scene;
    g_GameMode = OPTION_MODE_FADE;
    g_FadeStep = OPTION_FADE_EXIT_STEP;
}

/* OPTION_MODE_FADE: integrates the fade, then opens the root menu or leaves
 * for g_OptionMenuExitScene. */
void UpdateOptionMenuFade(void) {
    g_FadeLevel += g_FadeStep;

    if (g_FadeLevel < 0) {
        g_FadeStep = 0;
        g_GameMode = OPTION_MODE_ROOT;
    } else if (g_FadeLevel > OPTION_FADE_OPAQUE) {
        g_SceneId = g_OptionMenuExitScene;
    }

    DrawFullscreenFadeTile480(g_FadeLevel, 0x49);
    DrawOptionRootMenu();
}
