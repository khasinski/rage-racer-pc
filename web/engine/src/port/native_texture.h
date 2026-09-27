#ifndef PORT_NATIVE_TEXTURE_H
#define PORT_NATIVE_TEXTURE_H
#include <stddef.h>
#include <stdint.h>
#include "game/track_images.h"
#include "track_texture_snapshot.h"

/* Bounded source rectangle for retail texture conversion. An owned car image
 * occupies only its 64x256 words; no full VRAM image or upload is required. */
typedef struct TextureImage {
    const uint16_t *words;
    size_t count;
    uint32_t x, y, width, height;
    const struct TextureImage *next;
} TextureImage;

static inline int TextureImageIndex(const TextureImage *image,
                                    uint32_t x, uint32_t y, size_t *index) {
    if (image == NULL || index == NULL || image->words == NULL || x < image->x || y < image->y ||
        x - image->x >= image->width || y - image->y >= image->height) return 0;
    size_t column = x - image->x, row = y - image->y;
    if (row > (SIZE_MAX - column) / image->width) return 0;
    *index = row * image->width + column;
    return *index < image->count;
}

static inline int TextureImageRead(const TextureImage *image,
                                  uint32_t x, uint32_t y, uint16_t *word) {
    for (; image != NULL; image = image->next) {
        size_t index;
        if (TextureImageIndex(image, x, y, &index)) {
            *word = image->words[index];
            return 1;
        }
    }
    return 0;
}

static inline uint16_t TextureImageWord(const TextureImage *image,
                                       uint32_t x, uint32_t y) {
    uint16_t word = 0;
    TextureImageRead(image, x, y, &word);
    return word;
}

static inline uint8_t TexturePaletteIndex(const TextureImage *image,
    uint16_t tpage, uint32_t u, uint32_t v) {
    uint32_t mode = (tpage >> 7) & 3u;
    uint32_t pageX = (tpage & 15u) * 64u;
    uint32_t pageY = ((tpage >> 4) & 1u) * 256u;
    if (u >= 256 || v >= 256) return 0;
    if (mode == 0)
        return (uint8_t)((TextureImageWord(image, pageX + u / 4, pageY + v) >>
                         ((u & 3u) * 4u)) & 15u);
    if (mode == 1)
        return (uint8_t)((TextureImageWord(image, pageX + u / 2, pageY + v) >>
                         ((u & 1u) * 8u)) & 255u);
    return 0;
}

static inline uint16_t TextureWord(const TextureImage *image, uint16_t tpage,
                                  uint16_t clut, uint32_t u, uint32_t v) {
    uint32_t mode = (tpage >> 7) & 3u;
    uint32_t pageX = (tpage & 15u) * 64u;
    uint32_t pageY = ((tpage >> 4) & 1u) * 256u;
    if (u >= 256 || v >= 256) return 0;
    if (mode <= 1) {
        uint16_t texel;
        if (!TextureImageRead(image, pageX + u / (mode == 0 ? 4 : 2), pageY + v, &texel)) return 0;
        return TextureImageWord(image, (clut & 63u) * 16u +
            TexturePaletteIndex(image, tpage, u, v), (clut >> 6) & 511u);
    }
    return TextureImageWord(image, pageX + u, pageY + v);
}
#include "native_mesh_writer.h"
#include "game/car_model_data.h"

/* Stack-local views borrowing a complete owned car source. */
int CarTextureImages(const CarModelData *model, TextureImage images[3]);

void TextureColor(uint16_t word, uint8_t rgba[4]);
/* Caller owns output buffers. Invalid input leaves them unchanged. */
int DecodeTexture(const RageImportedTextureKey *texture, uint16_t clut,
                  const TextureImage *source, uint8_t *pixels, size_t pixelSize,
                  uint8_t *paint, size_t paintSize);

/* Owns both full retail-coordinate texture banks, reconstructed without GPU.
 * Page 1 precedes the deferred images; page 0 includes them. Coordinates not
 * authored by the base atlas or pack remain zero; no prior scene is borrowed. */
typedef struct TrackPixels {
    uint16_t pages[2][RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT];
} TrackPixels;
/* Base is the shared boot image chain, applied before the course pack. */
TrackPixels *CopyTrackPixels(const TrackImages *images, const void *base, size_t baseSize);
/* Decode a material from explicit immutable banks. Page selection and palette
 * variants follow the same rules as the legacy importer. Optional environmentClut
 * borrows 16 animated colors for this decode, without changing either bank. */
int DecodeTrackMaterial(const TrackPixels *pixels, const RageImportedTextureKey *texture,
                        RenderAssetSet set, unsigned variant, int currentPage,
                        const uint16_t *environmentClut,
                        uint8_t *rgba, size_t size);
void FreeTrackPixels(TrackPixels *pixels);
#endif

