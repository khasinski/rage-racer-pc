/*
 * The yes/no prompt both shops put up before they take the player's money.
 *
 * Two buttons side by side with a box round whichever one is picked, drawn
 * identically by the car shop and the engineer's shop, in both the waiting
 * state and the flashing one while the sale goes through.
 */

#include "game/menu.h"

void DrawShopPromptButtons(void *ot, s32 flash) {
    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20,
                      flash);
    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E);
    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95);
}
