#include "car_paint.h"

typedef struct RagePaintEndpoints {
    uint8_t primary[3];
    uint8_t secondary[3];
} RagePaintEndpoints;

static const RagePaintEndpoints s_colors[RAGE_CAR_PAINT_COLOR_COUNT] = {
    {{19, 19, 15}, {9, 9, 6}}, {{0, 4, 8}, {0, 2, 4}},
    {{20, 16, 2}, {12, 9, 2}}, {{18, 18, 18}, {8, 8, 8}},
    {{19, 10, 4}, {11, 6, 2}}, {{6, 7, 8}, {2, 3, 4}},
    {{2, 14, 5}, {0, 8, 2}}, {{16, 6, 4}, {8, 3, 1}},
    {{8, 10, 16}, {4, 5, 8}}, {{17, 4, 4}, {4, 3, 3}},
    {{4, 6, 11}, {1, 2, 5}}, {{20, 7, 7}, {10, 4, 4}},
    {{3, 3, 3}, {1, 1, 1}}, {{17, 9, 15}, {7, 3, 6}},
    {{5, 6, 4}, {1, 2, 0}}, {{19, 17, 9}, {10, 8, 2}},
    {{12, 16, 18}, {3, 6, 8}}, {{11, 13, 9}, {4, 5, 3}},
};

static uint8_t PaintExpand(uint8_t value) {
    return (uint8_t)((value << 3) | (value >> 2));
}

static uint8_t PaintShade(uint8_t primary, uint8_t secondary,
                              uint8_t shade) {
    uint8_t primaryHalf = primary / 2u;
    uint8_t secondaryHalf = secondary / 2u;
    uint8_t middle = (uint8_t)(primaryHalf + secondaryHalf);
    switch (shade) {
    case 0: return primary;
    case 1: return (uint8_t)(primaryHalf + middle / 2u);
    case 2: return (uint8_t)((primary * 2u + secondary) / 3u);
    case 3: return middle;
    case 4: return (uint8_t)((primary + secondary * 2u) / 3u);
    case 5: return (uint8_t)(middle / 2u + secondaryHalf);
    default: return secondary;
    }
}

int CarPaintApply(uint8_t *rgba, const uint8_t *mask, size_t pixelCount,
                      uint8_t firstColor, uint8_t secondColor) {
    size_t pixel;
    if (rgba == NULL || mask == NULL ||
        firstColor >= RAGE_CAR_PAINT_COLOR_COUNT ||
        secondColor >= RAGE_CAR_PAINT_COLOR_COUNT) return 0;
    for (pixel = 0; pixel < pixelCount; pixel++) {
        uint8_t code = mask[pixel];
        const RagePaintEndpoints *paint;
        uint8_t shade, channel;
        if (code == RAGE_CAR_PAINT_NONE ||
            code > RAGE_CAR_PAINT_SECOND_SECONDARY) continue;
        if (code >= RAGE_CAR_PAINT_SECOND_PRIMARY) {
            paint = &s_colors[secondColor];
            shade = (uint8_t)(code - RAGE_CAR_PAINT_SECOND_PRIMARY);
        } else {
            paint = &s_colors[firstColor];
            shade = (uint8_t)(code - RAGE_CAR_PAINT_FIRST_PRIMARY);
        }
        for (channel = 0; channel < 3; channel++)
            rgba[pixel * 4u + channel] = PaintExpand(PaintShade(
                paint->primary[channel], paint->secondary[channel], shade));
    }
    return 1;
}
