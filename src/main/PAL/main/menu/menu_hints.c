#include "game/prim.h"
#include "game/menu.h"
#include "game/render_internal.h"

static int CanDrawMenuHint(void) {
    return g_DrawBuffer != NULL && g_RenderState.packetCursor != NULL;
}

/* The 0xC x 0x18 selection arrow every setup-menu list draws beside its rows. */
void DrawMenuCursorArrow(s32 x, s32 y) {
    GameOrderingTableEntry *ot;
    u8 *next;

    if (!CanDrawMenuHint()) return;

    ot = GamePrimaryOrderingTable(51);
    next = GameQueueSpriteTrans(ot, RENDER_PRIM_CURSOR_AS(u8), x, y,
                                0xC, 0x18, 0xE0, 0x48, 0x7F40);

    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x3F);
}

/* The bottom hint bar: a left arrow, the caption `variant` selects, and a
 * right arrow. Variant 4 is the wide one and gets a fixed position plus an
 * extra 0x30-wide sprite between the caption and the closing arrow. */
void DrawOptionHintBar(s32 variant) {
    const OptionHintCaption *caption;
    GameOrderingTableEntry *ot;
    s32 x;
    u8 *next;

    if ((u32)variant >= MENU_OPTION_HINT_COUNT || !CanDrawMenuHint()) return;

    caption = &g_OptionHintCaptions[variant];
    ot = GamePrimaryOrderingTable(0);
    next = RENDER_PRIM_CURSOR_AS(u8);

    if (variant == MENU_OPTION_HINT_NEGCON_CALIBRATION) {
        x = 0x5A;
    } else {
        x = (0x120 - caption->width) / 2;
    }

    next = GameQueueSpriteTrans(ot, next, x, 0x180, 0xC, 0x18,
                                0xE0, 0x78, 0x7F40);

    x += 0x10;
    next = GameQueueSpriteTrans(ot, next, x, 0x180, caption->width, 0x18,
                                caption->u, caption->v, 0x7F40);

    x += caption->advance;
    if (variant == MENU_OPTION_HINT_NEGCON_CALIBRATION) {
        next = GameQueueSpriteTrans(ot, next, x, 0x180, 0x30, 0x18,
                                    0, 0x78, 0x7F40);
        x += 0x34;
    }

    next = GameQueueSpriteTrans(ot, next, x, 0x180, 0xC, 0x18,
                                0xEC, 0x78, 0x7F40);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x3F);
}

/* Two glyphs plus a label naming the connected pad; caches the last valid g_PadType. */
void DrawPadTypeHint(void) {
    GameOrderingTableEntry *ot;
    u8 padType = g_PadType;
    u8 *next;
    s32 textureU;

    if (!CanDrawMenuHint()) return;

    if (padType != PAD_TYPE_DIGITAL && padType != PAD_TYPE_NEGCON) {
        padType = g_LastValidPadType;
    }
    if (padType != PAD_TYPE_DIGITAL && padType != PAD_TYPE_NEGCON) {
        padType = PAD_TYPE_DIGITAL;
    }
    g_LastValidPadType = padType;

    ot = GamePrimaryOrderingTable(0);
    next = RENDER_PRIM_CURSOR_AS(u8);
    textureU = padType == PAD_TYPE_NEGCON ? 0xA0 : 0x90;
    next = GameQueueSpriteTrans(ot, next, 0x7A, 0x1A0, 8, 0x10,
                                textureU, 0xB8, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0x92, 0x1A0, 8, 0x10,
                                textureU + 8, 0xB8, 0x7F40);
    next = GameQueueSpriteTrans(ot, next, 0x58, 0x1A0, 0x90, 0x10,
                                0, 0xB8, 0x7F40);
    g_RenderState.packetCursor = QueueDrawModePrim(ot, next, 0x3F);
}
