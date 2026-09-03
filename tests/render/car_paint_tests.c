#include <stdio.h>
#include <stdlib.h>

#include "render/car_paint.h"

static int failures;
#define EXPECT_EQ(expected, actual) do { if ((expected) != (actual)) {         \
    fprintf(stderr, "%s:%d: expected %u, got %u\n", __FILE__, __LINE__,     \
            (unsigned)(expected), (unsigned)(actual)); failures++;             \
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
    EXPECT_EQ(115, pixels[8]);
    EXPECT_EQ(107, pixels[12]);
    EXPECT_EQ(90, pixels[16]);
    EXPECT_EQ(8, pixels[20]);
    EXPECT_EQ(255, pixels[23]);
    EXPECT_EQ(0, CarPaintApply(pixels, mask, 6, 18, 0));
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
