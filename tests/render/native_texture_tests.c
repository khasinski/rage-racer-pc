#include "native_texture.h"
#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static uint8_t rgba[256 * 256 * 4], reference[256 * 256 * 4], paint[256 * 256];
static uint16_t car[64 * 256], other[64 * 256], vram[1024 * 512];

int main(void) {
    for (size_t i = 0; i < 64 * 256; i++) car[i] = (uint16_t)(i * 19 + 3);
    TextureImage owned = {car, 64 * 256, 704, 0, 64, 256, NULL};
    TextureImage second = {other, 64 * 256, 704, 0, 64, 256, NULL};
    for (unsigned row = 0; row < 256; row++)
        memcpy(vram + row * 1024 + 704, car + row * 64, 64 * sizeof(uint16_t));
    TextureImage full = {vram, 1024 * 512, 0, 0, 1024, 512, NULL};
    const uint16_t clut = (240 << 6) | 44;
    for (unsigned mode = 0; mode < 3; mode++) {
        uint16_t tpage = (uint16_t)(11 | (mode << 7));
        unsigned width = mode == 0 ? 256 : mode == 1 ? 128 : 64;
        for (unsigned y = 0; y < 256; y++)
            for (unsigned x = 0; x < width; x++)
                CHECK(TextureWord(&owned, tpage, clut, x, y) ==
                      TextureWord(&full, tpage, clut, x, y));
    }
    car[0] = 0x3210;
    car[240 * 64] = 0x1111;
    car[240 * 64 + 3] = 0x3333;
    CHECK(TexturePaletteIndex(&owned, 11, 0, 0) == 0);
    CHECK(TexturePaletteIndex(&owned, 11, 3, 0) == 3);
    CHECK(TextureWord(&owned, 11, clut, 0, 0) == 0x1111);
    CHECK(TextureWord(&owned, 11, clut, 3, 0) == 0x3333);
    CHECK(TextureWord(&second, 11, clut, 3, 0) == 0);
    CHECK(TexturePaletteIndex(&owned, 11 | (1 << 7), 0, 0) == 0x10);
    CHECK(TexturePaletteIndex(&owned, 11 | (1 << 7), 1, 0) == 0x32);
    CHECK(TextureWord(&owned, 11 | (2 << 7), clut, 0, 0) == 0x3210);
    CHECK(TextureWord(&owned, 11 | (1 << 7), clut, 128, 0) == 0);
    CHECK(TextureWord(&owned, 11 | (2 << 7), clut, 64, 0) == 0);
    CHECK(TextureWord(&owned, 10, clut, 0, 0) == 0);
    CHECK(TextureWord(&owned, 11, clut, 256, 0) == 0);
    CHECK(TextureWord(&owned, 11, clut, 0, 256) == 0);
    CHECK(TextureWord(NULL, 11, clut, 0, 0) == 0);
    CHECK(TextureWord(&owned, 11, 0, 3, 0) == 0); /* palette outside owned image */
    TextureImage truncated = owned;
    truncated.count = 1;
    CHECK(TextureWord(&truncated, 11, clut, 3, 0) == 0);
    CHECK(TextureImageWord(&truncated, 705, 0) == 0);
    size_t index;
    CHECK(!TextureImageIndex(&owned, 704, 0, NULL));
    uint8_t color[4];
    TextureColor(0x001F, color);
    CHECK(color[0] == 255 && color[1] == 0 && color[2] == 0 && color[3] == 255);
    TextureColor(0, color); CHECK(color[3] == 0);
    TextureColor(0x8000, color); CHECK(color[0] == 0 && color[3] == 255);
    RageImportedTextureKey texture = {.tpage = 11};
    CHECK(DecodeTexture(&texture, clut, &owned, rgba, sizeof(rgba), NULL, 0));
    /* Sync fixture changes into a full source; decoder must preserve crop semantics. */
    for (unsigned row = 0; row < 256; ++row)
        memcpy(vram + row * 1024 + 704, car + row * 64, 64 * sizeof(uint16_t));
    CHECK(DecodeTexture(&texture, clut, &full, reference, sizeof(reference), NULL, 0));
    CHECK(memcmp(rgba, reference, sizeof(rgba)) == 0);
    const uint16_t paintClut = (225 << 6) | 47;
    car[0] = 0x1111;
    car[225 * 64 + 49] = 0x001F;
    CHECK(DecodeTexture(&texture, paintClut, &owned, rgba, sizeof(rgba), paint, sizeof(paint)));
    CHECK(rgba[0] == 255 && rgba[1] == 0 && paint[0] == 1);
    texture.hasWindow = 1; texture.windowWidthU = texture.windowWidthV = 8;
    CHECK(DecodeTexture(&texture, paintClut, &owned, rgba, sizeof(rgba), NULL, 0));
    CHECK(memcmp(rgba, rgba + 8 * 4, 4) == 0);
    memset(rgba, 0xA5, sizeof(rgba)); memcpy(reference, rgba, sizeof(rgba));
    texture.windowWidthU = 0;
    CHECK(!DecodeTexture(&texture, clut, &owned, rgba, sizeof(rgba), NULL, 0));
    CHECK(memcmp(rgba, reference, sizeof(rgba)) == 0);
    texture.hasWindow = 0;
    CHECK(!DecodeTexture(&texture, clut, &truncated, rgba, sizeof(rgba), NULL, 0));
    CHECK(!DecodeTexture(&texture, clut, &owned, rgba, sizeof(rgba) - 1, NULL, 0));
    CHECK(!DecodeTexture(&texture, clut, &owned, rgba, sizeof(rgba), paint, sizeof(paint) - 1));
    CHECK(memcmp(rgba, reference, sizeof(rgba)) == 0);
    texture.tpage = 11 | (2 << 7);
    memset(paint, 0xFF, sizeof(paint));
    CHECK(DecodeTexture(&texture, clut, &owned, rgba, sizeof(rgba), paint, sizeof(paint)));
    for (size_t i = 0; i < sizeof(paint); ++i) CHECK(paint[i] == 0);
    owned.width = 0;
    CHECK(!TextureImageIndex(&owned, 704, 0, &index));
/* Texels and palette can be independently owned source rectangles. */
uint16_t texels[] = {0x1111}, palette[16] = {0, 0x001f};
TextureImage colors = {palette, 16, 16, 480, 16, 1, NULL};
TextureImage split = {texels, 1, 640, 0, 1, 1, &colors};
texture.tpage = 10;
const uint16_t logoClut = (480 << 6) | 1;
CHECK(DecodeTexture(&texture, logoClut, &split, rgba, sizeof(rgba), NULL, 0));
CHECK(rgba[0] == 255 && rgba[3] == 255);
CHECK(rgba[4 * 4 + 3] == 0); /* Missing texels never select palette zero. */
memset(rgba, 0xa5, sizeof(rgba));
memcpy(reference, rgba, sizeof(rgba));
colors.count = 15;
CHECK(!DecodeTexture(&texture, logoClut, &split, rgba, sizeof(rgba), NULL, 0));
CHECK(memcmp(rgba, reference, sizeof(rgba)) == 0);
colors.count = 16;
colors.next = &split;
CHECK(!DecodeTexture(&texture, logoClut, &split, rgba, sizeof(rgba), NULL, 0));
CHECK(memcmp(rgba, reference, sizeof(rgba)) == 0);
    u32 primary[] = {0, 24, 0, 0, 16, 0x01000240, 0x00010002, 0x001f001f, 0};
    u32 secondary[] = {0, 24, 0, 0, 16, 0x01000242, 0x00010002, 0x03e003e0, 0};
    u32 carEntry[] = {0, 0, 16, 0x01000240, 0x00010002, 0x7c007c00};
    u32 active[] = {0, 24, 0, 0, 16, 0x01000240, 0x00010002, 0x7fff7fff, 0};
    u32 deferred[] = {0, 24, 0, 0, 16, 0x01000240, 0x00010002, 0x03ff03ff, 0};
    TrackImages source = {.view = {
        .blocks = {primary, secondary, carEntry, active, deferred},
        .sizes = {sizeof(primary), sizeof(secondary), sizeof(carEntry), sizeof(active), sizeof(deferred)}}};
    const u32 base[] = {0, 24, 0, 0, 16, 0, 0x00010002, 0x7c1f7c1f, 0};
    TrackPixels *pages = CopyTrackPixels(&source, base, sizeof(base)), *independent = CopyTrackPixels(&source, base, sizeof(base));
    CHECK(pages && independent);
    const size_t pixel = 256 * 1024 + 576;
    CHECK(pages->pages[1][pixel] == 0x7fff && pages->pages[0][pixel] == 0x03ff);
    CHECK(pages->pages[0][pixel + 2] == 0x03e0 && pages->pages[1][pixel + 2] == 0x03e0);
    CHECK(pages->pages[0][0] == 0x7c1f && pages->pages[1][0] == 0x7c1f);
    CHECK(pages->pages[0][2] == 0 && pages->pages[1][2] == 0);
    RageImportedTextureKey trackTexture = {.tpage = 281}; /* Direct-color page at 576,256. */
    CHECK(DecodeTrackMaterial(pages, &trackTexture, RAGE_RENDER_ASSET_COURSE, 0, 0, NULL, rgba, sizeof(rgba)));
    CHECK(rgba[0] == 255 && rgba[1] == 255 && rgba[2] == 0);
    CHECK(DecodeTrackMaterial(pages, &trackTexture, RAGE_RENDER_ASSET_COURSE, 4, 0, NULL, rgba, sizeof(rgba)));
    CHECK(rgba[0] == 255 && rgba[1] == 255 && rgba[2] == 255);
    CHECK(DecodeTrackMaterial(pages, &trackTexture, RAGE_RENDER_ASSET_TERRAIN, 2, 0, NULL, rgba, sizeof(rgba)));
    CHECK(rgba[2] == 255);
    CHECK(DecodeTrackMaterial(pages, &trackTexture, RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1, 0, 1, NULL, rgba, sizeof(rgba)));
    CHECK(rgba[2] == 255);
    /* Dynamic palette overlays only this race; immutable banks stay intact. */
    uint16_t environmentColors[16] = {0};
    environmentColors[1] = 0x001f;
    pages->pages[0][0] = 0x1111;
    RageImportedTextureKey fogTexture = {.tpage = 0, .clut = (486 << 6) | 14};
    CHECK(DecodeTrackMaterial(pages, &fogTexture, RAGE_RENDER_ASSET_TERRAIN, 0, 0, environmentColors, rgba, sizeof(rgba)));
    CHECK(rgba[0] == 255 && rgba[1] == 0 && rgba[2] == 0);
    environmentColors[1] = 0x03e0;
    CHECK(DecodeTrackMaterial(pages, &fogTexture, RAGE_RENDER_ASSET_TERRAIN, 0, 0, environmentColors, rgba, sizeof(rgba)));
    CHECK(rgba[0] == 0 && rgba[1] == 255 && rgba[2] == 0);
    CHECK(independent->pages[0][0] == 0x7c1f);
    CHECK(pages->pages[0][486 * 1024 + 225] == 0);
    CHECK(DecodeTrackMaterial(pages, &fogTexture, RAGE_RENDER_ASSET_TERRAIN, 0, 0, NULL, rgba, sizeof(rgba)));
    CHECK(rgba[3] == 0);
    CHECK(DecodeTrackMaterial(independent, &fogTexture, RAGE_RENDER_ASSET_TERRAIN, 0, 0, NULL, rgba, sizeof(rgba)));
    CHECK(rgba[3] == 0);
    memcpy(reference, rgba, sizeof(rgba));
    CHECK(!DecodeTrackMaterial(pages, &trackTexture, RAGE_RENDER_ASSET_MODEL_BANK, 0, 0, NULL, rgba, sizeof(rgba)));
    CHECK(!DecodeTrackMaterial(NULL, &trackTexture, RAGE_RENDER_ASSET_COURSE, 0, 0, NULL, rgba, sizeof(rgba)));
    CHECK(!DecodeTrackMaterial(pages, &trackTexture, RAGE_RENDER_ASSET_COURSE, 256, 0, NULL, rgba, sizeof(rgba)));
    CHECK(memcmp(reference, rgba, sizeof(rgba)) == 0);
    source.view.sizes[4] = 4;
    CHECK(CopyTrackPixels(&source, base, sizeof(base)) == NULL && CopyTrackPixels(NULL, base, sizeof(base)) == NULL);
    CHECK(pages->pages[1][pixel] == 0x7fff && pages->pages[0][pixel] == 0x03ff);
    memset(deferred, 0, sizeof(deferred));
    CHECK(pages->pages[0][pixel] == 0x03ff);
    FreeTrackPixels(pages);
    CHECK(independent->pages[0][pixel] == 0x03ff);
    FreeTrackPixels(independent);
    FreeTrackPixels(NULL);
    return 0;
}
