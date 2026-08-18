/* The decoded material has to agree with modern.frag.glsl texel for texel,
 * or the raster and raytraced paths will disagree on pixels. These tests
 * build a small synthetic VRAM and check the decode against the addressing
 * the shader performs. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/port/modern/material_decode.h"

#define VRAM_W 1024
#define VRAM_H 512
#define PITCH (VRAM_W * 4)

static int failures;

#define EXPECT_EQ(expected, actual)                                           \
    do {                                                                      \
        long e = (long)(expected), a = (long)(actual);                        \
        if (e != a) {                                                         \
            fprintf(stderr, "%s:%d: expected %ld, got %ld\n", __FILE__,       \
                    __LINE__, e, a);                                          \
            failures++;                                                       \
        }                                                                     \
    } while (0)

static uint8_t *vram;

static void PutPixel(int x, int y, int r, int g, int b, int a) {
    uint8_t *p = vram + (size_t)y * PITCH + (size_t)x * 4;
    p[0] = (uint8_t)r;
    p[1] = (uint8_t)g;
    p[2] = (uint8_t)b;
    p[3] = (uint8_t)a;
}

/* Writes a 16-bit word the way the GPU stores it, so the decode's
 * reconstruction is exercised rather than bypassed. */
static void PutWord(int x, int y, unsigned word) {
    unsigned r = word & 0x1fu, g = (word >> 5) & 0x1fu, b = (word >> 10) & 0x1fu;
    PutPixel(x, y, (int)(r * 255 / 31), (int)(g * 255 / 31), (int)(b * 255 / 31),
             (word & 0x8000u) ? 255 : 0);
}

static const uint8_t *TexelAt(const uint8_t *rgba, int x, int y) {
    return rgba + ((size_t)y * RAGE_PAGE_TEXELS + x) * 4;
}

static void test_page_addressing(void) {
    RageTexturePage page = RageTexturePageFromTpage(0);
    EXPECT_EQ(0, page.x);
    EXPECT_EQ(0, page.y);
    EXPECT_EQ(RAGE_TEXTURE_MODE_4BIT, page.mode);
    EXPECT_EQ(64, page.words);

    /* Low five bits pick the slot: x steps by 64, y flips at 16. */
    page = RageTexturePageFromTpage(17);
    EXPECT_EQ(64, page.x);
    EXPECT_EQ(256, page.y);

    page = RageTexturePageFromTpage(15);
    EXPECT_EQ(960, page.x);
    EXPECT_EQ(0, page.y);

    /* Bits 7-8 pick the depth, and the depth sets the VRAM footprint. */
    page = RageTexturePageFromTpage(0x80);
    EXPECT_EQ(RAGE_TEXTURE_MODE_8BIT, page.mode);
    EXPECT_EQ(128, page.words);
    page = RageTexturePageFromTpage(0x100);
    EXPECT_EQ(RAGE_TEXTURE_MODE_16BIT, page.mode);
    EXPECT_EQ(256, page.words);
}

static void test_clut_origin(void) {
    int x, y;
    RageClutOrigin(0, &x, &y);
    EXPECT_EQ(0, x);
    EXPECT_EQ(0, y);
    RageClutOrigin(1, &x, &y);
    EXPECT_EQ(16, x);
    EXPECT_EQ(0, y);
    RageClutOrigin(64, &x, &y);
    EXPECT_EQ(0, x);
    EXPECT_EQ(1, y);
    RageClutOrigin(65, &x, &y);
    EXPECT_EQ(16, x);
    EXPECT_EQ(1, y);
}

static void test_decode_4bit(void) {
    static uint8_t out[RAGE_PAGE_TEXELS * RAGE_PAGE_TEXELS * 4];
    RageTexturePage page = RageTexturePageFromTpage(0);
    int i;

    /* Palette of sixteen distinguishable colours at CLUT 0, entry 0 opaque
     * black so it survives the transparent-texel rule. */
    for (i = 0; i < 16; i++) PutPixel(i, 0, i * 16 + 1, 200, 100, 255);
    /* One word holding indices 3, 1, 4, 1 across its four texels. */
    PutWord(0, 0, 3u | (1u << 4) | (4u << 8) | (1u << 12));

    RageDecodeTexturePage(vram, PITCH, page, 0, out);

    EXPECT_EQ(3 * 16 + 1, TexelAt(out, 0, 0)[0]);
    EXPECT_EQ(1 * 16 + 1, TexelAt(out, 1, 0)[0]);
    EXPECT_EQ(4 * 16 + 1, TexelAt(out, 2, 0)[0]);
    EXPECT_EQ(1 * 16 + 1, TexelAt(out, 3, 0)[0]);
    EXPECT_EQ(255, TexelAt(out, 0, 0)[3]);
}

static void test_decode_8bit(void) {
    static uint8_t out[RAGE_PAGE_TEXELS * RAGE_PAGE_TEXELS * 4];
    RageTexturePage page = RageTexturePageFromTpage(0x80);
    int i;

    for (i = 0; i < 256; i++) PutPixel(i, 3, i, 90, 40, 255);
    /* At 8bpp one word carries two texels: low byte then high byte. */
    PutWord(0, 1, 200u | (37u << 8));

    RageDecodeTexturePage(vram, PITCH, page, 64 * 3, out);

    EXPECT_EQ(200, TexelAt(out, 0, 1)[0]);
    EXPECT_EQ(37, TexelAt(out, 1, 1)[0]);
}

static void test_transparent_texel_stays_transparent(void) {
    static uint8_t out[RAGE_PAGE_TEXELS * RAGE_PAGE_TEXELS * 4];
    RageTexturePage page = RageTexturePageFromTpage(0);

    /* Palette entry 5 is the all-zero word the shader discards. */
    PutPixel(5, 0, 0, 0, 0, 0);
    PutWord(0, 2, 5u);

    RageDecodeTexturePage(vram, PITCH, page, 0, out);

    EXPECT_EQ(0, TexelAt(out, 0, 2)[0]);
    EXPECT_EQ(0, TexelAt(out, 0, 2)[3]);
}

int main(void) {
    vram = calloc((size_t)VRAM_H * PITCH, 1);
    if (vram == NULL) return 1;
    test_page_addressing();
    test_clut_origin();
    test_decode_4bit();
    test_decode_8bit();
    test_transparent_texel_stays_transparent();
    free(vram);
    if (failures != 0) {
        fprintf(stderr, "%d material decode assertion(s) failed\n", failures);
        return 1;
    }
    printf("material decode tests passed\n");
    return 0;
}
