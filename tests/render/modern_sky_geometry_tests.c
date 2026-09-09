#include "modern_sky_geometry.h"
#include "modern_sky_reprojection.h"
#include <stdio.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)

int main(void) {
    RageRenderCamera source = {0};
    RageRenderCamera target = {0};
    RageCapturePacket packet = {0};
    float x[4], y[4];

    source.skyGridOrigin = (RageRenderVec3){10, 20, 0};
    source.skyGridColumn = (RageRenderVec3){4, 0, 3};
    source.skyGridRow = (RageRenderVec3){0, 5, 0};
    target.skyGridOrigin = (RageRenderVec3){100, 200, 0};
    target.skyGridColumn = (RageRenderVec3){8, 0, 4};
    target.skyGridRow = (RageRenderVec3){0, 10, 0};
    x[0] = 18;
    y[0] = 35;
    ModernSkyReprojectPoint(&source, &target, &x[0], &y[0]);
    CHECK(fabsf(x[0] - 116) < 0.0001f && fabsf(y[0] - 230) < 0.0001f);
    source.skyGridColumn.y = 0;
    source.skyGridRow.y = 0;
    x[0] = 7;
    y[0] = 9;
    ModernSkyReprojectPoint(&source, &target, &x[0], &y[0]);
    CHECK(x[0] == 7 && y[0] == 9);
    source.skyGridColumn = (RageRenderVec3){4, 0, 31};
    target.skyGridColumn = (RageRenderVec3){8, 0, 0};
    target.skyGridRow = (RageRenderVec3){0, 10, 7};
    target.skyCloudRow = 1;
    packet.skyIndex = 25;
    CHECK(ModernSkyBuildSmoothQuad(&packet, &source, &target, x, y));
    CHECK(fabsf(x[0] - 36) < 0.0001f && fabsf(y[0] - 190) < 0.0001f);
    packet.skyIndex = 96;
    CHECK(!ModernSkyBuildSmoothQuad(&packet, &source, &target, x, y));

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
