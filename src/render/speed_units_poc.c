#include "speed_units_poc.h"
#include <stdio.h>
#include <string.h>

static int s_car = -1, s_units;
static const uint8_t s_mph[7][15] = {
    {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,1,0,0,0},
    {1,1,0,1,0,0,1,1,1,0,0,1,1,1,0},
    {1,0,1,0,1,0,1,0,0,1,0,1,0,0,1},
    {1,0,1,0,1,0,1,1,1,0,0,1,0,0,1},
    {1,0,1,0,1,0,1,0,0,0,0,1,0,0,1},
    {1,0,1,0,1,0,1,0,0,0,0,1,0,0,1}
};

void RageSpeedUnitsConfigure(int car, const char *region, const char *requested) {
    s_car = car;
    s_units = RAGE_UNITS_AUTO;
    if (!region || strcmp(region, "PAL") || (car != 0 && car != 3) || !requested)
        return;
    if (!strcmp(requested, "mph")) s_units = RAGE_UNITS_MPH;
    if (!strcmp(requested, "kmh")) s_units = RAGE_UNITS_KMH;
}
int RageSpeedUnits(void) { return s_units; }
static int LabelY(void) { return s_car == 0 ? 6 : 7; }
static int LabelWidth(void) { return s_car == 0 ? 15 : 26; }
static unsigned Index(const uint16_t *face, int x, int y) {
    return (face[y * 24 + x / 4] >> ((x & 3) * 4)) & 15u;
}
void RageSpeedUnitsPatchIndexed(uint16_t face[24 * 94], const uint16_t palette[16]) {
    unsigned foreground = 0, brightness = 0;
    int width = LabelWidth(), top = LabelY();
    if (s_units != RAGE_UNITS_MPH) return;
    for (int y = top; y < top + 7; ++y) for (int x = 65; x < 65 + width; ++x) {
        unsigned i = Index(face, x, y), c = palette[i];
        unsigned b = (c & 31u) + ((c >> 5) & 31u) + ((c >> 10) & 31u);
        if (b > brightness) { brightness = b; foreground = i; }
    }
    for (int y = 0; y < 7; ++y) {
        unsigned background = Index(face, 64, top + y);
        for (int x = 0; x < width; ++x) {
            int glyphX = x - (width - 15), px = 65 + x;
            unsigned i = glyphX >= 0 && s_mph[y][glyphX] ? foreground : background;
            unsigned shift = (unsigned)(px & 3) * 4u;
            uint16_t *word = &face[(top + y) * 24 + px / 4];
            *word = (uint16_t)((*word & ~(15u << shift)) | (i << shift));
        }
    }
}
void RageSpeedUnitsPatchRGBA(const char *key, uint8_t *pixels, unsigned w, unsigned h) {
    char prefix[80];
    unsigned clut, scale, originY = 0, brightness = 0;
    uint8_t foreground[4] = {0};
    int consumed = 0, width = LabelWidth(), top = LabelY();
    if (s_units != RAGE_UNITS_MPH || !key || !pixels) return;
    snprintf(prefix, sizeof(prefix), "ui.hud.car.%d.page.10.clut.%%u%%n", s_car);
    if (sscanf(key, prefix, &clut, &consumed) != 1 || !consumed ||
        (clut != 13224 && clut != 13288)) return;
    if (!strcmp(key + consumed, ".region.0.112.96.94")) {
        if (w % 96 || h % 94 || w / 96 != h / 94) return;
        scale = w / 96;
    } else if (!key[consumed]) {
        if (w % 256 || h != w) return;
        scale = w / 256;
        originY = 112;
    } else return;
    if (!scale) return;
    unsigned left = 65u * scale, y0 = (originY + (unsigned)top) * scale;
    for (unsigned y = 0; y < 7u * scale; ++y)
        for (unsigned x = 0; x < (unsigned)width * scale; ++x) {
            const uint8_t *p = pixels + ((y0 + y) * w + left + x) * 4u;
            unsigned b = (unsigned)p[0] + p[1] + p[2];
            if (b > brightness) { brightness = b; memcpy(foreground, p, 4); }
        }
    for (unsigned y = 0; y < 7u * scale; ++y) {
        uint8_t background[4];
        memcpy(background, pixels + ((y0 + y) * w + left - 1u) * 4u, 4);
        for (unsigned x = 0; x < (unsigned)width * scale; ++x) {
            int gx = (int)(x / scale) - (width - 15);
            uint8_t *p = pixels + ((y0 + y) * w + left + x) * 4u;
            /* Keep the pack's transparency and day/night foreground colour. */
            memcpy(p, gx >= 0 && s_mph[y / scale][gx] ? foreground : background, 3);
        }
    }
}
