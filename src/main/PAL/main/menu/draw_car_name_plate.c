#include "game/car.h"
#include "game/menu.h"

typedef struct CarNamePlateSprite {
    s16 x;
    u16 width;
    u16 textureU;
    u16 textureV;
} CarNamePlateSprite;

enum CarManufacturer {
    CAR_MANUFACTURER_AGE,
    CAR_MANUFACTURER_LIZARD,
    CAR_MANUFACTURER_ASSOLUTO,
    CAR_MANUFACTURER_GNADE,
};

enum {
    CAR_NAME_PLATE_FADE_MAX = 508,
    CAR_NAME_PLATE_SHADE_DIVISOR = 4,
};

static const CarNamePlateSprite s_manufacturerSprites[] = {
    [CAR_MANUFACTURER_AGE] = {0x112, 0x14, 0x50, 0xBC},
    [CAR_MANUFACTURER_LIZARD] = {0x105, 0x20, 0x00, 0xBC},
    [CAR_MANUFACTURER_ASSOLUTO] = {0x106, 0x20, 0x64, 0xBC},
    [CAR_MANUFACTURER_GNADE] = {0x0F6, 0x30, 0x20, 0xBC},
};

static const u8 s_carManufacturers[GAME_CAR_COUNT] = {
    CAR_MANUFACTURER_AGE,      CAR_MANUFACTURER_AGE,
    CAR_MANUFACTURER_AGE,      CAR_MANUFACTURER_LIZARD,
    CAR_MANUFACTURER_ASSOLUTO, CAR_MANUFACTURER_ASSOLUTO,
    CAR_MANUFACTURER_ASSOLUTO, CAR_MANUFACTURER_GNADE,
    CAR_MANUFACTURER_GNADE,    CAR_MANUFACTURER_GNADE,
    CAR_MANUFACTURER_AGE,      CAR_MANUFACTURER_ASSOLUTO,
    CAR_MANUFACTURER_GNADE,
};

static const CarNamePlateSprite s_carNameSprites[GAME_CAR_COUNT] = {
    {0x0FC, 0x2A, 0x0A, 0x30}, {0x106, 0x20, 0x48, 0x30},
    {0x106, 0x20, 0x7C, 0x30}, {0x0F2, 0x34, 0x00, 0x40},
    {0x0FD, 0x28, 0x74, 0x50}, {0x0FC, 0x2A, 0x3E, 0x50},
    {0x107, 0x20, 0xB0, 0x50}, {0x0FE, 0x28, 0x40, 0x40},
    {0x104, 0x22, 0x7A, 0x40}, {0x0F7, 0x30, 0xA0, 0x40},
    {0x0FA, 0x2C, 0xA4, 0x30}, {0x0FC, 0x2A, 0x0A, 0x60},
    {0x0F6, 0x30, 0x04, 0x50},
};

static void DrawNamePlateSprite(GameOrderingTableEntry *ot,
                                const CarNamePlateSprite *sprite, s16 y,
                                u8 shade, u32 flags) {
    DrawSprite(ot, sprite->x, y, sprite->width, 0x10, sprite->textureU,
               sprite->textureV, shade, shade, shade, 0x244, 0, 1, flags);
}

static void AdvanceCarNamePlateFade(s32 step) {
    int64_t fade = (int64_t)g_CarNamePlateFade + step;

    if (fade < 0) fade = 0;
    if (fade > CAR_NAME_PLATE_FADE_MAX) fade = CAR_NAME_PLATE_FADE_MAX;
    g_CarNamePlateFade = (s32)fade;
}

/* The bottom-right plate: grade digit, manufacturer sprite and model-name sprite. */
void DrawCarNamePlate(s32 step, s32 model, s32 grade) {
    GameOrderingTableEntry *ot;
    s32 unlockLevel;
    int64_t displayedGrade;
    u32 shade;

    if (step == 0) {
        g_CarNamePlateFade = 0;
        return;
    }
    if (step < 0) {
        AdvanceCarNamePlateFade(step);
    }

    unlockLevel = GetCarUnlockLevel(model);
    displayedGrade = (int64_t)unlockLevel + grade;
    if ((u32)model >= GAME_CAR_COUNT || unlockLevel < 0 || grade < 0 ||
        RENDER_OT_BASE == NULL) {
        if (step > 0) {
            AdvanceCarNamePlateFade(step);
        }
        return;
    }

    ot = RENDER_OT_BASE + 1;
    shade = g_CarNamePlateFade / CAR_NAME_PLATE_SHADE_DIVISOR;
    DrawSprite(ot, 0x100, 0x168, 0x20, 0x10, 0x7C, 0x7C, (u8)shade,
                  (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);

    if (displayedGrade >= 5) {
        DrawSprite(ot, 0x11F, 0x168, 8, 0x10, 0x38, 0x28, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);
    } else {
        DrawSprite(ot, 0x11F, 0x168, 8, 0x10,
                   (s16)(displayedGrade * 8 + 8), 0x18, (u8)shade,
                   (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);
    }

    DrawNamePlateSprite(ot,
                        &s_manufacturerSprites[s_carManufacturers[model]],
                        0x178, (u8)shade, 0x3B);
    DrawNamePlateSprite(ot, &s_carNameSprites[model], 0x188, (u8)shade,
                        0x3E);

    if (step > 0) {
        AdvanceCarNamePlateFade(step);
    }
}
