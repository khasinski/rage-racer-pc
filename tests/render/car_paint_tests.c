#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/car_paint.h"

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
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
