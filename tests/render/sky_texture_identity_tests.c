#include "render/sky_layout.h"
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { ++failures; \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); } } while (0)

int main(void) {
    RageSkyTextureIdentity source = {0}, cached, changed;
    source.assetKey = 91;
    source.cloudRow = 1;
    source.hasLayout = 1;
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 8; ++column)
            source.layout.tiles[row][column] = (uint8_t)((row + column) % 8);
    cached = source;
    CHECK(RenderSkyTextureIdentityEqual(&cached, &source));
    for (unsigned row = 0; row < 2; ++row) {
        for (unsigned column = 0; column < 8; ++column) {
            changed = source;
            changed.layout.tiles[row][column] ^= 1;
            CHECK(!RenderSkyTextureIdentityEqual(&cached, &changed));
            CHECK(!RenderSkyTextureIdentityEqual(&changed, &cached));
            CHECK(RenderSkyTextureIdentityEqual(&cached, &source));
        }
    }
    changed = source;
    ++changed.assetKey;
    CHECK(!RenderSkyTextureIdentityEqual(&cached, &changed));
    changed = source;
    ++changed.cloudRow;
    CHECK(!RenderSkyTextureIdentityEqual(&cached, &changed));
    changed = source;
    changed.hasLayout = 0;
    CHECK(!RenderSkyTextureIdentityEqual(&cached, &changed));
    cached.hasLayout = 0;
    memset(changed.layout.tiles, 0xFF, sizeof(changed.layout.tiles));
    CHECK(RenderSkyTextureIdentityEqual(&cached, &changed));
    ++changed.cloudRow;
    CHECK(!RenderSkyTextureIdentityEqual(&cached, &changed));
    return failures != 0;
}
