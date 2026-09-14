#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

enum {
    PATTERN_VISIBLE_ROWS = 6,
    PATTERN_STATIC_COUNT = 2,
    PATTERN_ANIMATION_FIRST_FRAME = PATTERN_STATIC_COUNT,
    PATTERN_ANIMATION_PERIOD = 6,
    PATTERN_FOOTER_BLOCKS = 16,
};

static s32 s_animationFrame = PATTERN_ANIMATION_FIRST_FRAME;

static const MenuOverlayPatternFrame *AnimatedPatternFrame(void) {
    const MenuOverlayPatternFrame *candidate;

    if ((g_AnimTimer % PATTERN_ANIMATION_PERIOD) == 0) {
        if (s_animationFrame <
            MENU_OVERLAY_PATTERN_FRAME_COUNT - 1) {
            s_animationFrame++;
        } else {
            s_animationFrame = PATTERN_ANIMATION_FIRST_FRAME;
        }
    }

    candidate = &g_MenuOverlayPatternTable[s_animationFrame];
    if (candidate->rows[MENU_OVERLAY_PATTERN_ROW_COUNT - 1] != 0) {
        s_animationFrame = PATTERN_ANIMATION_FIRST_FRAME;
    }
    return &g_MenuOverlayPatternTable[s_animationFrame];
}

void DrawBitPatternOverlay(s32 pattern) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE + 1;
    const MenuOverlayPatternFrame *frame;
    s32 rowIndex;
    s32 bitIndex;

    if (pattern == 0 || pattern > PATTERN_STATIC_COUNT) {
        return;
    }

    frame = pattern < 0
        ? AnimatedPatternFrame()
        : &g_MenuOverlayPatternTable[pattern - 1];

    for (rowIndex = 0; rowIndex < PATTERN_VISIBLE_ROWS; rowIndex++) {
        const u8 row = frame->rows[rowIndex];

        for (bitIndex = 0; bitIndex < 8; bitIndex++) {
            if ((row & (0x80U >> bitIndex)) != 0) {
                DrawSprite(ot, (s16)(0x22 + bitIndex * 4),
                           (s16)(0x150 + rowIndex * 8), 4, 8, 0xFC, 0, 0, 0,
                           0, 0x244, 1, 1, 0x80);
            }
        }
    }

    for (bitIndex = 0; bitIndex < PATTERN_FOOTER_BLOCKS; bitIndex++) {
        DrawSprite(ot, (s16)(0x4C + bitIndex * 5), 0x33, 4, 8, 0xFC, 0, 0,
                   0, 0, 0x244, 1, 1, 0x80);
    }

    g_RenderState.draw.packetCursor =
        QueueDrawModePrim(ot, RENDER_PRIM_CURSOR_AS(void), 0x39);
}
