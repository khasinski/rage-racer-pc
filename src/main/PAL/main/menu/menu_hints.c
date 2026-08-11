#include "common.h"
#include "game/prim.h"
#include "game/menu.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/scratchpad.h"


/* The 0xC x 0x18 selection arrow every setup-menu list draws beside its rows. */
void DrawMenuCursorArrow(s32 x, s32 y) {
    u8 *base = g_DrawBuffer;
    u8 **scratch = &SCRATCH_PRIM_CURSOR_AS(u8);
    u8 *prim;
    u8 *scratchValue;

    base += 0xCC;
    scratchValue = *scratch;
    prim = GameQueueSpriteTrans(base, scratchValue, x, y, 0xC, 0x18, 0xE0, 0x48, 0x7F40);
    *scratch = QueueDrawModePrim(base, prim, 0x3F);
}

/* The bottom hint bar: a left arrow, the caption `variant` selects, and a
 * right arrow. Variant 4 is the wide one and gets a fixed position plus an
 * extra 0x30-wide sprite between the caption and the closing arrow. */
void DrawOptionHintBar(s32 variant) {
    s32 index;
    s32 x;
    s32 which;
    u8 *base;
    u8 *prim;

    base = (u8 *)GamePrimaryOrderingTable(0);
    prim = SCRATCH_PRIM_CURSOR_AS(u8);
    which = variant;

    if (which == 4) {
        x = 0x5A;
    } else {
        x = (0x120 - g_OptionHintCaptions[which].width) / 2;
    }

    prim = GameQueueSpriteTrans(base, prim, x, 0x180, 0xC, 0x18, 0xE0, 0x78, 0x7F40);

    x += 0x10;
    index = which;
    prim = GameQueueSpriteTrans(base, prim, x, 0x180, g_OptionHintCaptions[index].width, 0x18,
                                g_OptionHintCaptions[index].u, g_OptionHintCaptions[index].v, 0x7F40);

    x += g_OptionHintCaptions[index].advance;
    if (which == 4) {
        prim = GameQueueSpriteTrans(base, prim, x, 0x180, 0x30, 0x18, 0, 0x78, 0x7F40);
        x += 0x34;
    }

    prim = GameQueueSpriteTrans(base, prim, x, 0x180, 0xC, 0x18, 0xEC, 0x78, 0x7F40);
    SCRATCH_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, prim, 0x3F);
}

/* Two glyphs plus a label naming the connected pad; caches the last valid g_PadType. */
void DrawPadTypeHint(void) {
    s32 clutIndex;
    s32 v;
    s32 h;
    s32 w;
    s32 u;
    u8 *base;
    s32 padType;
    s32 rawPadType;
    u8 *scratch;
    u8 *prim;

    base = (u8 *)GamePrimaryOrderingTable(0);
    rawPadType = g_PadType;
    scratch = SCRATCH_PRIM_CURSOR_AS(u8);
    prim = scratch;

    if (rawPadType != 0x41 && rawPadType != 0x23) {
        padType = g_LastValidPadType;
    } else {
        rawPadType = g_PadType;
        padType = (u8)rawPadType;
        g_LastValidPadType = rawPadType;
    }

    u = padType == 0x23 ? 0xA0 : 0x90;
    w = 8;
    h = 0x10;
    v = 0xB8;
    clutIndex = 0x7F40;

    prim = GameQueueSpriteTrans(base, prim, 0x7A, 0x1A0, w, h, u, v, clutIndex);
    prim = GameQueueSpriteTrans(base, prim, 0x92, 0x1A0, w, h, u + 8, v, clutIndex);
    prim = GameQueueSpriteTrans(base, prim, 0x58, 0x1A0, 0x90, h, 0, v, clutIndex);
    SCRATCH_PRIM_CURSOR_AS(u8) = QueueDrawModePrim(base, prim, 0x3F);
}
