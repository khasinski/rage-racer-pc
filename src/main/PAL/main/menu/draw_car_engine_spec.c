#include "game/asset.h"
#include "game/menu.h"
#include "game/menu_internal.h"

#include <stdio.h>

enum {
    ENGINE_SPEC_TEXT_CAPACITY = 16,
    SMALL_TEXT_ADVANCE = 6
};

static s16 EngineSpecY(s32 base, s32 slide) {
    int64_t y = (int64_t)base - slide;

    if (y < INT16_MIN) {
        return INT16_MIN;
    }
    if (y > INT16_MAX) {
        return INT16_MAX;
    }
    return (s16)y;
}

static s32 DrawEngineSpecNumber(s32 x, s16 y, char *text, s32 value,
                                u8 brightness) {
    s32 length = snprintf(text, ENGINE_SPEC_TEXT_CAPACITY, g_FormatDecimal,
                          value);

    DrawSmallText(x, y, text, brightness, brightness, brightness, 0x244, 0x20);
    return x + length * SMALL_TEXT_ADVANCE;
}

void DrawCarEngineSpec(s32 slide, s32 brightness) {
    GameOrderingTableEntry *ot;
    char text[ENGINE_SPEC_TEXT_CAPACITY];
    u8 intensity;
    s16 powerY;
    s16 torqueY;
    s32 cursorX;

    if (g_MenuAltLayout != 0 || g_CarModelAsset == NULL ||
        RENDER_OT_BASE == NULL) {
        return;
    }
    ot = RENDER_OT_BASE + 1;
    intensity = (u8)AddClampedMenuValue(brightness, 0, 0, UINT8_MAX);
    powerY = EngineSpecY(0xCC, slide);
    torqueY = EngineSpecY(0xDA, slide);

    DrawSprite(ot, 0xA1, powerY, 0x10, 0xC, 0, 0xF4, intensity, intensity,
               intensity, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, 0xB2, powerY, 0x1C, 0xC, 0x10, 0xF4, intensity, intensity,
               intensity, 0x244, 0, 1, 0x3A);
    cursorX = DrawEngineSpecNumber(0xD2, powerY - 1, text,
                                   g_CarModelAsset->maxPower, intensity);
    DrawSprite(ot, cursorX + 2, powerY, 8, 0xC, 0x70, 0xF4, intensity,
               intensity, intensity, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, cursorX + 13, powerY, 6, 0xC, 0xD8, 0, intensity,
               intensity, intensity, 0x244, 0, 1, 0x3B);
    cursorX = DrawEngineSpecNumber(cursorX + 20, powerY - 1, text,
                                   g_CarModelAsset->maxPowerRpm, intensity);
    DrawSprite(ot, cursorX + 2, powerY, 0x10, 0xC, 0x78, 0xF4, intensity,
               intensity, intensity, 0x244, 0, 1, 0x3A);

    DrawSprite(ot, 0xA1, torqueY, 0x10, 0xC, 0, 0xF4, intensity, intensity,
               intensity, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, 0xB2, torqueY, 0x20, 0xC, 0x2C, 0xF4, intensity, intensity,
               intensity, 0x244, 0, 1, 0x3A);
    cursorX = DrawEngineSpecNumber(0xD2, torqueY - 1, text,
                                   g_CarModelAsset->maxTorqueWhole, intensity);
    DrawSprite(ot, cursorX + 1, torqueY, 3, 0xC, 0xE0, 0, intensity,
               intensity, intensity, 0x244, 0, 1, 0x3B);
    cursorX = DrawEngineSpecNumber(cursorX + 3, torqueY - 1, text,
                                   g_CarModelAsset->maxTorqueFraction,
                                   intensity);
    DrawSprite(ot, cursorX + 2, torqueY, 0x10, 0xC, 0x88, 0xF4, intensity,
               intensity, intensity, 0x244, 0, 1, 0x3A);
    DrawSprite(ot, cursorX + 19, torqueY, 6, 0xC, 0xD8, 0, intensity,
               intensity, intensity, 0x244, 0, 1, 0x3B);
    cursorX = DrawEngineSpecNumber(cursorX + 26, torqueY - 1, text,
                                   g_CarModelAsset->maxTorqueRpm, intensity);
    DrawSprite(ot, cursorX + 2, torqueY, 0x10, 0xC, 0x78, 0xF4, intensity,
               intensity, intensity, 0x244, 0, 1, 0x3A);
}
