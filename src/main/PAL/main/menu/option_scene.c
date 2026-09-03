#include "game/menu.h"
#include "game/prim.h"
#include "game/render_internal.h"

void DrawOptionSceneOverlay(void) {
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(54);
    s32 targetHeight = g_GameMode == OPTION_MODE_SCREEN_ADJUST ? 0x1E0 : 0xF0;
    u8 *next;

    if (g_GameMode != OPTION_MODE_NEGCON_NEUTRAL) {
        DrawPadTypeHint();
    }

    if (g_OptionLetterboxHeight < targetHeight) {
        g_OptionLetterboxHeight += 4;
    } else if (g_OptionLetterboxHeight > targetHeight) {
        g_OptionLetterboxHeight -= 4;
    }

    next = RENDER_PRIM_CURSOR_AS(u8);
    if (g_GameMode == OPTION_MODE_SCREEN_ADJUST) {
        next = AddTilePrim(ot, next, 0x10, 0x20, 0x120, 2,
                           0xFF, 0xFF, 0xFF);
        next = AddTilePrim(ot, next, 0x10, 0x1C0, 0x120, 2,
                           0xFF, 0xFF, 0xFF);
        next = GameQueueLine(ot, next, 0x10, 0x20, 0x10, 0x1C0,
                             0xFF, 0xFF, 0xFF);
        next = GameQueueLine(ot, next, 0x130, 0x20, 0x130, 0x1C0,
                             0xFF, 0xFF, 0xFF);
    }

    g_RenderState.packetCursor = AddTilePrim(
        ot, next, 0, 0, 0x140, g_OptionLetterboxHeight, 0x85, 0x15, 0xE);
}

/* Scene 23: the setup / OPTION scene, dispatching g_GameModeHandlers[g_GameMode]. */
void UpdateOptionScene(void) {
    g_RenderState.packetCursor = AddTilePrim(
        GamePrimaryOrderingTable(0), RENDER_PRIM_CURSOR_AS(u8),
        0, 0, 0x140, 2, 0, 0, 0);
    g_AnimTimer++;
    g_SceneTimer++;
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    if ((u32)g_GameMode >= OPTION_MODE_COUNT) {
        g_GameMode = OPTION_MODE_ROOT;
    }
    g_GameModeHandlers[g_GameMode]();
    DrawOptionSceneOverlay();
}
