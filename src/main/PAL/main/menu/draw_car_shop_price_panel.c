#include "game/menu.h"

/*
 * The two-row money panel of the CAR SHOP screen: the player's balance on the
 * upper row, the price of the highlighted car below it. `step` drives a 0..25
 * slide counter; the panel is only on screen from count 11 onwards, and rises
 * by 35 lines a frame over the eleven frames after that.
 */
void DrawCarShopPricePanel(s32 step, s32 money, s32 price) {
    void *ot;
    s32 slide;
    s32 risenFrames;
    s16 moneyY;
    s16 priceY;
    u32 rise;
    u32 risePhase;

    ot = RENDER_OT_BASE;

    if (step == 0) {
        g_CarShopPanelSlide = 0;
        return;
    }
    if (step < 0) {
        slide = step + g_CarShopPanelSlide;
        g_CarShopPanelSlide = slide;
        if (slide < 0) {
            g_CarShopPanelSlide = 0;
        }
    }
    slide = g_CarShopPanelSlide;
    risenFrames = slide - SHOP_PANEL_VISIBLE_AT;
    if (risenFrames >= 0 && g_MenuAltLayout == 0) {
        if (risenFrames >= SHOP_PANEL_RISE_FRAMES) {
            risenFrames = SHOP_PANEL_RISE_FRAMES - 1;
        }
        risePhase = risenFrames;
        rise = (risePhase * -SHOP_PANEL_RISE_PER_FRAME) >> SHOP_PANEL_RISE_SHIFT;
        moneyY = rise + SHOP_PANEL_MONEY_TEXT_Y;
        priceY = rise + SHOP_PANEL_PRICE_TEXT_Y;
        GameDrawNumber(0x39, moneyY, 7, money, 0x7F, 0x7F, 0x7F, 0x259, 0x20);
        GameDrawNumber(0x39, priceY, 7, price, 0x7F, 0x7F, 0x7F, 0x259, 0x20);
        DrawSprite(ot, 0x17, moneyY, 0x1D, 0x10, 0x1B, 0x8C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0x18, priceY, 0x18, 0x10, 0x3C, 0x8C, 0, 0, 0, 0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0x89, moneyY, 0xC, 0x10, 0x50, 0xDC, 0, 0, 0, 0x259, 1, 1, 0x3B);
        DrawSprite(ot, 0x89, priceY, 0xC, 0x10, 0x50, 0xDC, 0, 0, 0, 0x259, 1, 1, 0x3B);
        GameDrawMenuButton(0, (s16)(rise + SHOP_PANEL_MONEY_BOX_Y), 0x99, 0x23, 0, 0, 0);
        GameDrawMenuButton(0, (s16)(rise + SHOP_PANEL_PRICE_BOX_Y), 0x99, 0x23, 0, 0, 0);
    }
    if (step > 0) {
        slide = step + g_CarShopPanelSlide;
        g_CarShopPanelSlide = slide;
        if (slide > SHOP_PANEL_SLIDE_MAX) {
            g_CarShopPanelSlide = SHOP_PANEL_SLIDE_MAX;
        }
    }
}
