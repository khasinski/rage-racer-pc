#include "game/car.h"
#include "game/asset.h"

/*
 * Body paint is 15-bit RGB with the semi-transparency bit on top. HALVED_MASK
 * is what a channel-wise halving needs after the shift: `>> 1` drags the low
 * bit of each channel into the one below it, and this clears those three bits
 * again.
 */
enum PaintColor {
    PAINT_STP = 0x8000,
    PAINT_CHANNEL_MASK = 0x1F,
    PAINT_GREEN_SHIFT = 5,
    PAINT_BLUE_SHIFT = 10,
    PAINT_HALVED_MASK = 0x3DEF
};

/*
 * g_PaintBlendShade0 / g_PaintBlendShade1 (the two blended shade words this routine emits)
 * MUST keep the raw D_ spelling: they are referenced from the %hi/%lo pairs in
 * the inline asm below, which does not follow asm() labels.
 */
void BlendPaintColor(u32 color0, u32 color1) {
    u32 a;
    u32 b;

    a = color0 / 2;
    b = color1 / 2;
    a &= PAINT_HALVED_MASK;
    b &= PAINT_HALVED_MASK;
    g_PaintBlendShade0 = (a + b) | PAINT_STP;
}

void BlendPaintColorThirds(u32 color0, u32 color1) {
    u16 low0;
    u16 low1;
    s32 r0;
    s32 r1;
    s32 g0;
    s32 g1;
    s32 b0;
    s32 b1;
    s32 r;
    s32 g;
    s32 b;

    r0 = color0 & PAINT_CHANNEL_MASK;
    r1 = color1 & PAINT_CHANNEL_MASK;
    r = r0 + r1;
    low0 = color0;
    g0 = (low0 >> PAINT_GREEN_SHIFT) & PAINT_CHANNEL_MASK;
    low1 = color1;
    g1 = (low1 >> PAINT_GREEN_SHIFT) & PAINT_CHANNEL_MASK;
    g = g0 + g1;
    b0 = (low0 >> PAINT_BLUE_SHIFT) & PAINT_CHANNEL_MASK;
    b1 = (low1 >> PAINT_BLUE_SHIFT) & PAINT_CHANNEL_MASK;
    b = b0 + b1;

    r0 = r * 2 / 3;
    g0 = g * 2 / 3;
    b0 = b * 2 / 3;
    r1 = r / 3;
    g1 = g / 3;
    b1 = b / 3;

    g_PaintBlendShade0 = (b0 << PAINT_BLUE_SHIFT) + (g0 * 32) + r0 + PAINT_STP;
    g_PaintBlendShade1 = (b1 << PAINT_BLUE_SHIFT) + (g1 * 32) + r1 + PAINT_STP;
}

void BlendPaintColorQuarters(u32 color0, u32 color1) {
    u32 a;
    u32 b;
    u32 c;
    u32 high;
    s32 bias;

    a = color0 / 2;
    b = color1 / 2;
    a &= PAINT_HALVED_MASK;
    b &= PAINT_HALVED_MASK;
    c = a + b;
    high = c;
    bias = PAINT_STP;
    high += bias;
    c >>= 1;
    c &= PAINT_HALVED_MASK;
    a += c;
    a -= bias;
    c += b;
    c -= bias;

    g_PaintBlendShade1 = high;
    g_PaintBlendShade0 = a;
    g_PaintBlendShade2 = c;
}

/*
 * ApplyBodyColor1.
 *
 * The store to raw+0x7162 written just before the call is scheduled by the
 * compiler into the jal delay slot; no hand-written asm is needed.
 */

void ApplyBodyColor1(u32 colour, CarImageData *imageData) {
    u16 *base;
    u16 s1;
    u16 s2;
    volatile u16 *idx;
    volatile u16 *color;
    const u16 *primary;
    const u16 *secondary;
    CarPaintPaletteAddress paletteAddress;
    u16 c;
    s32 i;

    primary = &g_BodyColorPrimary[colour];
    secondary = &g_BodyColorSecondary[colour];
    base = imageData->paintPalette.entries;
    s1 = *primary;
    s2 = *secondary;

    imageData->paintPalette.fixed.bodyColor1 = s1;
    BlendPaintColor(s1, s2);

    i = 0;
    color = &g_PaintBlendShade0;
    idx = g_PaintSlots3StopA;
    for (; i < 9; idx++) {
        i++;
        base[idx[0] + 0] = s1;
        base[idx[0] + 1] = color[0];
        base[idx[0] + 2] = s2;
    }

    BlendPaintColorThirds(s1, s2);

    i = 0;
    color = &g_PaintBlendShade0;
    idx = g_PaintSlots4Stop;
    for (; i < 4; idx++) {
        i++;
        base[idx[0] + 0] = s1;
        base[idx[0] + 1] = color[0];
        base[idx[0] + 2] = color[1];
        base[idx[0] + 3] = s2;
    }

    BlendPaintColorQuarters(s1, s2);

    paletteAddress.entries = base;
    paletteAddress.palette->gradients.bodyColor1Gradient[0] = s1;
    paletteAddress.palette->gradients.bodyColor1Gradient[1] = g_PaintBlendShade0;
    paletteAddress.palette->gradients.bodyColor1Gradient[2] = g_PaintBlendShade1;
    c = g_PaintBlendShade2;
    paletteAddress.palette->gradients.bodyColor1Gradient[4] = s2;
    paletteAddress.palette->gradients.bodyColor1Gradient[3] = c;
}


void SetBodyColor1(u32 colour) {
    ApplyBodyColor1(colour, g_CarModelAsset->imageData.carImage);
    UploadCarImage(g_CarModelSlot);
}

void ApplyBodyColor2(u32 colour, CarImageData *imageData) {
    u16 *base;
    u16 s1;
    u16 s2;
    volatile u16 *idx;
    volatile u16 *color;
    u16 c;
    s32 i;
    CarPaintPaletteAddress paletteAddress;

    base = imageData->paintPalette.entries;
    s1 = g_BodyColorPrimary[colour];
    s2 = g_BodyColorSecondary[colour];

    BlendPaintColor(s1, s2);

    i = 0;
    color = &g_PaintBlendShade0;
    idx = g_PaintSlots3StopB;
    for (; i < 8; idx++) {
        i++;
        base[idx[0] + 3] = s1;
        base[idx[0] + 4] = color[0];
        base[idx[0] + 5] = s2;
    }

    BlendPaintColorThirds(s1, s2);

    i = 0;
    color = &g_PaintBlendShade0;
    idx = g_PaintSlots4Stop;
    for (; i < 4; idx++) {
        i++;
        base[idx[0] + 4] = s1;
        base[idx[0] + 5] = color[0];
        base[idx[0] + 6] = color[1];
        base[idx[0] + 7] = s2;
    }

    BlendPaintColorQuarters(s1, s2);

    paletteAddress.entries = base;
    paletteAddress.palette->gradients.bodyColor2Gradient[0] = s1;
    paletteAddress.palette->gradients.bodyColor2Gradient[1] = g_PaintBlendShade0;
    paletteAddress.palette->gradients.bodyColor2Gradient[2] = g_PaintBlendShade1;
    c = g_PaintBlendShade2;
    paletteAddress.palette->gradients.bodyColor2Gradient[4] = s2;
    paletteAddress.palette->gradients.bodyColor2Gradient[3] = c;
}


void SetBodyColor2(u32 colour) {
    ApplyBodyColor2(colour, g_CarModelAsset->imageData.carImage);
    UploadCarImage(g_CarModelSlot);
}
