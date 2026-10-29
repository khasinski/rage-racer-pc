#ifndef RAGE_TEXTURE_UPSCALE_H
#define RAGE_TEXTURE_UPSCALE_H

#include <stddef.h>
#include <stdint.h>

typedef struct RageTextureUpscaleWindow {
    uint16_t offsetU;
    uint16_t offsetV;
    uint16_t widthU;
    uint16_t heightV;
    int wrapHorizontal;
} RageTextureUpscaleWindow;

/* Returns the output byte size for upscaling width x height RGBA8 by factor
 * (1, 2 or 4), or 0 when the parameters are invalid or overflow. */
size_t TextureUpscaleSize(uint32_t width, uint32_t height, uint32_t factor);

/* Edge-directed RGBA8 texture upscaler (factor 1, 2 or 4).
 * - Preserves source-authoritative transparency (alpha == 0 stays 0).
 * - Dilates RGB across alpha boundaries before interpolation so transparent
 *   cutout edges never bleed dark texels into opaque silhouettes.
 * - Respects PS1 sub-page texture windows (offsetU/V, widthU/V) or horizontal
 *   sky wrapping so tiled UVs remain seamless.
 * - Smooths 45-degree diagonals and continuous shade ramps while keeping hard
 *   horizontal/vertical contrast edges crisp. */
int TextureUpscaleRGBA(const uint8_t *src, uint32_t width, uint32_t height,
                       uint32_t factor, const RageTextureUpscaleWindow *window,
                       uint8_t *dst, size_t dstCapacity);

/* Pre-processes a source RGBA8 texture before feeding an external AI upscaler
 * (such as RealESRGAN): dilates opaque neighbor RGB into transparent texels
 * so neural convolutions do not pull black fringes into cutout silhouettes. */
int TextureUpscaleDilateAlphaRGB(uint8_t *rgba, uint32_t width, uint32_t height,
                                 uint32_t passes);

#endif
