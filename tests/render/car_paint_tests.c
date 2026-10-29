#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/car_paint.h"
#include "render/texture_upscale.h"

static int failures;
#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) {         \
    fprintf(stderr, "%s:%d: expected %u, got %u\n", __FILE__, __LINE__,     \
            (unsigned)(expected), (unsigned)(actual)); failures++;             \
} } while (0)
#define EXPECT_TRUE(cond) do { if (!(cond)) {                                  \
    fprintf(stderr, "%s:%d: condition failed: %s\n", __FILE__, __LINE__,    \
            #cond); failures++;                                                \
} } while (0)

int main(void) {
    uint8_t pixels[] = {
        1, 2, 3, 255, 1, 2, 3, 255, 1, 2, 3, 255,
        1, 2, 3, 255, 1, 2, 3, 255, 1, 2, 3, 255,
    };
    const uint8_t mask[] = {
        RAGE_CAR_PAINT_NONE,
        RAGE_CAR_PAINT_FIRST_PRIMARY,
        RAGE_CAR_PAINT_FIRST_THIRD,
        RAGE_CAR_PAINT_FIRST_HALF,
        RAGE_CAR_PAINT_FIRST_TWO_THIRDS,
        RAGE_CAR_PAINT_SECOND_SECONDARY,
    };
    EXPECT_EQ(1, CarPaintApply(pixels, mask, 6, 3, 12));
    EXPECT_EQ(1, pixels[0]);
    EXPECT_EQ(148, pixels[4]);
    EXPECT_EQ(140, pixels[8]);
    EXPECT_EQ(107, pixels[12]);
    EXPECT_EQ(66, pixels[16]);
    EXPECT_EQ(8, pixels[20]);
    EXPECT_EQ(255, pixels[23]);
    EXPECT_EQ(0, CarPaintApply(pixels, mask, 6, 18, 0));
    EXPECT_EQ(0, CarPaintApply(pixels, mask, SIZE_MAX, 0, 0));

    {
        uint8_t scaled[4 * 4 * 4];
        const uint8_t mask2x2[4] = {
            RAGE_CAR_PAINT_NONE,
            RAGE_CAR_PAINT_FIRST_PRIMARY,
            RAGE_CAR_PAINT_FIRST_HALF,
            RAGE_CAR_PAINT_SECOND_SECONDARY,
        };
        for (size_t i = 0; i < sizeof(scaled); i += 4) {
            scaled[i] = 1;
            scaled[i + 1] = 2;
            scaled[i + 2] = 3;
            scaled[i + 3] = 255;
        }
        EXPECT_EQ(1, CarPaintApplyScaled(scaled, 4, 4, mask2x2, 2, 2, 3, 12));
        /* Outer corners map 100% to their respective mask quadrant. */
        EXPECT_EQ(1, scaled[(0 * 4 + 0) * 4]);
        EXPECT_EQ(148, scaled[(0 * 4 + 3) * 4]);
        EXPECT_EQ(107, scaled[(3 * 4 + 0) * 4]);
        EXPECT_EQ(8, scaled[(3 * 4 + 3) * 4]);
        /* Interior boundary pixels smoothly anti-alias between neighbors. */
        EXPECT_TRUE(scaled[(0 * 4 + 1) * 4] > 1 &&
                    scaled[(0 * 4 + 1) * 4] < 148);
        EXPECT_TRUE(scaled[(3 * 4 + 1) * 4] > 8 &&
                    scaled[(3 * 4 + 1) * 4] < 107);
        EXPECT_EQ(255, scaled[(3 * 4 + 3) * 4 + 3]);
        EXPECT_EQ(0, CarPaintApplyScaled(scaled, 0, 4, mask2x2, 2, 2, 3, 12));
        EXPECT_EQ(0, CarPaintApplyScaled(scaled, 4, 4, mask2x2, 0, 2, 3, 12));
        EXPECT_EQ(0, CarPaintApplyScaled(scaled, 4, 4, mask2x2, 2, 2, 18, 12));
    }

    /* TextureUpscale unit tests: 1x, 2x, 4x, alpha preservation, RGB dilation,
     * diagonal reconstruction and gradient smoothing. */
    {
        uint8_t src4x4[4 * 4 * 4];
        uint8_t out8x8[8 * 8 * 4];
        uint8_t out16x16[16 * 16 * 4];
        memset(src4x4, 0, sizeof(src4x4));
        /* Make (0, 0) transparent (0,0,0,0), (1, 0) and (0, 1) bright red
         * (200, 20, 10, 255) forming a 45-degree diagonal across (1, 1) which
         * is blue (20, 40, 200, 255). */
        src4x4[(0 * 4 + 1) * 4 + 0] = 200;
        src4x4[(0 * 4 + 1) * 4 + 1] = 20;
        src4x4[(0 * 4 + 1) * 4 + 2] = 10;
        src4x4[(0 * 4 + 1) * 4 + 3] = 255;
        src4x4[(1 * 4 + 0) * 4 + 0] = 200;
        src4x4[(1 * 4 + 0) * 4 + 1] = 20;
        src4x4[(1 * 4 + 0) * 4 + 2] = 10;
        src4x4[(1 * 4 + 0) * 4 + 3] = 255;
        src4x4[(1 * 4 + 1) * 4 + 0] = 20;
        src4x4[(1 * 4 + 1) * 4 + 1] = 40;
        src4x4[(1 * 4 + 1) * 4 + 2] = 200;
        src4x4[(1 * 4 + 1) * 4 + 3] = 255;

        EXPECT_EQ(4u * 4u * 4u, TextureUpscaleSize(4, 4, 1));
        EXPECT_EQ(8u * 8u * 4u, TextureUpscaleSize(4, 4, 2));
        EXPECT_EQ(16u * 16u * 4u, TextureUpscaleSize(4, 4, 4));
        EXPECT_EQ(0u, TextureUpscaleSize(4, 4, 3));

        EXPECT_EQ(1, TextureUpscaleRGBA(src4x4, 4, 4, 2, NULL, out8x8,
                                        sizeof(out8x8)));
        /* Transparent texel (0,0) -> all 4 subpixels in 2x output stay alpha 0, RGB 0. */
        EXPECT_EQ(0, out8x8[(0 * 8 + 0) * 4 + 3]);
        EXPECT_EQ(0, out8x8[(0 * 8 + 1) * 4 + 3]);
        EXPECT_EQ(0, out8x8[(1 * 8 + 0) * 4 + 3]);
        EXPECT_EQ(0, out8x8[(1 * 8 + 1) * 4 + 3]);
        /* Opaque texel (1,0) adjacent to transparent (0,0) must NOT darken from black bleed. */
        EXPECT_EQ(200, out8x8[(0 * 8 + 2) * 4 + 0]);
        EXPECT_EQ(255, out8x8[(0 * 8 + 2) * 4 + 3]);
        /* Top-left subpixel of (1,1) at (2,2) faces W=(0,1) red and N=(1,0) red across NW=(0,0) -> reconstructs diagonal! */
        EXPECT_TRUE(out8x8[(2 * 8 + 2) * 4 + 0] > 100);
        EXPECT_EQ(255, out8x8[(2 * 8 + 2) * 4 + 3]);

        EXPECT_EQ(1, TextureUpscaleRGBA(src4x4, 4, 4, 4, NULL, out16x16,
                                        sizeof(out16x16)));
        EXPECT_EQ(0, out16x16[(0 * 16 + 0) * 4 + 3]);
        EXPECT_EQ(255, out16x16[(0 * 16 + 4) * 4 + 3]);
    }
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
