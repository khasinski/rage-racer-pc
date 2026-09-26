#include "native_sky.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
int main(void) {
    enum { SIZE = 512 * 256 * 4 };
    TrackPixels *pixels = calloc(1, sizeof(*pixels));
    uint8_t *rgba = malloc(SIZE), *reference = malloc(SIZE);
    CHECK(pixels && rgba && reference);
    for (unsigned y = 256; y < 512; ++y)
        for (unsigned x = 512; x < 576; ++x) pixels->pages[1][y * 1024 + x] = 0x1111;
    uint16_t palette[16] = {0, 0x03e0};
    const TextureImage source = {pixels->pages[1], 1024 * 512, 0, 0, 1024, 512, NULL};
    const TextureImage overlay = {palette, 16, 224, 486, 16, 1, &source};
    RageSkyPanoramaLayout layout = {0};
    for (unsigned row = 0; row < 2; ++row)
        for (unsigned column = 0; column < 8; ++column) layout.tiles[row][column] = column;
    CHECK(DecodeSky(&overlay, &layout, rgba, SIZE));
    for (unsigned pixel = 0; pixel < SIZE; pixel += 4)
        CHECK(rgba[pixel] == 255 && rgba[pixel + 1] == 255 && rgba[pixel + 2] == 255 && rgba[pixel + 3] == 255);
    memcpy(reference, rgba, SIZE);
    layout.tiles[1][7] = 8;
    CHECK(!DecodeSky(&overlay, &layout, rgba, SIZE));
    CHECK(!DecodeSky(&overlay, NULL, rgba, SIZE));
    CHECK(!DecodeSky(NULL, &layout, rgba, SIZE));
    CHECK(!DecodeSky(&overlay, &layout, rgba, SIZE - 1));
    CHECK(memcmp(reference, rgba, SIZE) == 0);
    layout.tiles[1][7] = 7;
    CHECK(DecodeSky(&source, &layout, rgba, SIZE));
    CHECK(rgba[3] == 0); /* Overlay was never written into the source bank. */
    CHECK(pixels->pages[1][486 * 1024 + 225] == 0);
    memset(pixels, 0, sizeof(*pixels));
    CHECK(reference[0] == 255 && reference[3] == 255);
    free(pixels); free(rgba); free(reference);
    return 0;
}
