#include "game/asset.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"

static u16 CarGearTextureU(u8 gearCount) {
    switch (gearCount) {
    case 4:
        return 0x20;
    case 5:
        return 0x28;
    case 6:
        return 0x30;
    default:
        return 0;
    }
}

s32 DrawCarSelectScreen(s32 step) {
    GameOrderingTableEntry *ot;
    u8 brightness;
    s16 gearX;
    u16 gearTextureU;
    u8 transmission;

    if (step == 0) {
        AdvanceMenuFade(&g_CarSelectFadeAccum, step);
        return 0;
    }
    AdvanceMenuFade(&g_CarSelectFadeAccum, step);

    if (g_CarTable == NULL || g_CarModelAsset == NULL ||
        (u32)g_PlayerCarIndex >= GAME_CAR_COUNT || RENDER_OT_BASE == NULL) {
        return g_CarSelectFadeAccum;
    }
    gearTextureU = CarGearTextureU(g_CarModelAsset->gearCount);
    if (gearTextureU == 0) {
        return g_CarSelectFadeAccum;
    }

    ot = RENDER_OT_BASE + 1;
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

    DrawSprite(ot, gearX, 0x185, 8, 0x10, gearTextureU, 0x18, brightness,
               brightness, brightness, 0x244, 0, 1, 0x3B);
    return g_CarSelectFadeAccum;
}
