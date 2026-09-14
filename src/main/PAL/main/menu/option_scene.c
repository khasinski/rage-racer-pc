#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/prim.h"
#include "game/render_internal.h"

enum {
    OPTION_LETTERBOX_STEP = 4,
    OPTION_LETTERBOX_MENU_HEIGHT = 240,
};

static s32 ApproachLetterboxHeight(s32 height, s32 target) {
    if (height < target) {
        return height > target - OPTION_LETTERBOX_STEP
                   ? target
                   : height + OPTION_LETTERBOX_STEP;
    }
    if (height > target) {
        return height < target + OPTION_LETTERBOX_STEP
                   ? target
                   : height - OPTION_LETTERBOX_STEP;
    }
    return height;
}

static void DrawOptionSceneOverlay(void) {
    OptionMenu *menu = MenuOption();
    GameOrderingTableEntry *ot = GamePrimaryOrderingTable(54);
    u8 *next;

    if (g_GameMode != OPTION_MODE_NEGCON_NEUTRAL) {
        DrawPadTypeHint();
    }

    menu->letterboxHeight = AddClampedMenuValue(
        menu->letterboxHeight, 0, 0, OPTION_LETTERBOX_MENU_HEIGHT);
    menu->letterboxHeight = ApproachLetterboxHeight(
        menu->letterboxHeight, OPTION_LETTERBOX_MENU_HEIGHT);

    next = RENDER_PRIM_CURSOR_AS(u8);
    g_RenderState.draw.packetCursor = AddTilePrim(
        ot, next, 0, 0, 0x140, menu->letterboxHeight, 0x85, 0x15, 0xE);
}

/* Scene 23: the setup / OPTION scene, dispatching g_GameModeHandlers[g_GameMode]. */
void UpdateOptionScene(void) {
    g_RenderState.draw.packetCursor = AddTilePrim(
        GamePrimaryOrderingTable(0), RENDER_PRIM_CURSOR_AS(u8),
        0, 0, 0x140, 2, 0, 0, 0);
    g_AnimTimer = (s32)((u32)g_AnimTimer + 1u);
    g_SceneTimer = (s32)((u32)g_SceneTimer + 1u);
    if (g_SceneTimer == 2) {
        SetDispMask(1);
    }
    if ((u32)g_GameMode >= OPTION_MODE_COUNT) {
        g_GameMode = OPTION_MODE_ROOT;
    }
    g_GameModeHandlers[g_GameMode]();
    DrawOptionSceneOverlay();
}
