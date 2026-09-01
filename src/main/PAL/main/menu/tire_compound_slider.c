#include "game/menu.h"

/* The five-position tire-compound slider of the CUSTOMIZE screen. */
void DrawTireCompoundSlider(u8 x, s32 useFlag) {
    OT_TYPE *ot;
    OT_TYPE *otBase;
    s32 gray;
    s32 alpha;
    s32 angle;
    s32 color;
    s32 xTest;
    s32 yLarge;
    s32 ySmall;

    otBase = RENDER_OT_BASE_AS(OT_TYPE);

    switch ((u8)x) {
    case 4:
        x = 0xB8;
        break;
    case 3:
        x = 0xC7;
        break;
    case 2:
        x = 0xD7;
        break;
    case 1:
        x = 0xE7;
        break;
    case 0:
        x = 0xF7;
        break;
    }

    if (useFlag != 0) {
        if (g_AnimTimer & 2) {
            alpha = 0xFF;
        } else {
            alpha = 0x60;
        }
    } else {
        angle = g_TireSliderPulsePhase;
        color = rsin(angle % 0x1000);
        if (color < 0) {
            color += 0x3F;
        }
        alpha = (color >> 6) - 0x41;
    }

    gray = 0xB4;
    ot = otBase + 2;

    DrawSprite(ot, 0xBC, 0x50, 0x14, 0x10, 0, 0xB4, 0, 0, 0, 0x244, 1, 1, 0x3A);
    DrawSprite(ot, 0xE0, 0x72, 0x14, 0x10, 0x14, 0xB4, 0, 0, 0, 0x244, 1, 1, 0x3A);

    xTest = (u8)x;
    if (xTest != 0xB8) {
        s32 green = (u8)alpha;

        DrawLine(ot, (s16)((u8)x - 1), 0x4C, (s16)((u8)x - 1), 0x84, 0, green, 0, 0xFF);
        DrawLine(ot, (s16)((u8)x - 3), 0x60, (s16)((u8)x - 3), 0x68, 0, green, 0, 0xFF);
        DrawLine(ot, (s16)((u8)x - 5), 0x60, (s16)((u8)x - 5), 0x68, 0, green, 0, 0xFF);
        DrawLine(ot, (s16)((u8)x - 7), 0x60, (s16)((u8)x - 7), 0x68, 0, green, 0, 0xFF);
        DrawFlatTriangle(ot, (s16)((u8)x - 13), 0x64, (s16)((u8)x - 8), 0x5E, (s16)((u8)x - 8), 0x6A,
                      0, green, 0, 0, 0x80);
    }

    if (xTest != 0xF7) {
        s32 green = (u8)alpha;

        DrawLine(ot, (s16)((u8)x + 1), 0x4C, (s16)((u8)x + 1), 0x84, 0, green, 0, 0xFF);
        DrawLine(ot, (s16)((u8)x + 3), 0x6A, (s16)((u8)x + 3), 0x72, 0, green, 0, 0xFF);
        DrawLine(ot, (s16)((u8)x + 5), 0x6A, (s16)((u8)x + 5), 0x72, 0, green, 0, 0xFF);
        DrawLine(ot, (s16)((u8)x + 7), 0x6A, (s16)((u8)x + 7), 0x72, 0, green, 0, 0xFF);
        DrawFlatTriangle(ot, (s16)((u8)x + 14), 0x6E, (s16)((u8)x + 9), 0x69, (s16)((u8)x + 9), 0x73,
                      0, green, 0, 0, 0x80);
    }

    DrawRectOutline(ot, 0xB8, 0x48, 0x40, 0x40, gray, gray, gray, 0xFF);
    yLarge = 0x85;
    ySmall = 0x60;
    DrawLine(ot, 0xC7, 0x4A, 0xC7, yLarge, gray, gray, gray, ySmall);
    DrawLine(ot, 0xD7, 0x4A, 0xD7, yLarge, gray, gray, gray, ySmall);
    DrawLine(ot, 0xE7, 0x4A, 0xE7, yLarge, gray, gray, gray, ySmall);

    DrawFlatTriangle(ot, 0xB9, 0x87, 0xF7, 0x48, 0xF7, 0x87, 0x1E, 0x8E, 0x95, 0, 0x80);
    DrawSolidRect(ot, 0xB8, 0x48, 0x40, 0x40, 0x95, 0x25, 0x1E, 0xFF);

    g_TireSliderPulsePhase += 0x60;
}
