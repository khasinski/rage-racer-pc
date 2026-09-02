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

/* Which of the four buy prompts a car gets. Cars past the table get none. */
static const TimedDrawCommand *CarShopBuyPrompt(s32 car) {
    switch (car) {
    case 0:
    case 1:
    case 2:
    case 10:
        return g_CarShopBuyPromptScript1;
    case 3:
        return g_CarShopBuyPromptScript2;
    case 4:
    case 5:
    case 6:
    case 11:
        return g_CarShopBuyPromptScript3;
    case 7:
    case 8:
    case 9:
    case 12:
        return g_CarShopBuyPromptScript4;
    }
    return NULL;
}

/* Everything the shop keeps on the display whichever state it is in. */
static void DrawCarShopChrome(s32 price, s32 chromeStep) {
    DrawBrowseArrows(1, 0, g_PrevOwnedCarIndex != -1,
                     g_NextOwnedCarIndex != -1);
    DrawCarShopPricePanel(1, g_PlayerMoney, price);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
    RunTimedDrawScript(g_CarShopScreenScript, &g_UiScriptProgress, 0);
    if (chromeStep >= 0) {
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, chromeStep);
    }
}

/* Leaving the shop, either by choosing to or by backing out. */
static void LeaveCarShop(s32 busyState) {
    if (g_PlayerCarIndex != g_CarListCursor) {
        MenuSpinBackToPlayerCar();
    }
    PlaySoundCue(3);
    GameMenuBusy = busyState;
    g_MenuOverlayPattern = 2;
    g_MenuAltPanelStep = -1;
    g_MenuAltPanelStep2 = -1;
}

/* Confirm on the car itself: put up the buy prompt, unless it is already
 * owned, in which case there is nothing to buy. */
static void OfferToBuyCar(void) {
    const TimedDrawCommand *prompt;

    if (g_CarTable[g_CarListCursor].enabled != 0) {
        return;
    }
    PlaySoundCue(2);
    GameMenuBusy = -1;
    g_UiScriptProgress2 = 0;
    g_MenuSubCursor = 0;
    prompt = CarShopBuyPrompt(g_CarListCursor);
    if (prompt != NULL) {
        g_CarShopModalScript = prompt;
    }
}

/* Idle: the pad browses the cars and picks one of the two rows. */
static void UpdateCarShopInput(void) {
    s32 carBeforeSwap;

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_CarShopOption = (g_CarShopOption > 0) ? g_CarShopOption - 1 : 1;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_CarShopOption = (g_CarShopOption <= 0) ? g_CarShopOption + 1 : 0;
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
                      0x124F80);
    }

    /* The second panel only opens for a car whose gearbox can be changed. */
    g_MenuAltPanelStep = (g_CarModelAsset->transmissionAvailable == 0) ? 1 : -1;

    if (!MenuCarViewSettled() || (g_CarSwapToIndex >= 0)) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        if (g_CarShopOption == 1) {
            LeaveCarShop(1);
        } else if (g_CarShopOption == 0) {
            OfferToBuyCar();
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        LeaveCarShop(1);
    }
}

static void UpdateCarShopIdle(s32 price) {
    g_MenuPlateCarIndex = g_CarListCursor;
    RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    DrawCarShopChrome(price, -1);
    if ((RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) &&
        (g_UiScriptProgress2 <= 0)) {
        UpdateCarShopInput();
    }
}

/* The buy prompt, and the refusal that replaces it when the money is short.
 * Only the prompt itself takes input; the refusal just waits to be dismissed. */
static void UpdateBuyPrompt(void *ot, s32 price) {
    RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) == 0) {
        return;
    }
    if (GameMenuBusy == -1) {
        if (g_PadPressed & PAD_CONFIRM) {
            if (g_MenuSubCursor == 0) {
                PlaySoundCue(3);
                GameMenuBusy = 0;
            } else if (g_PlayerMoney >= price) {
                PlaySoundCue(2);
                GameMenuBusy = -3;
                g_MenuConfirmTimer = 0x23;
            } else {
                PlaySoundCue(5);
                g_CarShopModalScript = g_CarShopNoFundsScript;
                GameMenuBusy = -2;
            }
        }
        if (g_PadPressed & PAD_CANCEL) {
            PlaySoundCue(3);
            GameMenuBusy = 0;
        }
        /* Left picks yes, right picks no, and neither repeats itself. */
        if ((g_PadPressed & PAD_LEFT) && (g_MenuSubCursor == 0)) {
            PlaySoundCue(1);
            g_MenuSubCursor = 1;
        }
        if ((g_PadPressed & PAD_RIGHT) && (g_MenuSubCursor != 0)) {
            PlaySoundCue(1);
            g_MenuSubCursor = 0;
        }
    } else if (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) {
        GameMenuBusy = 0;
    }
    DrawShopPromptButtons(ot, 0);
}

/* The sale going through: the prompt flashes for a while, then the car is
 * marked owned and the screen starts on its way out. */
static void UpdateSaleCountdown(void *ot) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer -= 1;
        RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1);
        DrawShopPromptButtons(ot, 1);
        return;
    }
    RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (g_UiScriptProgress2 <= 0) {
        g_CarTable[g_CarListCursor].enabled = 1;
        g_TimeAttackCarEnabled[g_CarListCursor * 8] = 1;
        GameMenuBusy = 2;
        g_MenuAltPanelStep = -1;
        g_PlayerCarIndex = g_CarListCursor;
    }
}

static void UpdateCarShopModal(void *ot, s32 price) {
    if ((GameMenuBusy == -1) || (GameMenuBusy == -2)) {
        UpdateBuyPrompt(ot, price);
    } else if (GameMenuBusy == -3) {
        UpdateSaleCountdown(ot);
    }
    DrawCarShopChrome(price, 1);
}

/* On the way out, back to the car select screen. A sale is paid for here, so
 * the money only leaves once the screen has actually finished. */
static void UpdateCarShopOutgoing(s32 price) {
    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 0xB;
    DrawBrowseArrows(-1, 0, g_PrevOwnedCarIndex != -1,
                     g_NextOwnedCarIndex != -1);
    DrawCarShopPricePanel(-1, g_PlayerMoney, price);
    RunTimedDrawScript(g_CarShopScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
    if (g_UiScriptProgress > 0) {
        return;
    }
    if (GameMenuBusy == 2) {
        g_PlayerMoney -= price;
    }
    g_MenuScreen = 4;
    g_MenuHandlerIndex = 4;
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
    g_CarShopOption = 0;
    UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
    UploadTeamLogoClut();
}

void UpdateCarShopScreen(void) {
    void *ot = RENDER_OT_BASE;
    s32 price;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuAltPanel(g_MenuAltPanelStep, g_MenuAltPanelStep2);
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    price = g_CarPriceTable[GetOwnedCarAssetIndex(g_CarListCursor)];

    if (GameMenuBusy == 0) {
        UpdateCarShopIdle(price);
    } else if (GameMenuBusy < 0) {
        UpdateCarShopModal(ot, price);
    } else {
        UpdateCarShopOutgoing(price);
    }
}
