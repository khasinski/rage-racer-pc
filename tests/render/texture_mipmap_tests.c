#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/texture_mipmap.h"

static int failures;

#define EXPECT_EQ(expected, actual) do {                                  \
    unsigned long long expectedValue = (unsigned long long)(expected);     \
    unsigned long long actualValue = (unsigned long long)(actual);         \
    if (expectedValue != actualValue) {                                    \
        fprintf(stderr, "%s:%d: expected %llu, got %llu\n", __FILE__,   \
                __LINE__, expectedValue, actualValue);                     \
        failures++;                                                        \
    }                                                                      \
} while (0)

static void test_mip_chain_keeps_atlas_tiles_separate(void) {
    uint8_t source[16 * 8 * 4];
    uint8_t chain[16 * 8 * 4 + 8 * 4 * 4 + 4 * 2 * 4 + 2 * 1 * 4];
    size_t last;
    unsigned x, y;
    memset(source, 0, sizeof(source));
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 16; x++) {
            size_t pixel = (y * 16u + x) * 4u;
            source[pixel + (x < 8 ? 0 : 2)] = 255;
            source[pixel + 3] = 255;
        }
    }
    EXPECT_EQ(4, TextureMipLevelCount(16, 8,
                                          RAGE_TEXTURE_ATLAS_MIP_LEVELS));
    EXPECT_EQ(sizeof(chain), TextureMipChainSizeRGBA8(16, 8, 4));
    EXPECT_EQ(1, TextureBuildMipChainRGBA8(
                     source, 16, 8, 4, chain, sizeof(chain)));
    last = TextureMipLevelOffsetRGBA8(16, 8, 3);
    EXPECT_EQ(255, chain[last + 0]);
    EXPECT_EQ(0, chain[last + 2]);
    EXPECT_EQ(0, chain[last + 4]);
    EXPECT_EQ(255, chain[last + 6]);
}

static void test_mip_chain_filters_in_premultiplied_alpha(void) {
    const uint8_t source[16] = {
        255, 0, 0, 255, 255, 0, 0, 255,
        0, 0, 255, 0,   0, 0, 255, 0,
    };
    uint8_t chain[20];
    EXPECT_EQ(1, TextureBuildMipChainRGBA8(
                     source, 2, 2, 2, chain, sizeof(chain)));
    EXPECT_EQ(128, chain[16 + 0]);
    EXPECT_EQ(0, chain[16 + 1]);
    EXPECT_EQ(0, chain[16 + 2]);
    EXPECT_EQ(128, chain[16 + 3]);
}

static void test_mip_chain_premultiplies_png_base_level(void) {
    const uint8_t source[4] = {200, 100, 50, 128};
    uint8_t chain[4];
    EXPECT_EQ(1, TextureBuildMipChainRGBA8(
                     source, 1, 1, 1, chain, sizeof(chain)));
    EXPECT_EQ(100, chain[0]);
    EXPECT_EQ(50, chain[1]);
    EXPECT_EQ(25, chain[2]);
    EXPECT_EQ(128, chain[3]);
}

static void test_odd_mip_dimensions_include_the_last_texel(void) {
    const uint8_t source[12] = {
        255, 0, 0, 255,
        255, 0, 0, 255,
        0, 0, 255, 255,
    };
    uint8_t chain[16];
    EXPECT_EQ(1, TextureBuildMipChainRGBA8(
                     source, 3, 1, 2, chain, sizeof(chain)));
    EXPECT_EQ(170, chain[12 + 0]);
    EXPECT_EQ(0, chain[12 + 1]);
    EXPECT_EQ(85, chain[12 + 2]);
    EXPECT_EQ(255, chain[12 + 3]);
}

int main(void) {
    test_mip_chain_keeps_atlas_tiles_separate();
    test_mip_chain_filters_in_premultiplied_alpha();
    test_mip_chain_premultiplies_png_base_level();
    test_odd_mip_dimensions_include_the_last_texel();
    if (failures != 0) return EXIT_FAILURE;
    puts("texture mipmap tests passed");
    return EXIT_SUCCESS;
}
