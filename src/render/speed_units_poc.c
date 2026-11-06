#include "speed_units_poc.h"
#include <stdio.h>
#include <string.h>
#include "hud_unit_retail_data.h"

static int s_car = -1, s_requested, s_units, s_imperialSource, s_hdPatch;
static uint8_t s_target[RAGE_HUD_UNIT_BYTES];
static int s_left, s_top, s_right, s_bottom;

int RageSpeedUnitsRetailDiff(unsigned car, RageHudUnitDiff *out) {
    return car < 13 && RageHudUnitDiffDecode(out, s_retailDeltas[car].bytes, s_retailDeltas[car].size);
}
void RageSpeedUnitsConfigure(int car, const char *region, const char *requested) {
    s_car = -1; s_requested = s_units = s_hdPatch = 0;
    if (!region || (strcmp(region, "PAL") && strcmp(region, "NTSC-U")) ||
        car < 0 || car >= 13 || !requested) return;
    s_car = car;
    s_imperialSource = !strcmp(region, "NTSC-U");
    if (!strcmp(requested, "mph")) s_requested = RAGE_UNITS_MPH;
    if (!strcmp(requested, "kmh")) s_requested = RAGE_UNITS_KMH;
}
int RageSpeedUnitsOverrideRequested(void) { return s_requested != RAGE_UNITS_AUTO; }
int RageSpeedUnits(void) { return s_units; }
int RageSpeedUnitsPatchSnapshot(uint8_t snapshot[RAGE_HUD_UNIT_BYTES]) {
    RageHudUnitDiff diff;
    if (!s_requested || s_car < 0 || !RageSpeedUnitsRetailDiff((unsigned)s_car, &diff)) return 0;
    int result = RageHudUnitDiffApply(&diff, (unsigned)s_car, s_requested == RAGE_UNITS_MPH, snapshot);
    if (!result) { s_units = RAGE_UNITS_AUTO; s_hdPatch = 0; return 0; }
    s_units = s_requested;
    memcpy(s_target, snapshot, sizeof(s_target));
    s_left = 96; s_top = 94; s_right = s_bottom = -1;
    for (unsigned i = 0; i < RAGE_HUD_FACE_BYTES; ++i) for (unsigned n = 0; n < 2; ++n) {
        if (!((diff.delta[i] >> (n * 4u)) & 15u)) continue;
        int x = (int)((i % 48u) * 2u + n), y = (int)(i / 48u);
        if (x < s_left) s_left = x;
        if (x > s_right) s_right = x;
        if (y < s_top) s_top = y;
        if (y > s_bottom) s_bottom = y;
    }
    s_hdPatch = (s_requested == RAGE_UNITS_MPH) != s_imperialSource && s_right >= 0;
    return result;
}
static unsigned Pixel(int x, int y) {
    return (s_target[y * 48 + x / 2] >> ((x & 1) * 4)) & 15u;
}
int RageSpeedUnitsPatchRGBA(const char *key, uint8_t *pixels, unsigned w, unsigned h) {
    int car, consumed = 0;
    unsigned clut, scale, originY = 0;
    if (!s_hdPatch || !key || !pixels) return 1;
    unsigned page;
    int scene;
    /* Shared keys cannot bind a converted texture to a particular car. */
    if ((sscanf(key, "ui.hud.page.%u.clut.%u", &page, &clut) == 2 ||
        (sscanf(key, "ui.scene.%d.page.%u.clut.%u", &scene, &page, &clut) == 3 &&
         (scene == 12 || scene == 17))) && page == 10 &&
        (clut == 13224 || clut == 13288)) return 0;
    if (sscanf(key, "ui.hud.car.%d.page.10.clut.%u%n", &car, &clut, &consumed) != 2 ||
        !consumed || car != s_car || (clut != 13224 && clut != 13288)) return 1;
    if (!strcmp(key + consumed, ".region.0.112.96.94")) {
        if (w % 96 || h % 94 || w / 96 != h / 94) return 0;
        scale = w / 96;
    } else if (!key[consumed]) {
        if (w % 256 || h != w) return 0;
        scale = w / 256; originY = 112;
    } else return 0;
    if (!scale) return 0;
    unsigned palette = RAGE_HUD_FACE_BYTES + (clut == 13288 ? 32u : 0u);
    /* Pegase's metal panel has a four-pixel repeat, not a flat background. */
    unsigned backgroundX = s_car == 2 ? 62u : (unsigned)s_left - 2u;
    unsigned period = s_car == 2 ? 4u : 1u;
    /* HD antialiasing may extend beyond the changed native texels. Clear a
     * one-texel halo as well; otherwise the old italic strokes leave ghosts. */
    for (unsigned y = (unsigned)(s_top - 1) * scale; y < (unsigned)(s_bottom + 2) * scale; ++y)
        for (unsigned x = (unsigned)(s_left - 1) * scale; x < (unsigned)(s_right + 2) * scale; ++x) {
            unsigned bx = backgroundX * scale + (x - backgroundX * scale) % (period * scale);
            unsigned index = Pixel((int)(x / scale), (int)(y / scale));
            uint8_t *p = pixels + (((originY * scale) + y) * w + x) * 4u;
            const uint8_t *background = pixels + (((originY * scale) + y) * w + bx) * 4u;
            if ((int)(x / scale) < s_left || (int)(x / scale) > s_right ||
                (int)(y / scale) < s_top || (int)(y / scale) > s_bottom ||
                index == Pixel((int)(bx / scale), (int)(y / scale))) {
                memcpy(p, background, 3);
            } else {
                unsigned c = (unsigned)s_target[palette + index * 2u] |
                             (unsigned)s_target[palette + index * 2u + 1u] << 8;
                for (unsigned channel = 0; channel < 3; ++channel)
                    p[channel] = (uint8_t)(((c >> (channel * 5u)) & 31u) * 255u / 31u);
            }
        }
    return 1;
}
