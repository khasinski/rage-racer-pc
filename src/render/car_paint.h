#ifndef RAGE_CAR_PAINT_H
#define RAGE_CAR_PAINT_H

#include <stddef.h>
#include <stdint.h>

enum {
    RAGE_CAR_PAINT_NONE = 0,
    RAGE_CAR_PAINT_FIRST_PRIMARY = 1,
    RAGE_CAR_PAINT_FIRST_QUARTER = 2,
    RAGE_CAR_PAINT_FIRST_THIRD = 3,
    RAGE_CAR_PAINT_FIRST_HALF = 4,
    RAGE_CAR_PAINT_FIRST_TWO_THIRDS = 5,
    RAGE_CAR_PAINT_FIRST_THREE_QUARTERS = 6,
    RAGE_CAR_PAINT_FIRST_SECONDARY = 7,
    RAGE_CAR_PAINT_SECOND_PRIMARY = 8,
    RAGE_CAR_PAINT_SECOND_QUARTER = 9,
    RAGE_CAR_PAINT_SECOND_THIRD = 10,
    RAGE_CAR_PAINT_SECOND_HALF = 11,
    RAGE_CAR_PAINT_SECOND_TWO_THIRDS = 12,
    RAGE_CAR_PAINT_SECOND_THREE_QUARTERS = 13,
    RAGE_CAR_PAINT_SECOND_SECONDARY = 14,
    RAGE_CAR_PAINT_COLOR_COUNT = 18,
};

/* Recolour an imported RGBA material using a one-byte semantic body-paint
 * mask. The colour catalogue is game content expressed as RGB channels; no
 * texture page, CLUT or VRAM data reaches this layer. */
int RageCarPaintApply(uint8_t *rgba, const uint8_t *mask, size_t pixelCount,
                      uint8_t firstColor, uint8_t secondColor);

#endif
