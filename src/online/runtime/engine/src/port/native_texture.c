#include "native_texture.h"
#include "native_import_stream.h"
#include "track_material_page.h"
#include <stdlib.h>
#include <string.h>

void TextureColor(uint16_t word, uint8_t rgba[4]) {
    uint8_t r = (uint8_t)((word & 0x1Fu) << 3);
    uint8_t g = (uint8_t)(((word >> 5) & 0x1Fu) << 3);
    uint8_t b = (uint8_t)(((word >> 10) & 0x1Fu) << 3);
    rgba[0] = (uint8_t)(r | (r >> 5));
    rgba[1] = (uint8_t)(g | (g >> 5));
    rgba[2] = (uint8_t)(b | (b >> 5));
    rgba[3] = word == 0 ? 0 : 255;
}

static uint8_t ImportCarPaintCode(uint32_t x, uint32_t y) {
    static const uint16_t slots3A[] =
        {1, 0x41, 0xC1, 0x101, 0x181, 0x241, 0x281, 0x301, 0x341};
    static const uint16_t slots3B[] =
        {1, 0x41, 0xC1, 0x181, 0x241, 0x281, 0x301, 0x341};
    static const uint16_t slots4[] = {0x141, 0x1C1, 0x201, 0x401};
    static const uint8_t first3[] = {1, 4, 7};
    static const uint8_t second3[] = {8, 11, 14};
    static const uint8_t first4[] = {1, 3, 5, 7};
    static const uint8_t second4[] = {8, 10, 12, 14};
    static const uint8_t first5[] = {1, 2, 4, 6, 7};
    static const uint8_t second5[] = {8, 9, 11, 13, 14};
    uint32_t word, entry, index;
    if (x < 704 || x >= 768 || y >= 256) return 0;
    word = (y * 64u + (x - 704u));
    if (word < 0x7060u / 2u) return 0;
    entry = word - 0x7060u / 2u;
    /* CarPaintPalette.fixed.bodyColor1: the one entry retail's
     * ApplyPrimaryBodyColor sets outside the gradients, to the first colour's
     * primary (the Erriso's roof is drawn with it). */
    if (entry == 0x81) return 1;
    for (index = 0; index < sizeof(slots3A) / sizeof(slots3A[0]); index++) {
        uint32_t offset = entry - slots3A[index];
        if (entry >= slots3A[index] && offset < 3) return first3[offset];
    }
    for (index = 0; index < sizeof(slots3B) / sizeof(slots3B[0]); index++) {
        uint32_t start = slots3B[index] + 3u;
        uint32_t offset = entry - start;
        if (entry >= start && offset < 3) return second3[offset];
    }
    for (index = 0; index < sizeof(slots4) / sizeof(slots4[0]); index++) {
        uint32_t start = slots4[index];
        if (entry >= start && entry - start < 4)
            return first4[entry - start];
        start += 4;
        if (entry >= start && entry - start < 4)
            return second4[entry - start];
    }
    if (entry >= 0x2C1 && entry - 0x2C1 < 5)
        return first5[entry - 0x2C1];
    if (entry >= 0x2C6 && entry - 0x2C6 < 5)
        return second5[entry - 0x2C6];
    return 0;
}

int DecodeTexture(const RageImportedTextureKey *texture, uint16_t clut,
                  const TextureImage *source, uint8_t *pixels, size_t pixelSize,
                  uint8_t *paint, size_t paintSize) {
    if (!texture || !source || !source->words || !source->width || !source->height ||
        source->height > SIZE_MAX / source->width ||
        source->count < (size_t)source->width * source->height ||
        !pixels || pixelSize < 256u * 256u * 4u ||
        (paint && paintSize < 256u * 256u) ||
        (texture->hasWindow && (!texture->windowWidthU || !texture->windowWidthV ||
         texture->windowWidthU > 256 || texture->windowWidthV > 256 ||
         texture->windowOffsetU > 256 - texture->windowWidthU ||
         texture->windowOffsetV > 256 - texture->windowWidthV))) return 0;
    /* Reject cycles and malformed secondary rectangles before writing output. */
    const TextureImage *slow = source, *fast = source;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) return 0;
    }
    for (const TextureImage *image = source->next; image; image = image->next)
        if (!image->words || !image->width || !image->height ||
            image->height > SIZE_MAX / image->width ||
            image->count < (size_t)image->width * image->height) return 0;
    uint32_t y, x;
    for (y = 0; y < 256; y++) {
        uint32_t sourceV = texture->hasWindow
            ? y % texture->windowWidthV + texture->windowOffsetV : y;
        for (x = 0; x < 256; x++) {
            uint32_t sourceU = texture->hasWindow
                ? x % texture->windowWidthU + texture->windowOffsetU : x;
            uint8_t *rgba = pixels + (y * 256u + x) * 4u;
            TextureColor(TextureWord(
                                source, texture->tpage, clut, sourceU, sourceV),
                            rgba);
            if (paint) paint[y * 256u + x] = 0;
            if (paint != NULL && ((texture->tpage >> 7) & 3u) <= 1) {
                uint32_t clutX = (clut & 0x3Fu) * 16u;
                uint32_t clutY = (clut >> 6) & 0x1FFu;
                uint8_t palette = TexturePaletteIndex(
                    source, texture->tpage, sourceU, sourceV);
                paint[y * 256u + x] =
                    ImportCarPaintCode(clutX + palette, clutY);
            }
        }
    }
    return 1;
}

int CarTextureImages(const CarModelData *model, TextureImage images[3]) {
    if (!model || !model->image || !model->hasSharedImage || !images) return 0;
    images[2] = (TextureImage){model->logoPalette, 16, 16, 480, 16, 1, NULL};
    images[1] = (TextureImage){model->sharedImage, 64 * 256, 640, 0, 64, 256, &images[2]};
    images[0] = (TextureImage){(const u16 *)(const void *)model->image,
        sizeof(CarImageData) / sizeof(u16), 704, 0, 64, 256, &images[1]};
    return 1;
}

TrackPixels *CopyTrackPixels(const TrackImages *images, const void *base, size_t baseSize) {
    if (!images) return NULL;
    TrackPixels *pixels = calloc(1, sizeof(*pixels));
    if (!pixels) return NULL;
    ImagePixels page = {pixels->pages[1], RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT,
                        0, 0, RAGE_TRACK_VRAM_WIDTH, RAGE_TRACK_VRAM_HEIGHT};
    if (!CopyImageAssetPixels(base, baseSize, &page)) goto fail;
    const TrackTextureAssetView *view = &images->view;
    for (s32 block = TRACK_TEXTURE_PRIMARY_IMAGES; block <= TRACK_TEXTURE_ACTIVE_IMAGES; ++block) {
        int copied = block == TRACK_TEXTURE_CAR_IMAGE
            ? CopyImageEntryPixels(view->blocks[block], view->sizes[block], &page)
            : CopyImageAssetPixels(view->blocks[block], view->sizes[block], &page);
        if (!copied) goto fail;
    }
    memcpy(pixels->pages[0], pixels->pages[1], sizeof(pixels->pages[0]));
    page.words = pixels->pages[0];
    if (!CopyImageAssetPixels(view->blocks[TRACK_TEXTURE_DEFERRED_IMAGES],
                              view->sizes[TRACK_TEXTURE_DEFERRED_IMAGES], &page)) goto fail;
    return pixels;
fail:
    free(pixels);
    return NULL;
}

void FreeTrackPixels(TrackPixels *pixels) { free(pixels); }

int DecodeTrackMaterial(const TrackPixels *pixels, const RageImportedTextureKey *texture,
                        RenderAssetSet set, unsigned variant, int currentPage,
                        const uint16_t *environmentClut,
                        uint8_t *rgba, size_t size) {
    if (!pixels || !texture || variant > UINT8_MAX ||
        (set != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1 &&
         set != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2 &&
         set != RAGE_RENDER_ASSET_COURSE && set != RAGE_RENDER_ASSET_TERRAIN)) return 0;
    const TextureImage image = {
        pixels->pages[TrackMaterialPage(set, variant, currentPage)],
        RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT,
        0, 0, RAGE_TRACK_VRAM_WIDTH, RAGE_TRACK_VRAM_HEIGHT, NULL};
    const TextureImage palette = {environmentClut, 16, 224, 486, 16, 1, &image};
    return DecodeTexture(texture, ImportMaterialClut(texture, set, (uint8_t)variant),
                          environmentClut ? &palette : &image, rgba, size, NULL, 0);
}
