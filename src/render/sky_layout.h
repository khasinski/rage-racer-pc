#ifndef RAGE_RENDER_SKY_LAYOUT_H
#define RAGE_RENDER_SKY_LAYOUT_H
#include <stdint.h>

/* Resolved panorama cells, in row-major order. Values name one of 8 tiles. */
typedef struct RageSkyPanoramaLayout {
    uint8_t tiles[2][8];
} RageSkyPanoramaLayout;

typedef struct RageSkyTextureIdentity {
    uint32_t assetKey;
    uint32_t cloudRow;
    RageSkyPanoramaLayout layout;
    uint8_t hasLayout;
} RageSkyTextureIdentity;

/* Compare semantic fields, never structure padding. Legacy captures with no
 * layout do not give meaning to their tile bytes. Pixel-source generation is
 * still managed by the backend's track/session cache invalidation. */
static inline int RenderSkyTextureIdentityEqual(
    const RageSkyTextureIdentity *a, const RageSkyTextureIdentity *b) {
    if (a->assetKey != b->assetKey || a->cloudRow != b->cloudRow ||
        a->hasLayout != b->hasLayout) return 0;
    if (a->hasLayout) {
        for (unsigned row = 0; row < 2; ++row)
            for (unsigned column = 0; column < 8; ++column)
                if (a->layout.tiles[row][column] != b->layout.tiles[row][column])
                    return 0;
    }
    return 1;
}
#endif
