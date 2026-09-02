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
static s32 s_uploadCalls;
static s32 s_uploadedSlot;

void UploadCarImage(s32 slot) {
    s_uploadCalls++;
    s_uploadedSlot = slot;
}

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
    static const unsigned long expected = 2732080646UL;
    CarImageData image;
    CarImageData firstColour;
    CarImageData invalidColour;
    CarImageData activeImage;
    CarModelAsset activeModel;
    static const u16 slots3A[9] = {
        1, 0x41, 0xC1, 0x101, 0x181, 0x241, 0x281, 0x301, 0x341,
    };
    static const u16 slots3B[8] = {
        1, 0x41, 0xC1, 0x181, 0x241, 0x281, 0x301, 0x341,
    };
    static const u16 slots4[4] = {0x141, 0x1C1, 0x201, 0x401};
    size_t colour;

    memcpy(g_PaintSlots3StopA, slots3A, sizeof(slots3A));
    memcpy(g_PaintSlots3StopB, slots3B, sizeof(slots3B));
    memcpy(g_PaintSlots4Stop, slots4, sizeof(slots4));

    for (colour = 0; colour < sizeof(colours) / sizeof(colours[0]); colour++) {
        memset(&image, 0x5A, sizeof(image));
        g_BodyColorPrimary[colour] = colours[colour][0];
        g_BodyColorSecondary[colour] = colours[colour][1];
        ApplyPrimaryBodyColor((u32)colour, &image);
        Fold(&image.paintPalette, sizeof(image.paintPalette));

        memset(&image, 0xA5, sizeof(image));
        ApplySecondaryBodyColor((u32)colour, &image);
        Fold(&image.paintPalette, sizeof(image.paintPalette));
    }

    memset(&firstColour, 0x5A, sizeof(firstColour));
    memset(&invalidColour, 0x5A, sizeof(invalidColour));
    ApplyPrimaryBodyColor(0, &firstColour);
    ApplyPrimaryBodyColor(18, &invalidColour);
    if (memcmp(&firstColour.paintPalette, &invalidColour.paintPalette,
               sizeof(firstColour.paintPalette)) != 0) {
        puts("FAIL invalid paint colour did not use the first palette entry");
        return 1;
    }

    memset(&activeImage, 0x5A, sizeof(activeImage));
    memset(&activeModel, 0, sizeof(activeModel));
    activeModel.imageData.carImage = &activeImage;
    g_CarModelAsset = &activeModel;
    g_CarModelSlot = 7;
    s_uploadCalls = 0;
    s_uploadedSlot = -1;
    SetPrimaryBodyColor(0);
    if (s_uploadCalls != 1 || s_uploadedSlot != 7 ||
        activeImage.paintPalette.gradients.bodyColor1Gradient[0] !=
            g_BodyColorPrimary[0] ||
        activeImage.paintPalette.gradients.bodyColor1Gradient[4] !=
            g_BodyColorSecondary[0]) {
        printf("FAIL primary paint upload: calls=%d slot=%d ends=%u/%u\n",
               s_uploadCalls, s_uploadedSlot,
               activeImage.paintPalette.gradients.bodyColor1Gradient[0],
               activeImage.paintPalette.gradients.bodyColor1Gradient[4]);
        return 1;
    }
    SetSecondaryBodyColor(0);
    if (s_uploadCalls != 2 || s_uploadedSlot != 7 ||
        activeImage.paintPalette.gradients.bodyColor2Gradient[0] !=
            g_BodyColorPrimary[0] ||
        activeImage.paintPalette.gradients.bodyColor2Gradient[4] !=
            g_BodyColorSecondary[0]) {
        printf("FAIL secondary paint upload: calls=%d slot=%d ends=%u/%u\n",
               s_uploadCalls, s_uploadedSlot,
               activeImage.paintPalette.gradients.bodyColor2Gradient[0],
               activeImage.paintPalette.gradients.bodyColor2Gradient[4]);
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
