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
#include "game/menu_scripts_internal.h"

/* Everything the shop keeps on the display whichever state it is in. */
static void DrawEngineerShopChrome(s32 price) {
    DrawEngineerShopPricePanel(1, g_PlayerMoney, price);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
    RunTimedDrawScript(g_EngineerShopScreenScript, &g_UiScriptProgress, 0);
}

/* Backing out, and choosing the row that backs out, wind down the same way. */
static void LeaveEngineerShop(void) {
    PlaySoundCue(3);
    GameMenuBusy = 1;
    g_MenuOverlayPattern = 2;
}

/* Idle: two rows, tune up or leave. */
static void UpdateEngineerShopInput(s32 price) {
    g_MenuOverlayPattern = -1;
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
            if (g_PlayerMoney >= price) {
                PlaySoundCue(2);
                g_EngineerShopModalScript = g_EngineerShopTuneUpPromptScript;
                GameMenuBusy = -1;
                g_UiScriptProgress2 = 0;
                g_MenuSubCursor = 0;
            } else {
                PlaySoundCue(5);
                g_EngineerShopModalScript = g_EngineerShopNoFundsScript;
                GameMenuBusy = -3;
                g_UiScriptProgress2 = 0;
            }
        } else if (g_EngineerShopOption == 1) {
            LeaveEngineerShop();
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        LeaveEngineerShop();
    }
}

static void UpdateEngineerShopIdle(s32 price) {
    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    DrawEngineerShopChrome(price);
    if ((RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) &&
        (g_UiScriptProgress2 <= 0)) {
        UpdateEngineerShopInput(price);
    }
}

/* The tune-up prompt, with its own yes/no cursor. */
static void UpdateTuneUpPrompt(void *ot) {
    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) == 0) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        if (g_MenuSubCursor != 0) {
            PlaySoundCue(2);
            GameMenuBusy = -2;
            g_MenuConfirmTimer = 0x23;
            RequestUpgradedCarModel(g_PlayerCarIndex);
        } else {
            PlaySoundCue(3);
            GameMenuBusy = 0;
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
    DrawShopPromptButtons(ot, 0);
}

/*
 * The tune-up going through: the prompt flashes for a while, then the car
 * keeps its new variant and the screen starts on its way out, spinning the
 * turntable a half turn so the rebuilt car comes back round.
 */
static void UpdateTuneUpCountdown(void *ot) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer -= 1;
        RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1);
        DrawShopPromptButtons(ot, 1);
        return;
    }
    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (g_UiScriptProgress2 <= 0) {
        g_MenuViewAngle = 0x927C0;
        g_MenuViewAngleTarget = 0;
        GameMenuBusy = 2;
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
            GameMenuBusy = 0;
        }
    }
}

static void UpdateEngineerShopModal(void *ot, s32 price) {
    if (GameMenuBusy == -1) {
        UpdateTuneUpPrompt(ot);
    } else if (GameMenuBusy == -2) {
        UpdateTuneUpCountdown(ot);
    } else {
        UpdateNoFundsModal();
    }
    DrawEngineerShopChrome(price);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

/* On the way out, back to the car select screen. The tune-up is paid for and
 * recorded here, so it only counts once the screen has actually finished. */
static void UpdateEngineerShopOutgoing(s32 price) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = 0xC;
    DrawEngineerShopPricePanel(-1, g_PlayerMoney, price);
    RunTimedDrawScript(g_EngineerShopScreenScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
    if (g_UiScriptProgress > 0) {
        return;
    }
    if (GameMenuBusy == 2) {
        CarEntry *car = &g_CarTable[g_PlayerCarIndex];

        car->modelVariant++;
        /* The time attack copy keeps the best the car has ever been. */
        if (car->modelVariant > g_TimeAttackCars[g_PlayerCarIndex].modelVariant) {
            g_TimeAttackCars[g_PlayerCarIndex].modelVariant = car->modelVariant;
        }
        g_PlayerMoney -= price;
    }
    g_MenuScreen = 4;
    g_MenuHandlerIndex = 4;
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
    g_EngineerShopOption = 0;
}

void UpdateEngineerShopScreen(void) {
    void *ot = RENDER_OT_BASE;
    s32 price;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    g_MenuPlateCarIndex = g_PlayerCarIndex;
    price = g_CarTuneUpPriceTable[GetOwnedCarAssetIndex(g_PlayerCarIndex)];

    if (GameMenuBusy == 0) {
        UpdateEngineerShopIdle(price);
    } else if (GameMenuBusy < 0) {
        UpdateEngineerShopModal(ot, price);
    } else {
        UpdateEngineerShopOutgoing(price);
    }
}
