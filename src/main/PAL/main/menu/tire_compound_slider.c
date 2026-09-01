#include "game/menu.h"

enum {
    TIRE_COMPOUND_COUNT = 5,
    TIRE_SLIDER_PULSE_STEP = 0x60
};

static const s16 s_tireCompoundX[TIRE_COMPOUND_COUNT] = {
    0xF7, 0xE7, 0xD7, 0xC7, 0xB8,
};

static u8 GetSliderHighlight(s32 confirming) {
    if (confirming != 0) {
        return g_AnimTimer & 2 ? 0xFF : 0x60;
    }
    return (u8)(rsin(g_TireSliderPulsePhase % 0x1000) / 64 - 0x41);
}

static void DrawLeftSliderArrow(void *ot, s16 x, u8 green) {
    DrawLine(ot, x - 1, 0x4C, x - 1, 0x84, 0, green, 0, 0xFF);
    DrawLine(ot, x - 3, 0x60, x - 3, 0x68, 0, green, 0, 0xFF);
    DrawLine(ot, x - 5, 0x60, x - 5, 0x68, 0, green, 0, 0xFF);
    DrawLine(ot, x - 7, 0x60, x - 7, 0x68, 0, green, 0, 0xFF);
    DrawFlatTriangle(ot, x - 13, 0x64, x - 8, 0x5E, x - 8, 0x6A, 0,
                     green, 0, 0, 0x80);
}

static void DrawRightSliderArrow(void *ot, s16 x, u8 green) {
    DrawLine(ot, x + 1, 0x4C, x + 1, 0x84, 0, green, 0, 0xFF);
    DrawLine(ot, x + 3, 0x6A, x + 3, 0x72, 0, green, 0, 0xFF);
    DrawLine(ot, x + 5, 0x6A, x + 5, 0x72, 0, green, 0, 0xFF);
    DrawLine(ot, x + 7, 0x6A, x + 7, 0x72, 0, green, 0, 0xFF);
    DrawFlatTriangle(ot, x + 14, 0x6E, x + 9, 0x69, x + 9, 0x73, 0,
                     green, 0, 0, 0x80);
}

/* The five-position tire-compound slider of the CUSTOMIZE screen. */
void DrawTireCompoundSlider(u8 compound, s32 confirming) {
    void *ot = RENDER_OT_BASE_AS(OT_TYPE) + 2;
    s16 x = compound < TIRE_COMPOUND_COUNT ? s_tireCompoundX[compound]
                                           : compound;
    u8 highlight = GetSliderHighlight(confirming);
    const u8 frameColor = 0xB4;

    DrawSprite(ot, 0xBC, 0x50, 0x14, 0x10, 0, 0xB4, 0, 0, 0, 0x244, 1, 1,
               0x3A);
    DrawSprite(ot, 0xE0, 0x72, 0x14, 0x10, 0x14, 0xB4, 0, 0, 0, 0x244, 1,
               1, 0x3A);

    if (x != s_tireCompoundX[4]) {
        DrawLeftSliderArrow(ot, x, highlight);
    }
    if (x != s_tireCompoundX[0]) {
        DrawRightSliderArrow(ot, x, highlight);
    }

    DrawRectOutline(ot, 0xB8, 0x48, 0x40, 0x40, frameColor, frameColor,
                    frameColor, 0xFF);
    DrawLine(ot, 0xC7, 0x4A, 0xC7, 0x85, frameColor, frameColor, frameColor,
             0x60);
    DrawLine(ot, 0xD7, 0x4A, 0xD7, 0x85, frameColor, frameColor, frameColor,
             0x60);
    DrawLine(ot, 0xE7, 0x4A, 0xE7, 0x85, frameColor, frameColor, frameColor,
             0x60);
    DrawFlatTriangle(ot, 0xB9, 0x87, 0xF7, 0x48, 0xF7, 0x87, 0x1E, 0x8E,
                     0x95, 0, 0x80);
    DrawSolidRect(ot, 0xB8, 0x48, 0x40, 0x40, 0x95, 0x25, 0x1E, 0xFF);

    g_TireSliderPulsePhase += TIRE_SLIDER_PULSE_STEP;
}
