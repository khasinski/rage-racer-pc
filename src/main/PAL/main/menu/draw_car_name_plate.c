#include "game/car.h"
#include "game/menu.h"

/* The bottom-right plate: grade digit, manufacturer sprite and model-name sprite. */
void DrawCarNamePlate(s32 step, s32 model, s32 grade) {
    GameOrderingTableEntry *ot;
    u32 idx;
    u32 shade;

    ot = RENDER_OT_BASE_AS(GameOrderingTableEntry) + 1;
    if (step == 0) {
        g_CarNamePlateFade = 0;
        return;
    }
    if (step < 0) {
        g_CarNamePlateFade += step;
        if (g_CarNamePlateFade < 0) {
            g_CarNamePlateFade = 0;
        }
    }

    shade = g_CarNamePlateFade / 4U;
    DrawSprite(ot, 0x100, 0x168, 0x20, 0x10, 0x7C, 0x7C, (u8)shade,
                  (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);

    idx = (GetCarUnlockLevel(model) + grade) & 0xFFFF;
    if (idx >= 5) {
        DrawSprite(ot, 0x11F, 0x168, 8, 0x10, 0x38, 0x28, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);
    } else {
        DrawSprite(ot, 0x11F, 0x168, 8, 0x10, (s16)((idx * 8) + 8), 0x18,
                      (u8)shade, (u8)shade, (u8)shade, 0x244, 0, 1,
                      0x3B);
    }

    switch (model) {
    case 0:
    case 1:
    case 2:
    case 10:
        DrawSprite(ot, 0x112, 0x178, 0x14, 0x10, 0x50, 0xBC, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);
        break;
    case 3:
        DrawSprite(ot, 0x105, 0x178, 0x20, 0x10, 0, 0xBC, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);
        break;
    case 4:
    case 5:
    case 6:
    case 11:
        DrawSprite(ot, 0x106, 0x178, 0x20, 0x10, 0x64, 0xBC, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);
        break;
    case 7:
    case 8:
    case 9:
    case 12:
        DrawSprite(ot, 0xF6, 0x178, 0x30, 0x10, 0x20, 0xBC, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3B);
        break;
    default:
        break;
    }

    switch (model) {
    case 0:
        DrawSprite(ot, 0xFC, 0x188, 0x2A, 0x10, 0xA, 0x30, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 1:
        DrawSprite(ot, 0x106, 0x188, 0x20, 0x10, 0x48, 0x30, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 2:
        DrawSprite(ot, 0x106, 0x188, 0x20, 0x10, 0x7C, 0x30, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 10:
        DrawSprite(ot, 0xFA, 0x188, 0x2C, 0x10, 0xA4, 0x30, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 3:
        DrawSprite(ot, 0xF2, 0x188, 0x34, 0x10, 0, 0x40, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 4:
        DrawSprite(ot, 0xFD, 0x188, 0x28, 0x10, 0x74, 0x50, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 5:
        DrawSprite(ot, 0xFC, 0x188, 0x2A, 0x10, 0x3E, 0x50, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 6:
        DrawSprite(ot, 0x107, 0x188, 0x20, 0x10, 0xB0, 0x50, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 11:
        DrawSprite(ot, 0xFC, 0x188, 0x2A, 0x10, 0xA, 0x60, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 7:
        DrawSprite(ot, 0xFE, 0x188, 0x28, 0x10, 0x40, 0x40, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 8:
        DrawSprite(ot, 0x104, 0x188, 0x22, 0x10, 0x7A, 0x40, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 9:
        DrawSprite(ot, 0xF7, 0x188, 0x30, 0x10, 0xA0, 0x40, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    case 12:
        DrawSprite(ot, 0xF6, 0x188, 0x30, 0x10, 4, 0x50, (u8)shade,
                      (u8)shade, (u8)shade, 0x244, 0, 1, 0x3E);
        break;
    }

    if (step > 0) {
        g_CarNamePlateFade += step;
        if (g_CarNamePlateFade >= 509) {
            g_CarNamePlateFade = 508;
        }
    }
}
