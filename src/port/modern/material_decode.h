#ifndef RAGE_MATERIAL_DECODE_H
#define RAGE_MATERIAL_DECODE_H

#include <stdint.h>

/* Decoding PS1 texture pages into RGBA materials.
 *
 * The rasterizer reaches a texel through VRAM: read an index word, pull the
 * colour out of a CLUT row. A ray that hits a triangle cannot pay for that
 * indirection, so raytracing needs the same texels already resolved. The
 * decode has to agree with modern.frag.glsl exactly or the two paths will
 * disagree on pixels, which is why it lives here and is tested on its own.
 *
 * Coordinates are VRAM pixels: 1024x512 16-bit words. A page is 256x256
 * texels; how much VRAM that covers depends on the depth, 64 words wide at
 * 4bpp, 128 at 8bpp, 256 at 16bpp. */

enum {
    RAGE_TEXTURE_MODE_4BIT = 0,
    RAGE_TEXTURE_MODE_8BIT = 1,
    RAGE_TEXTURE_MODE_16BIT = 2,
    RAGE_PAGE_TEXELS = 256
};

/* Where a tpage's texels start in VRAM, and how they are packed. */
typedef struct RageTexturePage {
    int x, y;   /* VRAM pixel coordinates of the page origin */
    int mode;   /* RAGE_TEXTURE_MODE_* */
    int words;  /* VRAM words per texel row: 64, 128 or 256 */
} RageTexturePage;

/* Splits a tpage field the way the fragment shader does. */
RageTexturePage RageTexturePageFromTpage(unsigned tpage);

/* Where a clut field points, in VRAM pixels. */
void RageClutOrigin(unsigned clut, int *x, int *y);

/* Decodes one page into 256x256 RGBA8.
 *
 * vram holds the whole 1024x512 VRAM as RGBA8888 rows of vramPitch bytes,
 * as Psyz_VideoDownloadVramRegion_SDL3GPU produces it. out takes
 * 256*256*4 bytes. A texel whose colour word is all zero is transparent,
 * matching the shader's discard, and lands as zero here too. */
void RageDecodeTexturePage(const uint8_t *vram, int vramPitch,
                           RageTexturePage page, unsigned clut,
                           uint8_t *out);

/* Same decode from two read-back regions instead of a whole VRAM: pixels
 * points at the page origin, palette at the CLUT origin. A cache reads only
 * the regions it needs, so this is the form it uses; the whole-VRAM entry
 * point above is written in terms of this one. palette may be NULL at 16bpp,
 * which carries colour directly. */
void RageDecodeTexturePageRegions(const uint8_t *pixels, int pixelsPitch,
                                  const uint8_t *palette, int mode,
                                  uint8_t *out);

#endif
