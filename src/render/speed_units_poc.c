#include "speed_units_poc.h"
#include <stdio.h>
#include <string.h>

/* PAL artwork, measured in the 96x94 face, excluding digits and gear.
 * The two unlabeled racing dials deliberately remain unlabeled. */
static const RageSpeedUnitsLayout s_layouts[13] = {
    {65, 6, 12, 7, 2, 0, 64, 1, "km", "mi"},
    {68,77, 22, 7, 2, 0, 67, 1, "km/h", "mi/h"},
    {82,17, 10, 3, 0, 0, 62, 4, "km", "mi"},
    {68, 7, 22, 7, 2, 0, 67, 1, "km/h", "mi/h"},
    {68,10,  9, 4, 0, 0, 67, 1, "km", "mi"},
    {69, 7, 21, 7, 0, 1, 68, 1, "km/h", "mi/h"},
    {67,78, 22, 7, 2, 0, 66, 1, "km/h", "mi/h"},
    {68, 7, 22, 7, 2, 0, 67, 1, "km/h", "mi/h"},
    {69, 7, 12, 7, 2, 0, 68, 1, "km", "mi"},
    {0,0,0,0,0,0,0,0,"", ""},
    {0,0,0,0,0,0,0,0,"", ""},
    {68, 7, 22, 7, 2, 0, 67, 1, "km/h", "mi/h"},
    {55, 6, 22, 7, 2, 0, 54, 1, "km/h", "mi/h"}
};
static int s_car = -1, s_units;

void RageSpeedUnitsConfigure(int car, const char *region, const char *requested) {
    s_car = -1;
    s_units = RAGE_UNITS_AUTO;
    if (!region || strcmp(region, "PAL") || car < 0 || car >= 13 || !requested)
        return;
    s_car = car;
    if (!strcmp(requested, "mph")) s_units = RAGE_UNITS_MPH;
    if (!strcmp(requested, "kmh")) s_units = RAGE_UNITS_KMH;
}
int RageSpeedUnits(void) { return s_units; }
const RageSpeedUnitsLayout *RageSpeedUnitsLayoutForCar(int car) {
    return car >= 0 && car < 13 ? &s_layouts[car] : NULL;
}
static const RageSpeedUnitsLayout *ActivePatch(void) {
    const RageSpeedUnitsLayout *layout = RageSpeedUnitsLayoutForCar(s_car);
    return s_units == RAGE_UNITS_MPH && layout && layout->width ? layout : NULL;
}

/* Individual letter masks keep the original height, italic/bold treatment
 * and unit spelling per dial. Slashless "kmh" layouts can use "mph". */
static const uint8_t *Glyph(char c, int *width) {
    static const uint8_t m[7] = {0,0,26,21,21,21,21};
    static const uint8_t i[7] = {1,0,1,1,1,1,1};
    static const uint8_t slash[7] = {1,1,2,2,2,4,4};
    static const uint8_t h[7] = {8,8,14,9,9,9,9};
    static const uint8_t p[7] = {0,0,14,9,14,8,8};
    switch (c) {
        case 'm': *width = 5; return m;
        case 'i': *width = 1; return i;
        case '/': *width = 3; return slash;
        case 'h': *width = 4; return h;
        case 'p': *width = 4; return p;
        default: *width = 0; return NULL;
    }
}
static int Ink(const RageSpeedUnitsLayout *layout, int x, int y) {
    const char *text = layout->imperial;
    int total = -1, sourceY = layout->height == 3 ? 2 + y * 2 :
        6 - (layout->height - 1 - y) * 6 / (layout->height - 1);
    for (const char *c = text; *c; ++c) {
        int width;
        (void)Glyph(*c, &width);
        total += width + 1;
    }
    total += layout->slant + layout->bold + 1; /* trailing shadow */
    x -= layout->width - total + layout->slant * (6 - sourceY) / 6;
    for (const char *c = text; *c; ++c) {
        int width;
        const uint8_t *glyph = Glyph(*c, &width);
        if (x >= 0 && x < width && ((glyph[sourceY] >> (width - 1 - x)) & 1)) return 1;
        if (layout->bold && *c == 'h' && x > 0 && x <= width &&
            ((glyph[sourceY] >> (width - x)) & 1)) return 1;
        x -= width + 1;
    }
    return 0;
}
static unsigned Index(const uint16_t *face, int x, int y) {
    return (face[y * 24 + x / 4] >> ((x & 3) * 4)) & 15u;
}
void RageSpeedUnitsPatchIndexed(uint16_t face[24 * 94], const uint16_t palette[16]) {
    const RageSpeedUnitsLayout *l = ActivePatch();
    unsigned foreground = 0, brightness = 0, shadow = 0, shadowDistance = ~0u;
    if (!l) return;
    for (int y = l->y; y < l->y + l->height; ++y)
        for (int x = l->x; x < l->x + l->width; ++x) {
            unsigned i = Index(face, x, y), c = palette[i];
            unsigned b = (c & 31u) + ((c >> 5) & 31u) + ((c >> 10) & 31u);
            if (b > brightness) { brightness = b; foreground = i; }
        }
    /* A palette tone halfway to the background gives the retail 1px edge. */
    unsigned bg = palette[Index(face, l->backgroundX, l->y)];
    unsigned inkColor = palette[foreground];
    for (unsigned i = 1; i < 16; ++i) {
        unsigned distance = 0;
        for (unsigned channel = 0; channel < 3; ++channel) {
            unsigned shift = channel * 5u;
            int target = (int)(((inkColor >> shift) & 31u) + ((bg >> shift) & 31u)) / 2;
            int delta = (int)((palette[i] >> shift) & 31u) - target;
            distance += (unsigned)(delta * delta);
        }
        if (distance < shadowDistance) { shadowDistance = distance; shadow = i; }
    }
    for (int y = 0; y < l->height; ++y) for (int x = 0; x < l->width; ++x) {
        unsigned background = Index(face, l->backgroundX +
            (l->x + x - l->backgroundX) % l->backgroundPeriod, l->y + y);
        unsigned i = Ink(l, x, y) ? foreground : Ink(l, x - 1, y) ? shadow : background;
        int px = l->x + x;
        unsigned shift = (unsigned)(px & 3) * 4u;
        uint16_t *word = &face[(l->y + y) * 24 + px / 4];
        *word = (uint16_t)((*word & ~(15u << shift)) | (i << shift));
    }
}
void RageSpeedUnitsPatchRGBA(const char *key, uint8_t *pixels, unsigned w, unsigned h) {
    const RageSpeedUnitsLayout *l = ActivePatch();
    int car, consumed = 0;
    unsigned clut, scale, originY = 0, brightness = 0;
    uint8_t foreground[3] = {0};
    if (!l || !key || !pixels) return;
    if (sscanf(key, "ui.hud.car.%d.page.10.clut.%u%n", &car, &clut, &consumed) != 2 ||
        !consumed || car != s_car || (clut != 13224 && clut != 13288)) return;
    if (!strcmp(key + consumed, ".region.0.112.96.94")) {
        if (w % 96 || h % 94 || w / 96 != h / 94) return;
        scale = w / 96;
    } else if (!key[consumed]) {
        if (w % 256 || h != w) return;
        scale = w / 256;
        originY = 112;
    } else return;
    if (!scale) return;
    unsigned left = (unsigned)l->x * scale, y0 = (originY + (unsigned)l->y) * scale;
    for (unsigned y = 0; y < (unsigned)l->height * scale; ++y)
        for (unsigned x = 0; x < (unsigned)l->width * scale; ++x) {
            const uint8_t *p = pixels + ((y0 + y) * w + left + x) * 4u;
            unsigned b = (unsigned)p[0] + p[1] + p[2];
            if (b > brightness) { brightness = b; memcpy(foreground, p, 3); }
        }
    for (unsigned y = 0; y < (unsigned)l->height * scale; ++y)
        for (unsigned x = 0; x < (unsigned)l->width * scale; ++x) {
            unsigned backgroundX = (unsigned)l->backgroundX * scale +
                (left + x - (unsigned)l->backgroundX * scale) % ((unsigned)l->backgroundPeriod * scale);
            const uint8_t *background = pixels + ((y0 + y) * w + backgroundX) * 4u;
            uint8_t *p = pixels + ((y0 + y) * w + left + x) * 4u;
            int gx = (int)(x / scale), gy = (int)(y / scale);
            int ink = Ink(l, gx, gy), edge = !ink && Ink(l, gx - 1, gy);
            /* Preserve alpha and the pack's day/night foreground colour. */
            for (int c = 0; c < 3; ++c)
                p[c] = ink ? foreground[c] : edge ?
                    (uint8_t)(((unsigned)foreground[c] + background[c]) / 2u) : background[c];
        }
}
