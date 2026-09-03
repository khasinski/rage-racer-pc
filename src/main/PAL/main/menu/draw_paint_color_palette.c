#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/render.h"

#include <limits.h>

enum {
    PAINT_PALETTE_DRAW_START = 11,
    PAINT_PALETTE_LAST_FRAME = 10,
    PAINT_PALETTE_COMPLETE = 25,
    PAINT_PALETTE_X = 0x9E,
    PAINT_PALETTE_WIDE_OFFSET = 0x2C,
    PAINT_SWATCH_WIDTH = 8,
};

/* The 18-swatch PAINT COLOR strip with its selection frame and enlarged preview. */
s32 DrawPaintColorPalette(s32 *counter, s32 step, s32 index) {
    void *ot = RENDER_OT_BASE;
    s32 frame;
    s32 x;
    s32 y;
    s32 highlight;
    s32 colorIndex;

    if (counter == NULL) {
        return 0;
    }
    if ((u32)index >= MENU_PAINT_COLOR_COUNT) {
        index = 0;
    }

    if (step < 0) {
        int64_t updated = (int64_t)*counter + step;

        if (updated < 0) {
            *counter = 0;
        } else {
            *counter = updated > INT_MAX ? INT_MAX : (s32)updated;
        }
    }

    frame = *counter - PAINT_PALETTE_DRAW_START;
    if (frame >= 0) {
        if (frame > PAINT_PALETTE_LAST_FRAME) {
            frame = PAINT_PALETTE_LAST_FRAME;
        }

        x = PAINT_PALETTE_X;
        if (g_MenuAltLayout != 0) {
            x -= PAINT_PALETTE_WIDE_OFFSET;
        }
        y = 0x20B - frame * 15;

        highlight = rsin((g_PaintPalettePulsePhase * 2) & 0xFFF);
        if (highlight < 0) {
            highlight += 0x3F;
        }
        highlight = (highlight >> 6) - 0x41;

        g_PaintPalettePulsePhase =
            (s32)((u32)g_PaintPalettePulsePhase + 0x20u);

        DrawRectOutline(ot, x + index * PAINT_SWATCH_WIDTH - 2, y, 0xD,
                        0x1A, 0, (u8)highlight, 0, 0xFF);
        DrawSolidRect(ot, x + index * PAINT_SWATCH_WIDTH - 1, y + 2, 0xB,
                      0x16, g_PaintColorTable.colors[index].r,
                      g_PaintColorTable.colors[index].g,
                      g_PaintColorTable.colors[index].b, 0xFF);
        DrawRectOutline(ot, x, y + 3, 0x92, 0x14, 0xB4, 0xB4, 0xB4,
                        0xFF);

        for (colorIndex = 0; colorIndex < MENU_PAINT_COLOR_COUNT;
             colorIndex++) {
            const Rgb *color = &g_PaintColorTable.colors[colorIndex];

            DrawSolidRect(ot, x + 1 + colorIndex * PAINT_SWATCH_WIDTH, y + 5,
                          PAINT_SWATCH_WIDTH, 0x10, color->r, color->g,
                          color->b, 0xFF);
        }
    }

    if (step >= 0) {
        int64_t updated = (int64_t)*counter + step;

        if (updated >= PAINT_PALETTE_COMPLETE) {
            *counter = PAINT_PALETTE_COMPLETE;
            return 1;
        }
        *counter = updated < INT_MIN ? INT_MIN : (s32)updated;
    }

    return 0;
}
