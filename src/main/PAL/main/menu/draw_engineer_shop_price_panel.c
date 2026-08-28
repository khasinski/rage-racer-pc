#include "game/menu.h"

/*
 * The ENGINEER SHOP twin of DrawCarShopPricePanel: same slide geometry, its
 * own counter, and a wider caption sprite on the lower row.
 */
void DrawEngineerShopPricePanel(s32 step, s32 money, s32 price) {
    void *ot = SCRATCH_OT_BASE;
    s32 slide;
    s32 risenFrames;
    u32 rise;
    s16 moneyY;
    s16 priceY;

    if (step == 0) {
        g_EngineerShopPanelSlide = 0;
    } else {
        if (step < 0) {
            g_EngineerShopPanelSlide += step;
            if (g_EngineerShopPanelSlide < 0) {
                g_EngineerShopPanelSlide = 0;
            }
        }
        slide = g_EngineerShopPanelSlide;
        risenFrames = slide - SHOP_PANEL_VISIBLE_AT;
        if (risenFrames >= 0 && g_MenuAltLayout == 0) {
            if (risenFrames >= SHOP_PANEL_RISE_FRAMES) {
                risenFrames = SHOP_PANEL_RISE_FRAMES - 1;
            }
            rise = (u32)-(risenFrames * SHOP_PANEL_RISE_PER_FRAME) >> SHOP_PANEL_RISE_SHIFT;
            moneyY = rise + SHOP_PANEL_MONEY_TEXT_Y;
            GameDrawNumber(0x39, moneyY, 7, money, 0x7F, 0x7F, 0x7F, 0x259, 0x20);
            priceY = rise + SHOP_PANEL_PRICE_TEXT_Y;
            GameDrawNumber(0x39, priceY, 7, price, 0x7F, 0x7F, 0x7F, 0x259, 0x20);
            DrawSprite(ot, 0x17, moneyY, 0x1D, 0x10, 0x1B, 0x8C, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x18, priceY, 0x34, 0x10, 0x54, 0x8C, 0, 0, 0, 0x244, 1, 1, 0x3B);
            DrawSprite(ot, 0x89, moneyY, 0xC, 0x10, 0x50, 0xDC, 0, 0, 0, 0x259, 1, 1, 0x3B);
            DrawSprite(ot, 0x89, priceY, 0xC, 0x10, 0x50, 0xDC, 0, 0, 0, 0x259, 1, 1, 0x3B);
            GameDrawMenuButton(0, (s16)(rise + SHOP_PANEL_MONEY_BOX_Y), 0x99, 0x23, 0, 0, 0, 0, 0, 0, 0);
            GameDrawMenuButton(0, (s16)(rise + SHOP_PANEL_PRICE_BOX_Y), 0x99, 0x23, 0, 0, 0, 0, 0, 0, 0);
        }

        if (step > 0) {
            g_EngineerShopPanelSlide += step;
            if (g_EngineerShopPanelSlide > SHOP_PANEL_SLIDE_MAX) {
                g_EngineerShopPanelSlide = SHOP_PANEL_SLIDE_MAX;
            }
        }
    }
}
