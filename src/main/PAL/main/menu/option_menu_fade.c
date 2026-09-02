#include "game/prim.h"
#include "game/menu.h"
#include "game/render_internal.h"

/* The 0x140x0x1E0 twin of DrawFullscreenFadeTile, for the 480-line setup scene. */

void DrawFullscreenFadeTile480(s32 color, s32 tpage) {
    GameOrderingTableEntry *base = GamePrimaryOrderingTable(0);
    u8 *next;

    if (color < 0) {
        color = 0;
    } else if (color >= 0x100) {
        color = 0xFF;
    }

    next = GameQueueTileTrans(base, RENDER_PRIM_CURSOR_AS(u8), 0, 0,
                              0x140, 0x1E0, color, color, color);
    RENDER_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, next, tpage);
}

/* Arms the fade-out that leaves the setup menu for scene `scene`. */
void StartOptionMenuExit(u32 scene) {
    g_OptionMenuExitScene = scene;
    g_GameMode = 0;
    g_FadeStep = 8;
}

/* g_GameModeHandlers[0]: integrates the fade, then enters mode 1 or leaves for g_OptionMenuExitScene. */
void UpdateOptionMenuFade(void) {
    g_FadeLevel += g_FadeStep;

    if (g_FadeLevel < 0) {
        g_FadeStep = 0;
        g_GameMode = 1;
    } else if (g_FadeLevel >= 0x101) {
        g_SceneId = g_OptionMenuExitScene;
    }

    DrawFullscreenFadeTile480(g_FadeLevel, 0x49);
    DrawOptionRootMenu();
}
