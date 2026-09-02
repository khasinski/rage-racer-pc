#include "game/prim.h"
#include "game/render.h"

enum {
    NUMBER_TEXTURE_PAGE_BASE = 27,
    NUMBER_DIGIT_COUNT = 10,
};

s32 GameDrawNumber(s32 x, s16 y, s32 flags, u32 value, u8 red, u8 green,
                   u8 blue, u16 clut, u8 primitiveCount) {
    u8 digits[NUMBER_DIGIT_COUNT];
    GameOrderingTableEntry *ot = RENDER_OT_BASE +
        ((flags & DRAW_NUMBER_OVERLAY_LAYER) != 0);
    s32 digitWidth;
    s32 digitHeight;
    s32 textureV;
    s32 firstDigit;
    s32 fieldStart;
    s32 digitIndex;

    if ((flags & DRAW_NUMBER_ALT_DIGIT_ATLAS) != 0) {
        digitWidth = 8;
        digitHeight = 16;
        textureV = 0xDC;
    } else if ((flags & DRAW_NUMBER_LARGE_DIGITS) != 0) {
        digitWidth = 8;
        digitHeight = 16;
        textureV = 0x18;
    } else {
        digitWidth = 6;
        digitHeight = 12;
        textureV = 0;
    }

    for (digitIndex = NUMBER_DIGIT_COUNT - 1; digitIndex >= 0; digitIndex--) {
        digits[digitIndex] = value % 10;
        value /= 10;
    }

    firstDigit = 0;
    while (firstDigit < NUMBER_DIGIT_COUNT - 1 && digits[firstDigit] == 0) {
        firstDigit++;
    }

    fieldStart = (flags & DRAW_NUMBER_TEN_DIGIT_FIELD) != 0 ? 0 : firstDigit;
    x += (firstDigit - fieldStart) * digitWidth;
    for (digitIndex = firstDigit; digitIndex < NUMBER_DIGIT_COUNT;
         digitIndex++) {
        DrawSprite(ot, (s16)x, y, digitWidth, digitHeight,
                   digits[digitIndex] * digitWidth, textureV, red, green, blue,
                   clut, 0, 1, 0x80);
        x += digitWidth;
    }

    g_RenderState.packetCursor = QueueDrawModePrim(
        ot, RENDER_PRIM_CURSOR_AS(void),
        primitiveCount + NUMBER_TEXTURE_PAGE_BASE);
    return NUMBER_DIGIT_COUNT - firstDigit;
}
