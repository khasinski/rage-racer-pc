#include "material_decode.h"

#include <string.h>

/* The shader reads VRAM as RGBA8 and rebuilds the 16-bit word from it, so the
 * decode does the same rather than assuming a packed layout. */
static unsigned RageWordAt(const uint8_t *vram, int pitch, int x, int y) {
    const uint8_t *p = vram + (size_t)y * pitch + (size_t)x * 4;
    unsigned r = (unsigned)((p[0] * 31 + 127) / 255);
    unsigned g = (unsigned)((p[1] * 31 + 127) / 255);
    unsigned b = (unsigned)((p[2] * 31 + 127) / 255);
    unsigned a = p[3] >= 128 ? 1u : 0u;
    return r | (g << 5) | (b << 10) | (a << 15);
}

static void RageStoreTexel(const uint8_t *vram, int pitch, int x, int y,
                           uint8_t *out) {
    const uint8_t *p = vram + (size_t)y * pitch + (size_t)x * 4;
    /* An all-zero colour is the PS1's transparent texel; the shader discards
     * it, so the material carries it as fully transparent black. */
    if (p[0] == 0 && p[1] == 0 && p[2] == 0 && p[3] == 0) {
        out[0] = out[1] = out[2] = out[3] = 0;
        return;
    }
    out[0] = p[0];
    out[1] = p[1];
    out[2] = p[2];
    out[3] = 255;
}

RageTexturePage RageTexturePageFromTpage(unsigned tpage) {
    RageTexturePage page;
    unsigned low = tpage % 32u;
    page.x = (int)((low % 16u) * 64u);
    page.y = (int)((low / 16u) * 256u);
    page.mode = (int)((tpage >> 7) & 3u);
    if (page.mode > RAGE_TEXTURE_MODE_16BIT) page.mode = RAGE_TEXTURE_MODE_16BIT;
    page.words = page.mode == RAGE_TEXTURE_MODE_4BIT    ? 64
                 : page.mode == RAGE_TEXTURE_MODE_8BIT  ? 128
                                                        : 256;
    return page;
}

void RageClutOrigin(unsigned clut, int *x, int *y) {
    *x = (int)((clut % 64u) * 16u);
    *y = (int)(clut / 64u);
}

void RageDecodeTexturePageRegions(const uint8_t *pixels, int pixelsPitch,
                                  const uint8_t *palette, int mode,
                                  uint8_t *out) {
    int tx, ty;
    for (ty = 0; ty < RAGE_PAGE_TEXELS; ty++) {
        for (tx = 0; tx < RAGE_PAGE_TEXELS; tx++) {
            uint8_t *dst = out + ((size_t)ty * RAGE_PAGE_TEXELS + tx) * 4;
            if (mode >= RAGE_TEXTURE_MODE_16BIT) {
                RageStoreTexel(pixels, pixelsPitch, tx, ty, dst);
                continue;
            }
            {
                /* 4bpp packs four indices per word, 8bpp packs two. */
                int shiftPerTexel = mode == RAGE_TEXTURE_MODE_8BIT ? 1 : 2;
                unsigned subMask = mode == RAGE_TEXTURE_MODE_8BIT ? 1u : 3u;
                unsigned idxShift = mode == RAGE_TEXTURE_MODE_8BIT ? 8u : 4u;
                unsigned idxMask = mode == RAGE_TEXTURE_MODE_8BIT ? 0xffu
                                                                  : 0x0fu;
                unsigned sub = (unsigned)tx & subMask;
                unsigned word =
                    RageWordAt(pixels, pixelsPitch, tx >> shiftPerTexel, ty);
                unsigned index = (word >> (sub * idxShift)) & idxMask;
                if (palette == NULL) {
                    dst[0] = dst[1] = dst[2] = dst[3] = 0;
                    continue;
                }
                RageStoreTexel(palette, 0, (int)index, 0, dst);
            }
        }
    }
}

void RageDecodeTexturePage(const uint8_t *vram, int vramPitch,
                           RageTexturePage page, unsigned clut,
                           uint8_t *out) {
    int clutX, clutY;
    RageClutOrigin(clut, &clutX, &clutY);
    RageDecodeTexturePageRegions(
        vram + (size_t)page.y * vramPitch + (size_t)page.x * 4, vramPitch,
        vram + (size_t)clutY * vramPitch + (size_t)clutX * 4, page.mode, out);
}
