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
    /* Retail builds the four-stop ramp from thirds of the endpoint sum,
     * rather than using conventional linear interpolation. */
    case 2: return (uint8_t)((primary + secondary) * 2u / 3u);
    case 3: return middle;
    case 4: return (uint8_t)((primary + secondary) / 3u);
    case 5: return (uint8_t)(middle / 2u + secondaryHalf);
    default: return secondary;
    }
}

static int PaintTargetRGB(uint8_t code, uint8_t firstColor,
                          uint8_t secondColor, uint8_t rgbOut[3]) {
    const RagePaintEndpoints *paint;
    uint8_t shade, channel;
    if (code == RAGE_CAR_PAINT_NONE ||
        code > RAGE_CAR_PAINT_SECOND_SECONDARY) return 0;
    if (code >= RAGE_CAR_PAINT_SECOND_PRIMARY) {
        paint = &s_colors[secondColor];
        shade = (uint8_t)(code - RAGE_CAR_PAINT_SECOND_PRIMARY);
    } else {
        paint = &s_colors[firstColor];
        shade = (uint8_t)(code - RAGE_CAR_PAINT_FIRST_PRIMARY);
    }
    for (channel = 0; channel < 3; channel++)
        rgbOut[channel] = PaintExpand(PaintShade(
            paint->primary[channel], paint->secondary[channel], shade));
    return 1;
}

static void PaintPixel(uint8_t *rgba, uint8_t code, uint8_t firstColor,
                       uint8_t secondColor) {
    uint8_t rgb[3];
    if (!PaintTargetRGB(code, firstColor, secondColor, rgb)) return;
    rgba[0] = rgb[0];
    rgba[1] = rgb[1];
    rgba[2] = rgb[2];
}

int CarPaintApply(uint8_t *rgba, const uint8_t *mask, size_t pixelCount,
                      uint8_t firstColor, uint8_t secondColor) {
    size_t pixel;
    if (rgba == NULL || mask == NULL ||
        pixelCount > SIZE_MAX / 4u ||
        firstColor >= RAGE_CAR_PAINT_COLOR_COUNT ||
        secondColor >= RAGE_CAR_PAINT_COLOR_COUNT) return 0;
    for (pixel = 0; pixel < pixelCount; pixel++)
        PaintPixel(rgba + pixel * 4u, mask[pixel], firstColor, secondColor);
    return 1;
}

int CarPaintApplyScaled(uint8_t *rgba, uint32_t width, uint32_t height,
                        const uint8_t *mask, uint32_t maskWidth,
                        uint32_t maskHeight, uint8_t firstColor,
                        uint8_t secondColor) {
    uint32_t y, x;
    if (rgba == NULL || mask == NULL || width == 0 || height == 0 ||
        maskWidth == 0 || maskHeight == 0 ||
        (size_t)width > SIZE_MAX / 4u ||
        (size_t)height > SIZE_MAX / ((size_t)width * 4u) ||
        (size_t)maskHeight > SIZE_MAX / (size_t)maskWidth ||
        firstColor >= RAGE_CAR_PAINT_COLOR_COUNT ||
        secondColor >= RAGE_CAR_PAINT_COLOR_COUNT) return 0;
    if (width == maskWidth && height == maskHeight)
        return CarPaintApply(rgba, mask, (size_t)width * (size_t)height,
                             firstColor, secondColor);
    for (y = 0; y < height; y++) {
        /* Map high-res pixel center to 256-fixed-point mask coordinates. */
        int64_t fy256 =
            (((int64_t)y * 2 + 1) * (int64_t)maskHeight * 128) / (int64_t)height -
            128;
        int64_t y0 = fy256 >> 8;
        uint32_t ty = (uint32_t)(fy256 & 255);
        uint32_t my0, my1;
        uint8_t *rgbaRow = rgba + (size_t)y * (size_t)width * 4u;
        if (fy256 < 0) {
            my0 = my1 = 0;
            ty = 0;
        } else if ((uint64_t)y0 >= (uint64_t)(maskHeight - 1u)) {
            my0 = my1 = maskHeight - 1u;
            ty = 0;
        } else {
            my0 = (uint32_t)y0;
            my1 = my0 + 1u;
        }
        for (x = 0; x < width; x++) {
            int64_t fx256 =
                (((int64_t)x * 2 + 1) * (int64_t)maskWidth * 128) /
                    (int64_t)width -
                128;
            int64_t x0 = fx256 >> 8;
            uint32_t tx = (uint32_t)(fx256 & 255);
            uint32_t mx0, mx1;
            uint8_t c00, c10, c01, c11;
            uint32_t w[4];
            uint8_t codes[4];
            uint32_t paintW = 0;
            uint32_t sumR = 0, sumG = 0, sumB = 0;
            uint8_t *px = rgbaRow + (size_t)x * 4u;
            int i;
            if (fx256 < 0) {
                mx0 = mx1 = 0;
                tx = 0;
            } else if ((uint64_t)x0 >= (uint64_t)(maskWidth - 1u)) {
                mx0 = mx1 = maskWidth - 1u;
                tx = 0;
            } else {
                mx0 = (uint32_t)x0;
                mx1 = mx0 + 1u;
            }
            c00 = mask[(size_t)my0 * (size_t)maskWidth + (size_t)mx0];
            c10 = mask[(size_t)my0 * (size_t)maskWidth + (size_t)mx1];
            c01 = mask[(size_t)my1 * (size_t)maskWidth + (size_t)mx0];
            c11 = mask[(size_t)my1 * (size_t)maskWidth + (size_t)mx1];
            if (c00 == c10 && c00 == c01 && c00 == c11) {
                PaintPixel(px, c00, firstColor, secondColor);
                continue;
            }
            w[0] = (256u - tx) * (256u - ty);
            w[1] = tx * (256u - ty);
            w[2] = (256u - tx) * ty;
            w[3] = tx * ty;
            codes[0] = c00;
            codes[1] = c10;
            codes[2] = c01;
            codes[3] = c11;
            for (i = 0; i < 4; i++) {
                uint8_t rgb[3];
                if (w[i] == 0) continue;
                if (PaintTargetRGB(codes[i], firstColor, secondColor, rgb)) {
                    paintW += w[i];
                    sumR += w[i] * (uint32_t)rgb[0];
                    sumG += w[i] * (uint32_t)rgb[1];
                    sumB += w[i] * (uint32_t)rgb[2];
                }
            }
            if (paintW == 0) continue;
            {
                uint32_t paintR = (sumR + paintW / 2u) / paintW;
                uint32_t paintG = (sumG + paintW / 2u) / paintW;
                uint32_t paintB = (sumB + paintW / 2u) / paintW;
                if (paintW >= 65536u) {
                    px[0] = (uint8_t)paintR;
                    px[1] = (uint8_t)paintG;
                    px[2] = (uint8_t)paintB;
                } else {
                    uint32_t unpW = 65536u - paintW;
                    px[0] = (uint8_t)((paintR * paintW +
                                       (uint32_t)px[0] * unpW + 32768u) >>
                                      16);
                    px[1] = (uint8_t)((paintG * paintW +
                                       (uint32_t)px[1] * unpW + 32768u) >>
                                      16);
                    px[2] = (uint8_t)((paintB * paintW +
                                       (uint32_t)px[2] * unpW + 32768u) >>
                                      16);
                }
            }
        }
    }
    return 1;
}
