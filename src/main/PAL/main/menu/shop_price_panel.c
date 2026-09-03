#include "game/menu.h"
#include "game/menu_internal.h"

typedef struct ShopPriceCaption {
    u16 width;
    u16 textureU;
} ShopPriceCaption;

static void DrawShopPricePanel(s32 step, s32 money, s32 price, s32 *slide,
                               ShopPriceCaption priceCaption) {
    void *ot = RENDER_OT_BASE;
    s32 risenFrames;

    if (step == 0) {
        *slide = 0;
        return;
    }

    *slide = AddClampedMenuValue(*slide, 0, 0, SHOP_PANEL_SLIDE_MAX);

    if (step < 0) {
        *slide = AddClampedMenuValue(*slide, step, 0, SHOP_PANEL_SLIDE_MAX);
    }

    risenFrames = *slide - SHOP_PANEL_VISIBLE_AT;
    if (risenFrames >= 0 && g_MenuAltLayout == 0) {
        s32 rise;
        s16 moneyY;
        s16 priceY;

        if (risenFrames >= SHOP_PANEL_RISE_FRAMES) {
            risenFrames = SHOP_PANEL_RISE_FRAMES - 1;
        }
        rise = -(risenFrames * SHOP_PANEL_RISE_PIXELS_PER_FRAME);
        moneyY = rise + SHOP_PANEL_MONEY_TEXT_Y;
        priceY = rise + SHOP_PANEL_PRICE_TEXT_Y;

        const s32 numberFlags = DRAW_NUMBER_LARGE_DIGITS |
                                DRAW_NUMBER_TEN_DIGIT_FIELD |
                                DRAW_NUMBER_ALT_DIGIT_ATLAS;
        GameDrawNumber(0x39, moneyY, numberFlags, money, 0x7F, 0x7F, 0x7F,
                       0x259, 0x20);
        GameDrawNumber(0x39, priceY, numberFlags, price, 0x7F, 0x7F, 0x7F,
                       0x259, 0x20);
        DrawSprite(ot, 0x17, moneyY, 0x1D, 0x10, 0x1B, 0x8C, 0, 0, 0, 0x244,
                   1, 1, 0x3B);
        DrawSprite(ot, 0x18, priceY, priceCaption.width, 0x10,
                   priceCaption.textureU, 0x8C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0x89, moneyY, 0xC, 0x10, 0x50, 0xDC, 0, 0, 0, 0x259,
                   1, 1, 0x3B);
        DrawSprite(ot, 0x89, priceY, 0xC, 0x10, 0x50, 0xDC, 0, 0, 0, 0x259,
                   1, 1, 0x3B);
        GameDrawMenuButton(0, rise + SHOP_PANEL_MONEY_BOX_Y, 0x99, 0x23, 0, 0,
                           0);
        GameDrawMenuButton(0, rise + SHOP_PANEL_PRICE_BOX_Y, 0x99, 0x23, 0, 0,
                           0);
    }

    if (step > 0) {
        *slide = AddClampedMenuValue(*slide, step, 0, SHOP_PANEL_SLIDE_MAX);
    }
}

void DrawCarShopPricePanel(s32 step, s32 money, s32 price) {
    const ShopPriceCaption caption = {0x18, 0x3C};

    DrawShopPricePanel(step, money, price, &g_CarShopPanelSlide, caption);
}

void DrawEngineerShopPricePanel(s32 step, s32 money, s32 price) {
    const ShopPriceCaption caption = {0x34, 0x54};

    DrawShopPricePanel(step, money, price, &g_EngineerShopPanelSlide, caption);
}
