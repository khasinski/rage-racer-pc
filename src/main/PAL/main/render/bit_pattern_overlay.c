#include "game/prim.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/state.h"

enum {
    PATTERN_FRAME_SIZE = 8,
    PATTERN_VISIBLE_ROWS = 6,
    PATTERN_STATIC_COUNT = 2,
    PATTERN_ANIMATION_START = 16,
    PATTERN_ANIMATION_PERIOD = 6,
    PATTERN_FOOTER_BLOCKS = 16,
    PATTERN_TABLE_SIZE = 584,
};

static const u8 *AnimatedPatternRows(void) {
    const u8 *candidate;

    if (g_MenuOverlayPatternAnimOffset < PATTERN_ANIMATION_START ||
        g_MenuOverlayPatternAnimOffset >
            PATTERN_TABLE_SIZE - PATTERN_FRAME_SIZE ||
        g_MenuOverlayPatternAnimOffset % PATTERN_FRAME_SIZE != 0) {
        g_MenuOverlayPatternAnimOffset = PATTERN_ANIMATION_START;
    }

    if ((g_AnimTimer % PATTERN_ANIMATION_PERIOD) == 0) {
        if (g_MenuOverlayPatternAnimOffset <=
            PATTERN_TABLE_SIZE - 2 * PATTERN_FRAME_SIZE) {
            g_MenuOverlayPatternAnimOffset += PATTERN_FRAME_SIZE;
        } else {
            g_MenuOverlayPatternAnimOffset = PATTERN_ANIMATION_START;
        }
    }

    candidate = &g_MenuOverlayPatternTable[g_MenuOverlayPatternAnimOffset];
    if (candidate[PATTERN_FRAME_SIZE - 1] != 0) {
        g_MenuOverlayPatternAnimOffset = PATTERN_ANIMATION_START;
    }
    return &g_MenuOverlayPatternTable[g_MenuOverlayPatternAnimOffset];
}

void DrawBitPatternOverlay(s32 pattern) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE + 1;
    const u8 *rows;
    s32 rowIndex;
    s32 bitIndex;

    if (pattern == 0 || pattern > PATTERN_STATIC_COUNT) {
        return;
    }

    rows = pattern < 0
        ? AnimatedPatternRows()
        : &g_MenuOverlayPatternTable[(pattern - 1) * PATTERN_FRAME_SIZE];

    for (rowIndex = 0; rowIndex < PATTERN_VISIBLE_ROWS; rowIndex++) {
        const u8 row = rows[rowIndex];

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

    g_RenderState.packetCursor =
        QueueDrawModePrim(ot, RENDER_PRIM_CURSOR_AS(void), 0x39);
}
