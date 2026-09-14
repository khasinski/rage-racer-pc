/*
 * The car shop: the only place in the game where money changes hands.
 *
 * The player browses the cars they do not own on the same turntable the select
 * screen uses, and buying one goes through a prompt with its own yes/no
 * cursor, a refusal if they cannot afford it, and a short countdown while the
 * sale goes through. GameMenuBusy tells the states apart: zero is idle, the
 * negatives are the prompt and its countdown, the positives are the way out.
 */

#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"

static CarShop s_shop;

void ResetCarShopScreen(void) {
    s_shop = (CarShop){g_UiEmptyScript, 0};
}

/* Everything the shop keeps on the display whichever state it is in. */
static void DrawCarShopChrome(const CarShop *shop, s32 price, s32 chromeStep) {
    DrawBrowseArrows(MenuBrowseArrows(), 1, 0, g_PrevOwnedCarIndex != -1,
                     g_NextOwnedCarIndex != -1);
    DrawCarShopPricePanel(1, g_PlayerMoney, price);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, shop->option);
    RunTimedDrawScript(g_CarShopScreenScript, &g_UiScriptProgress, 0);
    if (chromeStep >= 0) {
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, chromeStep);
    }
}

/* Leaving the shop, either by choosing to or by backing out. */
static void LeaveCarShop(void) {
    if (g_PlayerCarIndex != g_CarListCursor) {
        MenuSpinBackToPlayerCar();
    }
    PlaySoundCue(3);
    GameMenuBusy = CAR_SHOP_LEAVE;
    g_MenuOverlayPattern = 2;
    g_MenuUpperAltPanelStep = -1;
    g_MenuLowerAltPanelStep = -1;
}

/* Confirm on the car itself: put up the buy prompt, unless it is already
 * owned, in which case there is nothing to buy. */
static void OfferToBuyCar(CarShop *shop, s32 purchaseAvailable) {
    const TimedDrawCommand *prompt;

    if (!purchaseAvailable || g_CarTable[g_CarListCursor].enabled != 0) {
        return;
    }
    PlaySoundCue(2);
    GameMenuBusy = CAR_SHOP_BUY_PROMPT;
    g_UiScriptProgress2 = 0;
    g_MenuSubCursor = 0;
    prompt = CarShopPrompt(GetCarMaker(g_CarListCursor));
    shop->modal = prompt;
}

/* Idle: the pad browses the cars and picks one of the two rows. */
static void UpdateCarShopInput(CarShop *shop, s32 purchaseAvailable) {
    s32 carBeforeSwap;

    g_MenuOverlayPattern = -1;
    shop->option = AddClampedMenuValue(shop->option, 0, 0, 1);
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        shop->option = (shop->option > 0) ? shop->option - 1 : 1;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        shop->option = (shop->option <= 0) ? shop->option + 1 : 0;
    }
    UpdateCarListCursor();

    carBeforeSwap = g_CarListCursor;
    if ((g_PadHeld & PAD_LEFT) && (g_PrevOwnedCarIndex != -1) &&
        MenuCarViewSettled() && (g_CarSwapToIndex < 0)) {
        MenuSpinToCar(&g_CarListCursor, carBeforeSwap, g_PrevOwnedCarIndex, 0);
    }
    if ((g_PadHeld & PAD_RIGHT) && (g_NextOwnedCarIndex != -1) &&
        MenuCarViewSettled() && (g_CarSwapToIndex < 0)) {
        MenuSpinToCar(&g_CarListCursor, carBeforeSwap, g_NextOwnedCarIndex,
                      MENU_CAR_VIEW_RIGHT_TARGET);
    }

    /* The upper panel only opens for a car whose gearbox can be changed. */
    g_MenuUpperAltPanelStep =
        (g_CarModelAsset != NULL &&
         g_CarModelAsset->transmissionAvailable == 0)
            ? 1
            : -1;

    if (!MenuCarViewSettled() || (g_CarSwapToIndex >= 0)) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        if (shop->option == 0) {
            OfferToBuyCar(shop, purchaseAvailable);
        } else {
            LeaveCarShop();
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        LeaveCarShop();
    }
}

static void UpdateCarShopIdle(CarShop *shop, ShopPrice price) {
    g_MenuPlateCarIndex = g_CarListCursor;
    RunTimedDrawScript(shop->modal, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    DrawCarShopChrome(shop, price.amount, -1);
    if ((RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) &&
        (g_UiScriptProgress2 <= 0)) {
        UpdateCarShopInput(shop, price.available);
    }
}

/* The buy prompt, and the refusal that replaces it when the money is short.
 * Only the prompt itself takes input; the refusal just waits to be dismissed. */
static void UpdateBuyPrompt(CarShop *shop, GameOrderingTableEntry *ot,
                            ShopPrice price) {
    MenuDialogAction action;

    RunTimedDrawScript(shop->modal, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) == 0) {
        return;
    }
    g_MenuSubCursor = (u8)AddClampedMenuValue(g_MenuSubCursor, 0, 0, 1);
    if (GameMenuBusy == CAR_SHOP_BUY_PROMPT) {
        action = ChooseMenuDialogAction(g_PadPressed);
        if (action == MENU_DIALOG_CONFIRM) {
            if (g_MenuSubCursor == 0) {
                PlaySoundCue(3);
                GameMenuBusy = CAR_SHOP_IDLE;
            } else if (price.available && g_PlayerMoney >= price.amount) {
                PlaySoundCue(2);
                GameMenuBusy = CAR_SHOP_SALE_COUNTDOWN;
                g_MenuConfirmTimer = 0x23;
            } else {
                PlaySoundCue(5);
                shop->modal = g_CarShopNoFundsScript;
                GameMenuBusy = CAR_SHOP_NO_FUNDS;
            }
        } else if (action == MENU_DIALOG_CANCEL) {
            PlaySoundCue(3);
            GameMenuBusy = CAR_SHOP_IDLE;
        } else if (action == MENU_DIALOG_LEFT && g_MenuSubCursor == 0) {
            PlaySoundCue(1);
            g_MenuSubCursor = 1;
        } else if (action == MENU_DIALOG_RIGHT && g_MenuSubCursor != 0) {
            PlaySoundCue(1);
            g_MenuSubCursor = 0;
        }
    } else if (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) {
        GameMenuBusy = CAR_SHOP_IDLE;
    }
    DrawShopPromptButtons(ot, 0);
}

/* The sale going through: the prompt flashes for a while, then the car is
 * marked owned and the screen starts on its way out. */
static void UpdateSaleCountdown(CarShop *shop, GameOrderingTableEntry *ot,
                                s32 purchaseAvailable) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer -= 1;
        RunTimedDrawScript(shop->modal, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1);
        DrawShopPromptButtons(ot, 1);
        return;
    }
    RunTimedDrawScript(shop->modal, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (g_UiScriptProgress2 > 0) {
        return;
    }
    if (!purchaseAvailable) {
        GameMenuBusy = CAR_SHOP_IDLE;
    } else {
        g_CarTable[g_CarListCursor].enabled = 1;
        g_TimeAttackCars[g_CarListCursor].enabled = 1;
        GameMenuBusy = CAR_SHOP_LEAVE_AFTER_SALE;
        g_MenuUpperAltPanelStep = -1;
        g_PlayerCarIndex = g_CarListCursor;
    }
}

static void UpdateCarShopModal(CarShop *shop, GameOrderingTableEntry *ot,
                               ShopPrice price) {
    if ((GameMenuBusy == CAR_SHOP_BUY_PROMPT) ||
        (GameMenuBusy == CAR_SHOP_NO_FUNDS)) {
        UpdateBuyPrompt(shop, ot, price);
    } else if (GameMenuBusy == CAR_SHOP_SALE_COUNTDOWN) {
        UpdateSaleCountdown(shop, ot, price.available);
    } else {
        GameMenuBusy = CAR_SHOP_IDLE;
    }
    DrawCarShopChrome(shop, price.amount, 1);
}

/* On the way out, back to the car select screen. A sale is paid for here, so
 * the money only leaves once the screen has actually finished. */
static void UpdateCarShopOutgoing(CarShop *shop, ShopPrice price) {
    MenuBeginExit(MENU_SCREEN_CAR_SHOP);
    DrawBrowseArrows(MenuBrowseArrows(), -1, 0, g_PrevOwnedCarIndex != -1,
                     g_NextOwnedCarIndex != -1);
    DrawCarShopPricePanel(-1, g_PlayerMoney, price.amount);
    RunTimedDrawScript(g_CarShopScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, shop->option);
    if (g_UiScriptProgress > 0) {
        return;
    }
    if (GameMenuBusy == CAR_SHOP_LEAVE_AFTER_SALE && price.available) {
        g_PlayerMoney -= price.amount;
    }
    MenuActivateScreen(MENU_SCREEN_CAR_SELECT);
    g_UiScriptProgress = 0;
    GameMenuBusy = CAR_SHOP_IDLE;
    shop->option = 0;
    UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
    UploadTeamLogoClut();
}

void UpdateCarShop(CarShop *shop) {
    GameOrderingTableEntry *ot = RENDER_OT_BASE;
    ShopPrice price;
    s32 assetIndex;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuAltPanel(g_MenuUpperAltPanelStep, g_MenuLowerAltPanelStep);
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex);
    DrawMenuCarView();
    if ((u32)g_CarListCursor >= GAME_CAR_COUNT || g_CarTable == NULL) {
        price = (ShopPrice){0, 0};
    } else {
        assetIndex = GetOwnedCarAssetIndex(g_CarListCursor);
        price = LookupShopPrice(g_CarPriceTable, CAR_PRICE_COUNT, assetIndex);
    }

    if (GameMenuBusy == CAR_SHOP_IDLE) {
        UpdateCarShopIdle(shop, price);
    } else if (GameMenuBusy < 0) {
        UpdateCarShopModal(shop, ot, price);
    } else {
        UpdateCarShopOutgoing(shop, price);
    }
}

void UpdateCarShopScreen(void) { UpdateCarShop(&s_shop); }
