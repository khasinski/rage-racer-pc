#include "render/hud_unit_diff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t vram[1024 * 512 * 2];
static int Read(const char *path, uint8_t snapshot[RAGE_HUD_UNIT_BYTES]) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int ok = fread(vram, 1, sizeof(vram), f) == sizeof(vram) && fgetc(f) == EOF;
    if (fclose(f)) ok = 0;
    return ok && RageHudUnitSnapshot(vram, sizeof(vram), snapshot);
}
int main(int argc, char **argv) {
    char *end;
    if (argc != 5) {
        fprintf(stderr, "usage: %s CAR METRIC.vram NTSC-U.vram OUTPUT.rrhud\n", argv[0]);
        return 2;
    }
    long car = strtol(argv[1], &end, 10);
    if (!*argv[1] || *end || car < 0 || car > 12) return 2;
    uint8_t metric[RAGE_HUD_UNIT_BYTES], imperial[RAGE_HUD_UNIT_BYTES];
    uint8_t encoded[RAGE_HUD_DIFF_MAX_BYTES], check[RAGE_HUD_UNIT_BYTES];
    RageHudUnitDiff diff;
    if (!Read(argv[2], metric) || !Read(argv[3], imperial) ||
        !RageHudUnitDiffBuild(&diff, (unsigned)car, metric, imperial)) {
        fprintf(stderr, "cannot read paired raw 1024x512x16 VRAM captures\n");
        return 1;
    }
    memcpy(check, metric, sizeof(check));
    if (!RageHudUnitDiffApply(&diff, (unsigned)car, 1, check) || memcmp(check, imperial, sizeof(check)) ||
        !RageHudUnitDiffApply(&diff, (unsigned)car, 0, check) || memcmp(check, metric, sizeof(check))) return 1;
    size_t size = RageHudUnitDiffEncode(&diff, encoded, sizeof(encoded));
    if (!size) return 1;
    FILE *f = fopen(argv[4], "wb");
    if (!f) return 1;
    int ok = fwrite(encoded, 1, size, f) == size;
    if (fclose(f)) ok = 0;
    unsigned pixels = 0, paletteBytes = 0;
    int left = 96, top = 94, right = -1, bottom = -1;
    for (unsigned i = 0; i < RAGE_HUD_FACE_BYTES; ++i) for (unsigned nibble = 0; nibble < 2; ++nibble) {
        if (!((diff.delta[i] >> (4u * nibble)) & 15u)) continue;
        int x = (int)((i % 48u) * 2u + nibble), y = (int)(i / 48u);
        if (x < left) left = x;
        if (x > right) right = x;
        if (y < top) top = y;
        if (y > bottom) bottom = y;
        ++pixels;
    }
    for (unsigned i = RAGE_HUD_FACE_BYTES; i < RAGE_HUD_UNIT_BYTES; ++i) paletteBytes += diff.delta[i] != 0;
    printf("car=%ld changed_pixels=%u bounds=[%d,%d,%d,%d] palette_bytes=%u patch_bytes=%zu\n",
        car, pixels, pixels ? left : 0, pixels ? top : 0,
        pixels ? right-left+1 : 0, pixels ? bottom-top+1 : 0, paletteBytes, size);
    printf("metric_sha256=%s\nimperial_sha256=%s\n", diff.metricHash, diff.imperialHash);
    return ok ? 0 : 1;
}
