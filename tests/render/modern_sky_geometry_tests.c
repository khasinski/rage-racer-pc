#include "modern_sky_geometry.h"
#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main(void) {
    /* Old UVs must move one pixel left, not 63 pixels right, as the
     * geometric origin wraps to the next 64-pixel tile. Test both directions,
     * every tile boundary, fractional presentation and the 31/0 wrap. */
    for (int tile = 0; tile < 32; ++tile) {
        for (int direction = -1; direction <= 1; direction += 2) {
            for (int step = 0; step <= 8; ++step) {
                float t = step / 8.0f;
                float target = tile + direction * t;
                float origin = direction * 63.0f * t;
                float x = origin + 64.0f * ModernSkySourceColumn(tile, target);
                CHECK(fabsf(x + direction * t) < 0.0001f);
                CHECK(fabsf(ModernSkySourceColumn(tile, target + 32) -
                             ModernSkySourceColumn(tile, target)) < 0.0001f);
            }
        }
    }
    for (int roll = -45; roll <= 45; ++roll) {
        float a = roll * 0.01745329252f, c = cosf(a), s = sinf(a);
        for (int wide = 0; wide < 3; ++wide) {
            float width = wide == 0 ? 320 : wide == 1 ? 426.6667f : 960;
            float x[4], y[4];
            for (int i = 0; i < 4; ++i) {
                float u = (i & 1) ? 256 : -256;
                float v = i < 2 ? -128 : 128;
                x[i] = 160 + c*u - s*v;
                y[i] = 120 + s*u + c*v;
            }
            ModernSkyExtendBand(x, y, width);
            for (int i = 0; i < 4; ++i) {
                float u = c*(x[i]-160) + s*(y[i]-120);
                float v = -s*(x[i]-160) + c*(y[i]-120);
                CHECK(fabsf(v - (i < 2 ? -128 : 128)) < 0.001f);
                CHECK((i & 1) ? u > hypotf(width,240) : u < -hypotf(width,240));
            }
        }
    }
    return 0;
}
