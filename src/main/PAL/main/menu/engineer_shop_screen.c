/*
 * The engineer's shop: the other place the player's money goes.
 *
 * A tune-up raises the car's model variant by one, permanently, and if it
 * beats the best that car has ever had it raises the time attack copy too.
 * The shop has the same shape as the car shop: GameMenuBusy is zero while it
 * is idle, negative while the prompt, its refusal or the countdown is up, and
 * positive on the way out.
 */

#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"

enum { ENGINEER_SHOP_TURNTABLE_HALF_TURN = 0x927C0 };

/* Everything the shop keeps on the display whichever state it is in. */
static void DrawEngineerShopChrome(s32 price) {
    DrawEngineerShopPricePanel(1, g_PlayerMoney, price);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
    RunTimedDrawScript(g_EngineerShopScreenScript, &g_UiScriptProgress, 0);
}

/* Backing out, and choosing the row that backs out, wind down the same way. */
static void LeaveEngineerShop(void) {
    PlaySoundCue(3);
    GameMenuBusy = ENGINEER_SHOP_LEAVE;
    g_MenuOverlayPattern = 2;
}

/* Idle: two rows, tune up or leave. */
static void UpdateEngineerShopInput(ShopPrice price) {
    g_MenuOverlayPattern = -1;
    g_EngineerShopOption = AddClampedMenuValue(g_EngineerShopOption, 0, 0, 1);
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_EngineerShopOption =
            (g_EngineerShopOption > 0) ? g_EngineerShopOption - 1 : 1;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_EngineerShopOption =
            (g_EngineerShopOption <= 0) ? g_EngineerShopOption + 1 : 0;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        if (g_EngineerShopOption == 0) {
            if (price.available && g_PlayerMoney >= price.amount) {
                PlaySoundCue(2);
                g_EngineerShopModalScript = g_EngineerShopTuneUpPromptScript;
                GameMenuBusy = ENGINEER_SHOP_TUNE_UP_PROMPT;
                g_UiScriptProgress2 = 0;
                g_MenuSubCursor = 0;
            } else {
                PlaySoundCue(5);
                g_EngineerShopModalScript = g_EngineerShopNoFundsScript;
                GameMenuBusy = ENGINEER_SHOP_NO_FUNDS;
                g_UiScriptProgress2 = 0;
            }
        } else if (g_EngineerShopOption == 1) {
            LeaveEngineerShop();
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        LeaveEngineerShop();
    }
}

static void UpdateEngineerShopIdle(ShopPrice price) {
    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    DrawEngineerShopChrome(price.amount);
    if ((RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) &&
        (g_UiScriptProgress2 <= 0)) {
        UpdateEngineerShopInput(price);
    }
}

/* The tune-up prompt, with its own yes/no cursor. */
static void UpdateTuneUpPrompt(void *ot, ShopPrice price) {
    MenuDialogAction action;

    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) == 0) {
        return;
    }
    g_MenuSubCursor = (u8)AddClampedMenuValue(g_MenuSubCursor, 0, 0, 1);
    action = ChooseMenuDialogAction(g_PadPressed);
    if (action == MENU_DIALOG_CONFIRM) {
        if (g_MenuSubCursor != 0 && price.available &&
            g_PlayerMoney >= price.amount) {
            if (!RequestUpgradedCarModel(g_PlayerCarIndex)) {
                DrawShopPromptButtons(ot, 0);
                return;
            }
            PlaySoundCue(2);
            GameMenuBusy = ENGINEER_SHOP_TUNE_UP_COUNTDOWN;
            g_MenuConfirmTimer = 0x23;
        } else if (g_MenuSubCursor != 0) {
            PlaySoundCue(5);
            g_EngineerShopModalScript = g_EngineerShopNoFundsScript;
            GameMenuBusy = ENGINEER_SHOP_NO_FUNDS;
        } else {
            PlaySoundCue(3);
            GameMenuBusy = ENGINEER_SHOP_IDLE;
        }
    } else if (action == MENU_DIALOG_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = ENGINEER_SHOP_IDLE;
    } else if (action == MENU_DIALOG_LEFT && g_MenuSubCursor == 0) {
        PlaySoundCue(1);
        g_MenuSubCursor = 1;
    } else if (action == MENU_DIALOG_RIGHT && g_MenuSubCursor != 0) {
        PlaySoundCue(1);
        g_MenuSubCursor = 0;
    }
    DrawShopPromptButtons(ot, 0);
}

/*
 * The tune-up going through: the prompt flashes for a while, then the car
 * keeps its new variant and the screen starts on its way out, spinning the
 * turntable a half turn so the rebuilt car comes back round.
 */
static void UpdateTuneUpCountdown(void *ot, s32 purchaseAvailable) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer -= 1;
        RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1);
        DrawShopPromptButtons(ot, 1);
        return;
    }
    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (g_UiScriptProgress2 <= 0 && !purchaseAvailable) {
        GameMenuBusy = ENGINEER_SHOP_IDLE;
    } else if (g_UiScriptProgress2 <= 0) {
        g_MenuViewAngle = ENGINEER_SHOP_TURNTABLE_HALF_TURN;
        g_MenuViewAngleTarget = 0;
        GameMenuBusy = ENGINEER_SHOP_LEAVE_AFTER_TUNE_UP;
        g_MenuOverlayPattern = 2;
        g_CarSwapFromIndex = g_PlayerCarIndex;
        g_CarSwapToIndex = g_PlayerCarIndex;
    }
}

/* "You cannot afford this": nothing to do but dismiss it. */
static void UpdateNoFundsModal(void) {
    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
        if (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) {
            GameMenuBusy = ENGINEER_SHOP_IDLE;
        }
    }
}

static void UpdateEngineerShopModal(void *ot, ShopPrice price) {
    if (GameMenuBusy == ENGINEER_SHOP_TUNE_UP_PROMPT) {
        UpdateTuneUpPrompt(ot, price);
    } else if (GameMenuBusy == ENGINEER_SHOP_TUNE_UP_COUNTDOWN) {
        UpdateTuneUpCountdown(ot, price.available);
    } else if (GameMenuBusy == ENGINEER_SHOP_NO_FUNDS) {
        UpdateNoFundsModal();
    } else {
        GameMenuBusy = ENGINEER_SHOP_IDLE;
    }
    DrawEngineerShopChrome(price.amount);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

/* On the way out, back to the car select screen. The tune-up is paid for and
 * recorded here, so it only counts once the screen has actually finished. */
static void UpdateEngineerShopOutgoing(ShopPrice price) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_ENGINEER_SHOP;
    DrawEngineerShopPricePanel(-1, g_PlayerMoney, price.amount);
    RunTimedDrawScript(g_EngineerShopScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
    if (g_UiScriptProgress > 0) {
        return;
    }
    if (GameMenuBusy == ENGINEER_SHOP_LEAVE_AFTER_TUNE_UP && price.available) {
        CarEntry *car = &g_CarTable[g_PlayerCarIndex];

        car->modelVariant++;
        /* The time attack copy keeps the best the car has ever been. */
        if (car->modelVariant > g_TimeAttackCars[g_PlayerCarIndex].modelVariant) {
            g_TimeAttackCars[g_PlayerCarIndex].modelVariant = car->modelVariant;
        }
        g_PlayerMoney -= price.amount;
    }
    g_MenuScreen = MENU_SCREEN_CAR_SELECT;
    g_MenuHandlerIndex = MENU_SCREEN_CAR_SELECT;
    g_UiScriptProgress = 0;
    GameMenuBusy = ENGINEER_SHOP_IDLE;
    g_EngineerShopOption = 0;
}

void UpdateEngineerShopScreen(void) {
    void *ot = RENDER_OT_BASE;
    ShopPrice price;
    s32 assetIndex;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    g_MenuPlateCarIndex = g_PlayerCarIndex;
    if ((u32)g_PlayerCarIndex >= GAME_CAR_COUNT || g_CarTable == NULL) {
        price = (ShopPrice){0, 0};
    } else {
        assetIndex = GetOwnedCarAssetIndex(g_PlayerCarIndex);
        price = LookupShopPrice(g_CarTuneUpPriceTable,
                                CAR_TUNE_UP_PRICE_COUNT, assetIndex);
        if (g_CarTable[g_PlayerCarIndex].modelVariant == UINT8_MAX) {
            price.available = 0;
        }
    }

    if (GameMenuBusy == ENGINEER_SHOP_IDLE) {
        UpdateEngineerShopIdle(price);
    } else if (GameMenuBusy < 0) {
        UpdateEngineerShopModal(ot, price);
    } else {
        UpdateEngineerShopOutgoing(price);
    }
}
