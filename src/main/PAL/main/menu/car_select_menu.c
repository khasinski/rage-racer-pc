#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"

s32 DrawCarSelectScreen(s32 step) {
    void *ot;
    u8 brightness;
    s16 gearX;
    u16 gearTextureU;
    u8 transmission;

    if (step == 0) {
        g_CarSelectFadeAccum = 0;
        return 0;
    }

    g_CarSelectFadeAccum += step;
    if (g_CarSelectFadeAccum >= MENU_FADE_COMPLETE) {
        g_CarSelectFadeAccum = MENU_FADE_MAX;
    } else if (g_CarSelectFadeAccum < 0) {
        g_CarSelectFadeAccum = 0;
    }

    ot = RENDER_OT_BASE_AS(GameOrderingTableEntry) + 1;
    brightness = (u8)(g_CarSelectFadeAccum / 4);
    transmission = g_CarTable[g_PlayerCarIndex].transmission;
    DrawRectOutline(ot, 0xA3, 0x180, 0x1A, 0x19, brightness, brightness,
                    brightness, 0x20);

    if (transmission != 0) {
        DrawSprite(ot, 0xAD, 0x185, 0x10, 0x10, 0x6C, 0x7C, brightness,
                   brightness, brightness, 0x244, 0, 1, 0x3B);
        gearX = 0xA5;
    } else {
        DrawSprite(ot, 0xAE, 0x185, 0xC, 0x10, 0x60, 0x7C, brightness,
                   brightness, brightness, 0x244, 0, 1, 0x3B);
        gearX = 0xA6;
    }

    switch (g_CarModelAsset->gearCount) {
    case 4:
        gearTextureU = 0x20;
        break;
    case 5:
        gearTextureU = 0x28;
        break;
    case 6:
        gearTextureU = 0x30;
        break;
    default:
        return g_CarSelectFadeAccum;
    }
    DrawSprite(ot, gearX, 0x185, 8, 0x10, gearTextureU, 0x18, brightness,
               brightness, brightness, 0x244, 0, 1, 0x3B);
    return g_CarSelectFadeAccum;
}
