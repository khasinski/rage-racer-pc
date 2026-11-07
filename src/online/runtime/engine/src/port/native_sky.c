#include "native_sky.h"
#include <stdlib.h>
#include <string.h>

int DecodeSky(const TextureImage *source,
                    const RageSkyPanoramaLayout *layout, uint8_t *rgba, size_t size) {
    enum { PAGE_SIZE = 256 * 256 * 4, SKY_SIZE = 512 * 256 * 4 };
    if (!source || !layout || !rgba || size < SKY_SIZE) return 0;
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 8; ++column)
            if (layout->tiles[row][column] >= RAGE_SKY_TILE_COUNT) return 0;
    uint8_t *storage = malloc(PAGE_SIZE + SKY_SIZE);
    if (!storage) return 0;
    uint8_t *sky = storage + PAGE_SIZE;
    const RageImportedTextureKey texture = {.tpage = 0x18, .clut = 0x798e};
    int decoded = DecodeTexture(&texture, texture.clut, source, storage, PAGE_SIZE, NULL, 0) &&
        RageSkyExpandTexturePageLayout(sky, SKY_SIZE, storage, PAGE_SIZE, layout);
    if (decoded) {
        /* Sky texels modulate the environment gradient; keep their brightness,
         * not the palette's tint, matching the existing native importer. */
        for (size_t pixel = 0; pixel < SKY_SIZE; pixel += 4) {
            uint8_t brightness = sky[pixel];
            if (sky[pixel + 1] > brightness) brightness = sky[pixel + 1];
            if (sky[pixel + 2] > brightness) brightness = sky[pixel + 2];
            if (!sky[pixel + 3]) brightness = 0;
            sky[pixel] = sky[pixel + 1] = sky[pixel + 2] = brightness;
        }
        memcpy(rgba, sky, SKY_SIZE);
    }
    free(storage);
    return decoded;
}
