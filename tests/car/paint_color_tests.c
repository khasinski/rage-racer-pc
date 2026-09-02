#include "common.h"
#include "game/asset.h"
#include "game/car.h"

#include <stdio.h>
#include <string.h>

u16 g_BodyColorPrimary[18];
u16 g_BodyColorSecondary[18];
u16 g_PaintSlots3StopA[9];
u16 g_PaintSlots3StopB[8];
u16 g_PaintSlots4Stop[4];
CarModelAsset *g_CarModelAsset;
u32 g_CarModelSlot;

void UploadCarImage(s32 slot) { (void)slot; }

static unsigned long s_digest = 2166136261UL;

static void Fold(const void *data, size_t size) {
    const u8 *bytes = data;
    size_t i;

    for (i = 0; i < size; i++) {
        s_digest ^= bytes[i];
        s_digest = (s_digest * 16777619UL) & 0xFFFFFFFFUL;
    }
}

int main(void) {
    static const u16 colours[][2] = {
        {0x8000, 0xFFFF}, {0x801F, 0x83E0}, {0xFC00, 0x8421},
        {0x94A5, 0xE739}, {0x0000, 0x7FFF},
    };
    static const unsigned long expected = 101119966UL;
    CarImageData image;
    CarImageData firstColour;
    CarImageData invalidColour;
    size_t colour;
    s32 i;

    for (i = 0; i < 9; i++) g_PaintSlots3StopA[i] = (u16)(1 + i * 8);
    for (i = 0; i < 8; i++) g_PaintSlots3StopB[i] = (u16)(3 + i * 9);
    for (i = 0; i < 4; i++) g_PaintSlots4Stop[i] = (u16)(90 + i * 12);

    for (colour = 0; colour < sizeof(colours) / sizeof(colours[0]); colour++) {
        memset(&image, 0x5A, sizeof(image));
        g_BodyColorPrimary[colour] = colours[colour][0];
        g_BodyColorSecondary[colour] = colours[colour][1];
        ApplyBodyColor1((u32)colour, &image);
        Fold(&image.paintPalette, sizeof(image.paintPalette));

        memset(&image, 0xA5, sizeof(image));
        ApplyBodyColor2((u32)colour, &image);
        Fold(&image.paintPalette, sizeof(image.paintPalette));
    }

    memset(&firstColour, 0x5A, sizeof(firstColour));
    memset(&invalidColour, 0x5A, sizeof(invalidColour));
    ApplyBodyColor1(0, &firstColour);
    ApplyBodyColor1(18, &invalidColour);
    if (memcmp(&firstColour.paintPalette, &invalidColour.paintPalette,
               sizeof(firstColour.paintPalette)) != 0) {
        puts("FAIL invalid paint colour did not use the first palette entry");
        return 1;
    }

    if (s_digest != expected) {
        printf("FAIL paint colour digest %lu, expected %lu\n",
               s_digest, expected);
        return 1;
    }
    puts("body paint gradients update every palette stop");
    return 0;
}
