#include "game/car.h"
#include "game/asset.h"

/* Body paint is 15-bit RGB with the semi-transparency bit on top. */
enum PaintColor {
    PAINT_STP = 0x8000,
    PAINT_CHANNEL_MASK = 0x1F,
    PAINT_GREEN_SHIFT = 5,
    PAINT_BLUE_SHIFT = 10,
    PAINT_HALVED_MASK = 0x3DEF,
    PAINT_COLOR_COUNT = 18,
    PAINT_GRADIENT_STOP_COUNT = 5,
    PRIMARY_THREE_STOP_SLOT_COUNT = 9,
    SECONDARY_THREE_STOP_SLOT_COUNT = 8,
    FOUR_STOP_SLOT_COUNT = 4,
    PRIMARY_THREE_STOP_COLUMN = 0,
    SECONDARY_THREE_STOP_COLUMN = 3,
    PRIMARY_FOUR_STOP_COLUMN = 0,
    SECONDARY_FOUR_STOP_COLUMN = 4,
};

typedef enum PaintLayer {
    PAINT_LAYER_PRIMARY,
    PAINT_LAYER_SECONDARY,
} PaintLayer;

typedef struct PaintGradient {
    u16 stops[PAINT_GRADIENT_STOP_COUNT];
} PaintGradient;

static u16 AveragePaintColour(u32 colour0, u32 colour1) {
    u32 half0 = (colour0 / 2) & PAINT_HALVED_MASK;
    u32 half1 = (colour1 / 2) & PAINT_HALVED_MASK;

    return (u16)((half0 + half1) | PAINT_STP);
}

static PaintGradient BuildFourStopPaintGradient(u32 colour0, u32 colour1) {
    PaintGradient gradient;
    s32 red = (colour0 & PAINT_CHANNEL_MASK) +
              (colour1 & PAINT_CHANNEL_MASK);
    s32 green = ((colour0 >> PAINT_GREEN_SHIFT) & PAINT_CHANNEL_MASK) +
                ((colour1 >> PAINT_GREEN_SHIFT) & PAINT_CHANNEL_MASK);
    s32 blue = ((colour0 >> PAINT_BLUE_SHIFT) & PAINT_CHANNEL_MASK) +
               ((colour1 >> PAINT_BLUE_SHIFT) & PAINT_CHANNEL_MASK);

    gradient.stops[0] = (u16)colour0;
    gradient.stops[1] =
        (u16)(((blue * 2 / 3) << PAINT_BLUE_SHIFT) +
              ((green * 2 / 3) << PAINT_GREEN_SHIFT) +
              red * 2 / 3 + PAINT_STP);
    gradient.stops[2] =
        (u16)(((blue / 3) << PAINT_BLUE_SHIFT) +
              ((green / 3) << PAINT_GREEN_SHIFT) + red / 3 + PAINT_STP);
    gradient.stops[3] = (u16)colour1;
    gradient.stops[4] = 0;
    return gradient;
}

static PaintGradient BuildFiveStopPaintGradient(u32 colour0, u32 colour1) {
    PaintGradient gradient;
    u32 half0 = (colour0 / 2) & PAINT_HALVED_MASK;
    u32 half1 = (colour1 / 2) & PAINT_HALVED_MASK;
    u32 mixedHalves = half0 + half1;
    u32 quarterMix = (mixedHalves >> 1) & PAINT_HALVED_MASK;

    gradient.stops[0] = (u16)colour0;
    gradient.stops[1] = (u16)(half0 + quarterMix - PAINT_STP);
    gradient.stops[2] = (u16)(mixedHalves | PAINT_STP);
    gradient.stops[3] = (u16)(quarterMix + half1 - PAINT_STP);
    gradient.stops[4] = (u16)colour1;
    return gradient;
}

static void WriteThreeStopPaintGradient(u16 *palette, const u16 *slots,
                                        s32 count, s32 column,
                                        u16 colour0, u16 colour1) {
    u16 middle = AveragePaintColour(colour0, colour1);
    s32 i;

    for (i = 0; i < count; i++) {
        u16 *destination = &palette[slots[i] + column];
        destination[0] = colour0;
        destination[1] = middle;
        destination[2] = colour1;
    }
}

static void WriteFourStopPaintGradient(u16 *palette, const u16 *slots,
                                       s32 count, s32 column,
                                       u16 colour0, u16 colour1) {
    PaintGradient gradient = BuildFourStopPaintGradient(colour0, colour1);
    s32 i;

    for (i = 0; i < count; i++) {
        u16 *destination = &palette[slots[i] + column];
        destination[0] = gradient.stops[0];
        destination[1] = gradient.stops[1];
        destination[2] = gradient.stops[2];
        destination[3] = gradient.stops[3];
    }
}

static void ApplyBodyColours(u32 colour, CarImageData *imageData,
                             PaintLayer layer) {
    if (colour >= PAINT_COLOR_COUNT) {
        colour = 0;
    }

    u16 primary = g_BodyColorPrimary[colour];
    u16 secondary = g_BodyColorSecondary[colour];
    u16 *palette = imageData->paintPalette.entries;
    PaintGradient fiveStop = BuildFiveStopPaintGradient(primary, secondary);
    s32 i;

    if (layer == PAINT_LAYER_SECONDARY) {
        WriteThreeStopPaintGradient(
            palette, g_PaintSlots3StopB, SECONDARY_THREE_STOP_SLOT_COUNT,
            SECONDARY_THREE_STOP_COLUMN, primary, secondary);
        WriteFourStopPaintGradient(
            palette, g_PaintSlots4Stop, FOUR_STOP_SLOT_COUNT,
            SECONDARY_FOUR_STOP_COLUMN, primary, secondary);
        for (i = 0; i < PAINT_GRADIENT_STOP_COUNT; i++) {
            imageData->paintPalette.gradients.bodyColor2Gradient[i] =
                fiveStop.stops[i];
        }
    } else {
        imageData->paintPalette.fixed.bodyColor1 = primary;
        WriteThreeStopPaintGradient(
            palette, g_PaintSlots3StopA, PRIMARY_THREE_STOP_SLOT_COUNT,
            PRIMARY_THREE_STOP_COLUMN, primary, secondary);
        WriteFourStopPaintGradient(
            palette, g_PaintSlots4Stop, FOUR_STOP_SLOT_COUNT,
            PRIMARY_FOUR_STOP_COLUMN, primary, secondary);
        for (i = 0; i < PAINT_GRADIENT_STOP_COUNT; i++) {
            imageData->paintPalette.gradients.bodyColor1Gradient[i] =
                fiveStop.stops[i];
        }
    }
}

void ApplyPrimaryBodyColor(u32 colour, CarImageData *imageData) {
    ApplyBodyColours(colour, imageData, PAINT_LAYER_PRIMARY);
}

void SetPrimaryBodyColor(s32 colour) {
    ApplyPrimaryBodyColor((u32)colour, g_CarModelAsset->imageData.carImage);
    UploadCarImage(g_CarModelSlot);
}

void ApplySecondaryBodyColor(u32 colour, CarImageData *imageData) {
    ApplyBodyColours(colour, imageData, PAINT_LAYER_SECONDARY);
}

void SetSecondaryBodyColor(s32 colour) {
    ApplySecondaryBodyColor((u32)colour, g_CarModelAsset->imageData.carImage);
    UploadCarImage(g_CarModelSlot);
}
