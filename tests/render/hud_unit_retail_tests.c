#include "render/speed_units_poc.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d car %u: %s\n",__LINE__,car,#x); return 1; } } while (0)
static uint8_t vram[1024 * 512 * 2], rgba[384 * 376 * 4], before[sizeof(rgba)];
static int Read(const char *root, unsigned car, uint8_t *snapshot) {
    char path[4096];
    int n = snprintf(path, sizeof(path), "%s/car-%u.vram", root, car);
    if (n < 0 || n >= (int)sizeof(path)) return 0;
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int ok = fread(vram, 1, sizeof(vram), f) == sizeof(vram) && fgetc(f) == EOF;
    if (fclose(f)) ok = 0;
    return ok && RageHudUnitSnapshot(vram, sizeof(vram), snapshot);
}
int main(int argc, char **argv) {
    const unsigned expectedPixels[13] = {52,106,35,120,41,105,134,102,55,0,0,99,99};
    for (unsigned car = 0; car < 13; ++car) {
        RageHudUnitDiff diff;
        CHECK(RageSpeedUnitsRetailDiff(car, &diff));
        CHECK(diff.car == car);
        unsigned changed = 0;
        int left = 96, top = 94, right = -1, bottom = -1;
        for (unsigned i = 0; i < RAGE_HUD_FACE_BYTES; ++i) for (unsigned n = 0; n < 2; ++n) {
            if (!((diff.delta[i] >> (4u * n)) & 15u)) continue;
            int x = (int)((i % 48u) * 2u + n), y = (int)(i / 48u);
            ++changed;
            if (x < left) left = x;
            if (x > right) right = x;
            if (y < top) top = y;
            if (y > bottom) bottom = y;
        }
        CHECK(changed == expectedPixels[car]);
        for (unsigned i = RAGE_HUD_FACE_BYTES; i < RAGE_HUD_UNIT_BYTES; ++i) CHECK(!diff.delta[i]);
        if (argc != 3) continue;
        uint8_t metric[RAGE_HUD_UNIT_BYTES], imperial[RAGE_HUD_UNIT_BYTES], snapshot[RAGE_HUD_UNIT_BYTES];
        CHECK(Read(argv[1], car, metric) && Read(argv[2], car, imperial));
        for (int sourceUS = 0; sourceUS < 2; ++sourceUS) for (int targetUS = 0; targetUS < 2; ++targetUS) {
            const uint8_t *source = sourceUS ? imperial : metric, *target = targetUS ? imperial : metric;
            memcpy(snapshot, source, sizeof(snapshot));
            RageSpeedUnitsConfigure((int)car, sourceUS ? "NTSC-U" : "PAL", targetUS ? "mph" : "kmh");
            CHECK(RageSpeedUnits() == RAGE_UNITS_AUTO);
            CHECK(RageSpeedUnitsPatchSnapshot(snapshot));
            CHECK(!memcmp(snapshot, target, sizeof(snapshot)));
            CHECK(RageSpeedUnits() == (targetUS ? RAGE_UNITS_MPH : RAGE_UNITS_KMH));
            CHECK(RageSpeedUnitsPatchSnapshot(snapshot) == 1);
            for (int full = 0; full < 2; ++full) for (int night = 0; night < 2; ++night) {
                int w = full ? 256 : 384, h = full ? 256 : 376, scale = full ? 1 : 4;
                int originY = full ? 112 : 0;
                size_t size = (size_t)w * (size_t)h * 4u;
                char key[128];
                snprintf(key, sizeof(key), "ui.hud.car.%u.page.10.clut.%d%s",car,
                    night ? 13288 : 13224, full ? "" : ".region.0.112.96.94");
                for (size_t i = 0; i < size; ++i) rgba[i] = (uint8_t)(17u + i % 19u);
                memcpy(before, rgba, size);
                CHECK(RageSpeedUnitsPatchRGBA(key, rgba, (unsigned)w, (unsigned)h));
                CHECK((memcmp(before, rgba, size) != 0) == (sourceUS != targetUS && changed != 0));
                for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
                    int at = (y*w+x)*4;
                    CHECK(rgba[at+3] == before[at+3]);
                    if (x >= (left-1)*scale && x < (right+2)*scale &&
                        y >= (originY+top-1)*scale && y < (originY+bottom+2)*scale) continue;
                    CHECK(!memcmp(rgba+at,before+at,4));
                }
            }
            if (sourceUS != targetUS && changed) {
                char key[128];
                snprintf(key,sizeof(key),"ui.hud.car.%u.page.10.clut.13224.region.0.112.96.94",car);
                memcpy(before,rgba,sizeof(rgba));
                CHECK(!RageSpeedUnitsPatchRGBA(key,rgba,383,376));
                CHECK(!RageSpeedUnitsPatchRGBA("ui.hud.page.10.clut.13224",rgba,256,256));
                CHECK(!RageSpeedUnitsPatchRGBA("ui.scene.12.page.10.clut.13224",rgba,256,256));
                CHECK(!memcmp(before,rgba,sizeof(rgba)));
            }
            /* A modified source must neither receive a partial XOR nor change digits. */
            memcpy(snapshot, source, sizeof(snapshot)); snapshot[500] ^= 1;
            uint8_t saved[sizeof(snapshot)]; memcpy(saved, snapshot, sizeof(saved));
            RageSpeedUnitsConfigure((int)car, sourceUS ? "NTSC-U" : "PAL", targetUS ? "mph" : "kmh");
            CHECK(!RageSpeedUnitsPatchSnapshot(snapshot));
            CHECK(!memcmp(snapshot,saved,sizeof(saved)) && RageSpeedUnits() == RAGE_UNITS_AUTO);
        }
    }
    puts(argc == 3 ? "retail references: all 13 cars exact in both directions; HD boundaries/alpha and rejection verified" :
        "retail manifest: 13 verified deltas, exact changed-pixel counts, identical day/night palettes");
    return 0;
}
