#include "menu_label_font.h"

#include "game/frontend_internal.h"

#include <stdlib.h>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wdouble-promotion"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "fonts/archivo_black.ttf.inc"

enum { MENU_LABEL_COUNT = TITLE_MENU_ITEM_COUNT };

static const char *kLabels[MENU_LABEL_COUNT] = {
    "GRAND PRIX",
    "EXTRA GP",
    "TIME ATTACK",
    "CUSTOM",
    "SAVE&LOAD",
    "OPTION",
};

/* Cap height in the 16px bar, plus one pixel of drop shadow under the ink.
 * Zero tracking keeps the heavy face tight, the way the retail cells read. */
static const float kPixelHeight = 11.0f;
static const float kTracking = 0.0f;

static stbtt_fontinfo g_font;
static int g_fontState;
static uint8_t g_pixels[MENU_LABEL_COUNT][2][MENU_LABEL_WIDTH * MENU_LABEL_HEIGHT * 3];
static uint8_t g_ready[MENU_LABEL_COUNT][2];

static int FontReady(void) {
    int offset;

    if (g_fontState != 0) return g_fontState > 0;
    if (archivo_black_ttf_len < 1024) {
        g_fontState = -1;
        return 0;
    }
    offset = stbtt_GetFontOffsetForIndex(archivo_black_ttf, 0);
    if (offset < 0 ||
        !stbtt_InitFont(&g_font, archivo_black_ttf, offset)) {
        g_fontState = -1;
        return 0;
    }
    g_fontState = 1;
    return 1;
}

static float Advance(float scale, int codepoint) {
    int advance;
    int leftBearing;

    stbtt_GetCodepointHMetrics(&g_font, codepoint, &advance, &leftBearing);
    (void)leftBearing;
    return (float)advance * scale;
}

static float LineWidth(float scale, const char *text) {
    float width = 0.0f;
    const char *cursor;

    for (cursor = text; *cursor != '\0'; cursor++) {
        width += Advance(scale, (unsigned char)*cursor);
        if (cursor[1] != '\0') width += kTracking;
    }
    return width;
}

static void BlendPixel(uint8_t *pixel, int red, int green, int blue, int alpha) {
    pixel[0] = (uint8_t)((pixel[0] * (255 - alpha) + red * alpha) / 255);
    pixel[1] = (uint8_t)((pixel[1] * (255 - alpha) + green * alpha) / 255);
    pixel[2] = (uint8_t)((pixel[2] * (255 - alpha) + blue * alpha) / 255);
}

static void Stamp(uint8_t *image, float scale, float penX, int baseline,
                  int codepoint, int offsetX, int offsetY,
                  int red, int green, int blue) {
    int width = 0;
    int height = 0;
    int bitmapX = 0;
    int bitmapY = 0;
    int originX;
    int row;
    int column;
    float shift = penX - (float)(int)penX;
    unsigned char *bitmap = stbtt_GetCodepointBitmapSubpixel(
        &g_font, scale, scale, shift, 0.0f, codepoint,
        &width, &height, &bitmapX, &bitmapY);

    if (bitmap == NULL) return;
    originX = (int)penX + bitmapX + offsetX;
    for (row = 0; row < height; row++) {
        int y = baseline + bitmapY + row + offsetY;

        if (y < 0 || y >= MENU_LABEL_HEIGHT) continue;
        for (column = 0; column < width; column++) {
            int x = originX + column;
            int alpha = bitmap[row * width + column];

            if (x < 1 || x >= MENU_LABEL_WIDTH - 1 || alpha == 0) continue;
            BlendPixel(image + (y * MENU_LABEL_WIDTH + x) * 3,
                       red, green, blue, alpha);
        }
    }
    stbtt_FreeBitmap(bitmap, NULL);
}

static void Paint(int item, int selected) {
    uint8_t *image = g_pixels[item][selected];
    const char *text = kLabels[item];
    float scale = stbtt_ScaleForPixelHeight(&g_font, kPixelHeight);
    float pen;
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    int body;
    int top;
    int baseline;
    int barRed = selected ? 0xD0 : 0x78;
    int barGreen = selected ? 0x50 : 0x78;
    int barBlue = selected ? 0x48 : 0x78;
    int y;
    int x;
    const char *cursor;

    stbtt_GetFontVMetrics(&g_font, &ascent, &descent, &lineGap);
    body = (int)(((float)ascent - (float)descent) * scale + 0.5f);
    top = (MENU_LABEL_HEIGHT - body - 1) / 2;
    if (top < 0) top = 0;
    baseline = top + (int)((float)ascent * scale + 0.5f);

    for (y = 0; y < MENU_LABEL_HEIGHT; y++) {
        for (x = 0; x < MENU_LABEL_WIDTH; x++) {
            uint8_t *pixel = image + (y * MENU_LABEL_WIDTH + x) * 3;

            if (x == 0) {
                pixel[0] = (uint8_t)(selected ? 0xF8 : 0x98);
                pixel[1] = (uint8_t)(selected ? 0x70 : 0x98);
                pixel[2] = (uint8_t)(selected ? 0x70 : 0x98);
            } else if (x == MENU_LABEL_WIDTH - 1) {
                pixel[0] = (uint8_t)(selected ? 0x90 : 0x48);
                pixel[1] = (uint8_t)(selected ? 0x20 : 0x48);
                pixel[2] = (uint8_t)(selected ? 0x20 : 0x48);
            } else {
                pixel[0] = (uint8_t)barRed;
                pixel[1] = (uint8_t)barGreen;
                pixel[2] = (uint8_t)barBlue;
            }
        }
    }

    pen = ((float)MENU_LABEL_WIDTH - LineWidth(scale, text)) * 0.5f;
    for (cursor = text; *cursor != '\0'; cursor++) {
        int codepoint = (unsigned char)*cursor;

        Stamp(image, scale, pen + 1.0f, baseline, codepoint, 0, 1,
              selected ? 0x58 : 0x30, selected ? 0x08 : 0x30,
              selected ? 0x08 : 0x30);
        Stamp(image, scale, pen, baseline, codepoint, 0, 0,
              0x08, selected ? 0x00 : 0x08, selected ? 0x00 : 0x08);
        pen += Advance(scale, codepoint);
        if (cursor[1] != '\0') pen += kTracking;
    }
    g_ready[item][selected] = 1;
}

const uint8_t *MenuLabelFontPixels(int item, int selected) {
    if (item < 0 || item >= MENU_LABEL_COUNT) return NULL;
    if (!FontReady()) return NULL;
    selected = selected != 0;
    if (!g_ready[item][selected]) Paint(item, selected);
    return g_pixels[item][selected];
}
