#include "common.h"
#include "game/game_input.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/asset_internal.h"
#include "game/menu.h"
#include "game/menu_controller.h"
#include "game/menu_scripts_internal.h"
#include "game/shop_controller.h"
#include "game/render.h"
#include "game/render_workspace.h"
#include "game/state.h"
#include "game/menu_context.h"

typedef enum CarShopState {
    CAR_SHOP_PURCHASING = -3,
    CAR_SHOP_NO_FUNDS = -2,
    CAR_SHOP_PURCHASE_PROMPT = -1,
    CAR_SHOP_ACTIVE = 0,
    CAR_SHOP_BACK = 1,
    CAR_SHOP_PURCHASED = 2
} CarShopState;

static ShopScreenPhase CarShopPhaseFromLegacy(s32 phase) {
    switch (phase) {
    case CAR_SHOP_PURCHASING: return SHOP_PHASE_COMMITTING;
    case CAR_SHOP_NO_FUNDS: return SHOP_PHASE_NO_FUNDS;
    case CAR_SHOP_PURCHASE_PROMPT: return SHOP_PHASE_PURCHASE_PROMPT;
    case CAR_SHOP_BACK: return SHOP_PHASE_LEAVING;
    case CAR_SHOP_PURCHASED: return SHOP_PHASE_COMPLETED;
    default: return SHOP_PHASE_ACTIVE;
    }
}

static s32 CarShopPhaseToLegacy(ShopScreenPhase phase) {
    switch (phase) {
    case SHOP_PHASE_COMMITTING: return CAR_SHOP_PURCHASING;
    case SHOP_PHASE_NO_FUNDS: return CAR_SHOP_NO_FUNDS;
    case SHOP_PHASE_PURCHASE_PROMPT: return CAR_SHOP_PURCHASE_PROMPT;
    case SHOP_PHASE_LEAVING: return CAR_SHOP_BACK;
    case SHOP_PHASE_COMPLETED: return CAR_SHOP_PURCHASED;
    default: return CAR_SHOP_ACTIVE;
    }
}

static ShopScreenState CurrentCarShopState(void) {
    ShopScreenState state;
    state.phase = CarShopPhaseFromLegacy(GameMenuBusy);
    state.selection = g_CarShopOption;
    state.modalCursor = g_MenuSubCursor;
    state.confirmTimer = g_MenuConfirmTimer;
    return state;
}

static void ApplyCarShopState(const ShopScreenState *state) {
    GameMenuBusy = CarShopPhaseToLegacy(state->phase);
    g_CarShopOption = state->selection;
    g_MenuSubCursor = state->modalCursor;
    g_MenuConfirmTimer = state->confirmTimer;
}

static void RestoreSelectedCarModel(void) {
    if (g_PlayerCarIndex != g_CarListCursor) {
        s32 previousAngle = g_MenuViewAngleTarget;

        RequestCarModel(g_PlayerCarIndex);
        g_MenuViewAngleTarget = 0;
        g_CarSwapFromIndex = g_CarListCursor;
        g_CarSwapToIndex = g_PlayerCarIndex;
        g_MenuViewAngle = (g_MenuViewAngle - previousAngle) + 0x927C0;
    }
}

static void OpenCarPurchasePrompt(void) {
    PlaySoundCue(2);
    GameMenuBusy = CAR_SHOP_PURCHASE_PROMPT;
    g_UiScriptProgress2 = 0;
    g_MenuSubCursor = 0;
    switch (g_CarListCursor) {
    case 0:
    case 1:
    case 2:
    case 10:
        g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript1;
        break;
    case 3:
        g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript2;
        break;
    case 4:
    case 5:
    case 6:
    case 11:
        g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript3;
        break;
    case 7:
    case 8:
    case 9:
    case 12:
        g_CarShopModalScript = (u8 *)&g_CarShopBuyPromptScript4;
        break;
    }
}

static void ApplyCarShopCommand(ShopCommand command) {
    if (command == SHOP_COMMAND_OPEN_PURCHASE) {
        OpenCarPurchasePrompt();
    } else if (command == SHOP_COMMAND_BACK) {
        RestoreSelectedCarModel();
        PlaySoundCue(3);
        g_MenuOverlayPattern = 2;
        GameMenuBusy = CAR_SHOP_BACK;
        g_MenuAltPanelStep = -1;
        g_MenuAltPanelStep2 = -1;
    }
}

void UpdateCarShopScreen(void) {
    void *ot;
    s32 value;
    s32 res;
    s32 sel;
    s32 t;
    s32 u;
    u32 modalState;

    ot = RENDER_OT_BASE_AS(void);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawMenuAltPanel(g_MenuAltPanelStep, g_MenuAltPanelStep2);
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    value = g_CarPriceTable[GetOwnedCarAssetIndex(g_CarListCursor)];
    if (GameMenuBusy == CAR_SHOP_ACTIVE) {
        g_MenuPlateCarIndex = g_CarListCursor;
        RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        DrawBrowseArrows(1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        DrawCarShopPricePanel(1, g_PlayerMoney, value);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
        RunTimedDrawScript(&g_CarShopScreenScript, &g_UiScriptProgress, 0);
        {
        s32 initial;

        initial = -1;
        res = RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        if ((res != 0) && (g_UiScriptProgress2 <= 0)) {
            ShopScreenState shopState;
            ShopScreenInput shopInput;
            ShopScreenResult input;
            s32 soundIndex;

            g_MenuOverlayPattern = initial;
            shopState = CurrentCarShopState();
            shopInput.pressed = g_GameInput.pressed;
            shopInput.canOpenPurchase =
                g_CarTable[g_CarListCursor].enabled == 0;
            shopInput.showNoFundsWhenBlocked = 0;
            shopInput.hasFunds = g_PlayerMoney >= value;
            input = ShopReduceInput(&shopState, &shopInput);
            ApplyCarShopState(&input.state);
            for (soundIndex = 0; soundIndex < input.moveCount; soundIndex++) {
                PlaySoundCue(1);
            }
            UpdateCarListCursor();
            sel = g_CarListCursor;
            if ((g_GameInput.held & PAD_LEFT) && (g_PrevOwnedCarIndex != -1)) {
                t = g_MenuViewAngleTarget;
                u = g_MenuViewAngle;
                if (MenuViewIsSettled(u, t, 0x493DF)) {
                    if (g_CarSwapToIndex < 0) {
                        s32 lprev;

                        PlaySoundCue(8);
                        g_CarListCursor = g_PrevOwnedCarIndex;
                        RequestCarModel(g_PrevOwnedCarIndex);
                        lprev = g_MenuViewAngleTarget;
                        g_CarSwapFromIndex = sel;
                        g_MenuViewAngleTarget = 0;
                        g_MenuAltPanelStep2 = -1;
                        g_CarSwapToIndex = g_CarListCursor;
                        g_MenuViewAngle = (g_MenuViewAngle - lprev) + 0x927C0;
                    }
                }
            }
            if ((g_GameInput.held & PAD_RIGHT) && (g_NextOwnedCarIndex != -1)) {
                t = g_MenuViewAngleTarget;
                u = g_MenuViewAngle;
                if (MenuViewIsSettled(u, t, 0x493DF)) {
                    if (g_CarSwapToIndex < 0) {
                        s32 base;
                        s32 lprev;

                        PlaySoundCue(8);
                        g_CarListCursor = g_NextOwnedCarIndex;
                        RequestCarModel(g_NextOwnedCarIndex);
                        base = 0x927C0;
                        lprev = g_MenuViewAngleTarget;
                        g_MenuViewAngleTarget = 0x124F80;
                        g_CarSwapFromIndex = sel;
                        g_MenuAltPanelStep2 = -1;
                        g_CarSwapToIndex = g_CarListCursor;
                        g_MenuViewAngle = base - (lprev - g_MenuViewAngle);
                    }
                }
            }
            if (g_CarModelAsset->transmissionAvailable == 0) {
                g_MenuAltPanelStep = 1;
            } else {
                g_MenuAltPanelStep = -1;
            }
            t = g_MenuViewAngleTarget;
            u = g_MenuViewAngle;
            if (MenuViewIsSettled(u, t, 0x493DF)) {
                if (g_CarSwapToIndex < 0) {
                    ApplyCarShopCommand(input.command);
                    if (input.command != SHOP_COMMAND_NONE) return;
                }
            }
        }
        }
    } else {
        if (GameMenuBusy < 0) {
            modalState = GameMenuBusy + 2;
            if (modalState < 2U) {
                RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, 0);
                if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                    if (GameMenuBusy == CAR_SHOP_PURCHASE_PROMPT) {
                        ShopScreenState shopState = CurrentCarShopState();
                        ShopScreenInput shopInput;
                        ShopScreenResult dialog;
                        s32 soundIndex;

                        shopInput.pressed = g_GameInput.pressed;
                        shopInput.canOpenPurchase = 1;
                        shopInput.showNoFundsWhenBlocked = 0;
                        shopInput.hasFunds = g_PlayerMoney >= value;
                        dialog = ShopReduceInput(&shopState, &shopInput);
                        if ((dialog.effects & SHOP_EFFECT_ACCEPT) != 0) {
                            PlaySoundCue(2);
                        }
                        if ((dialog.effects & SHOP_EFFECT_CANCEL) != 0) {
                            PlaySoundCue(3);
                        }
                        if (dialog.state.phase == SHOP_PHASE_NO_FUNDS &&
                            shopState.phase != SHOP_PHASE_NO_FUNDS) {
                            PlaySoundCue(5);
                            g_CarShopModalScript =
                                (u8 *)&g_CarShopNoFundsScript;
                        }
                        ApplyCarShopState(&dialog.state);
                        for (soundIndex = 0;
                             soundIndex < dialog.moveCount; soundIndex++) {
                            PlaySoundCue(1);
                        }
                    } else {
                        ShopScreenState shopState = CurrentCarShopState();
                        ShopScreenInput shopInput;
                        ShopScreenResult dialog;
                        shopInput.pressed = g_GameInput.pressed;
                        shopInput.canOpenPurchase = 0;
                        shopInput.showNoFundsWhenBlocked = 0;
                        shopInput.hasFunds = 0;
                        dialog = ShopReduceInput(&shopState, &shopInput);
                        ApplyCarShopState(&dialog.state);
                    }
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 0);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            } else if (GameMenuBusy == CAR_SHOP_PURCHASING) {
                if (g_MenuConfirmTimer <= 0) {
                    RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, -1);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
                    if (g_UiScriptProgress2 <= 0) {
                        ShopScreenState shopState = CurrentCarShopState();
                        g_CarTable[g_CarListCursor].enabled = 1;
                        g_TimeAttackCarEnabled[g_CarListCursor * 8] = 1;
                        shopState = ShopFinishCommit(&shopState);
                        ApplyCarShopState(&shopState);
                        g_MenuAltPanelStep = -1;
                        g_PlayerCarIndex = g_CarListCursor;
                    }
                } else {
                    ShopScreenState shopState = CurrentCarShopState();
                    shopState = ShopTickConfirmTimer(&shopState);
                    ApplyCarShopState(&shopState);
                    RunTimedDrawScript(g_CarShopModalScript, &g_UiScriptProgress2, 0);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1);
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 1);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            }
            DrawBrowseArrows(1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
            DrawCarShopPricePanel(1, g_PlayerMoney, value);
            DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
            RunTimedDrawScript(&g_CarShopScreenScript, &g_UiScriptProgress, 0);
            RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
            return;
        }
        MenuFlowFadeOut(MENU_SCREEN_CAR_SHOP);
        DrawBrowseArrows(-1, 0, ~g_PrevOwnedCarIndex != 0, ~g_NextOwnedCarIndex != 0);
        DrawCarShopPricePanel(-1, g_PlayerMoney, value);
        RunTimedDrawScript(&g_CarShopScreenScript, &g_UiScriptProgress, -1);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_CarShopOption);
        if (g_UiScriptProgress <= 0) {
            if (GameMenuBusy == CAR_SHOP_PURCHASED) {
                g_PlayerMoney -= value;
            }
            MenuFlowOpen(MENU_SCREEN_CAR_SELECT);
            g_UiScriptProgress = 0;
            GameMenuBusy = CAR_SHOP_ACTIVE;
            g_CarShopOption = 0;
            UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
            UploadTeamLogoClut();
        }
    }
}

u32 DrawEngineerShopScreen(s32 step) {
    s32 value;
    s32 amount;

    if (step == 0) {
        g_EngineSpecStep = 0;
        return 0;
    }

    if (step > 0) {
        value = step + g_EngineSpecStep;
        g_EngineSpecStep = value;
        if (value >= 0x1FD) {
            g_EngineSpecStep = 0x1FC;
        }
        amount = 0;
    } else {
        s32 diff = 0x1FC;
        u32 product;

        value = step + g_EngineSpecStep;
        g_EngineSpecStep = value;
        if (value < 0) {
            g_EngineSpecStep = 0;
        }
        diff -= g_EngineSpecStep;
        product = diff * diff;
        amount = product / 2048;
    }

    DrawCarEngineSpec((s16)amount, (u8)(g_EngineSpecStep >> 2));
    return g_EngineSpecStep;
}


typedef enum EngineerShopState {
    ENGINEER_SHOP_UPGRADING = -2,
    ENGINEER_SHOP_UPGRADE_PROMPT = -1,
    ENGINEER_SHOP_ACTIVE = 0,
    ENGINEER_SHOP_BACK = 1,
    ENGINEER_SHOP_UPGRADED = 2,
    ENGINEER_SHOP_NO_FUNDS = -3
} EngineerShopState;

static ShopScreenPhase EngineerShopPhaseFromLegacy(s32 phase) {
    switch (phase) {
    case ENGINEER_SHOP_UPGRADING: return SHOP_PHASE_COMMITTING;
    case ENGINEER_SHOP_UPGRADE_PROMPT: return SHOP_PHASE_PURCHASE_PROMPT;
    case ENGINEER_SHOP_NO_FUNDS: return SHOP_PHASE_NO_FUNDS;
    case ENGINEER_SHOP_BACK: return SHOP_PHASE_LEAVING;
    case ENGINEER_SHOP_UPGRADED: return SHOP_PHASE_COMPLETED;
    default: return SHOP_PHASE_ACTIVE;
    }
}

static s32 EngineerShopPhaseToLegacy(ShopScreenPhase phase) {
    switch (phase) {
    case SHOP_PHASE_COMMITTING: return ENGINEER_SHOP_UPGRADING;
    case SHOP_PHASE_PURCHASE_PROMPT: return ENGINEER_SHOP_UPGRADE_PROMPT;
    case SHOP_PHASE_NO_FUNDS: return ENGINEER_SHOP_NO_FUNDS;
    case SHOP_PHASE_LEAVING: return ENGINEER_SHOP_BACK;
    case SHOP_PHASE_COMPLETED: return ENGINEER_SHOP_UPGRADED;
    default: return ENGINEER_SHOP_ACTIVE;
    }
}

static ShopScreenState CurrentEngineerShopState(void) {
    ShopScreenState state;
    state.phase = EngineerShopPhaseFromLegacy(GameMenuBusy);
    state.selection = g_EngineerShopOption;
    state.modalCursor = g_MenuSubCursor;
    state.confirmTimer = g_MenuConfirmTimer;
    return state;
}

static void ApplyEngineerShopState(const ShopScreenState *state) {
    GameMenuBusy = EngineerShopPhaseToLegacy(state->phase);
    g_EngineerShopOption = state->selection;
    g_MenuSubCursor = state->modalCursor;
    g_MenuConfirmTimer = state->confirmTimer;
}

void UpdateEngineerShopScreen(void) {
    void *ot;
    s32 value;
    s32 res;

    ot = RENDER_OT_BASE_AS(void);
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    g_MenuPlateCarIndex = g_PlayerCarIndex;
    value = g_CarTuneUpPriceTable[GetOwnedCarAssetIndex(g_PlayerCarIndex)];
    if (GameMenuBusy == ENGINEER_SHOP_ACTIVE) {
        RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        DrawEngineerShopPricePanel(1, g_PlayerMoney, value);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
        RunTimedDrawScript(&g_EngineerShopScreenScript, &g_UiScriptProgress, 0);
        res = RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
        if ((res != 0) && (g_UiScriptProgress2 <= 0)) {
            ShopScreenState shopState = CurrentEngineerShopState();
            ShopScreenInput shopInput;
            ShopScreenResult input;
            s32 soundIndex;

            g_MenuOverlayPattern = -1;
            shopInput.pressed = g_GameInput.pressed;
            shopInput.canOpenPurchase = g_PlayerMoney >= value;
            shopInput.showNoFundsWhenBlocked = 1;
            shopInput.hasFunds = g_PlayerMoney >= value;
            input = ShopReduceInput(&shopState, &shopInput);
            ApplyEngineerShopState(&input.state);
            for (soundIndex = 0; soundIndex < input.moveCount; soundIndex++) {
                PlaySoundCue(1);
            }
            if (input.command == SHOP_COMMAND_OPEN_PURCHASE) {
                PlaySoundCue(2);
                g_EngineerShopModalScript =
                    (u8 *)&g_EngineerShopTuneUpPromptScript;
                GameMenuBusy = ENGINEER_SHOP_UPGRADE_PROMPT;
                g_UiScriptProgress2 = 0;
                g_MenuSubCursor = 0;
            } else if (input.command == SHOP_COMMAND_NO_FUNDS) {
                PlaySoundCue(5);
                g_EngineerShopModalScript = (u8 *)&g_EngineerShopNoFundsScript;
                GameMenuBusy = ENGINEER_SHOP_NO_FUNDS;
                g_UiScriptProgress2 = 0;
            } else if (input.command == SHOP_COMMAND_BACK) {
                PlaySoundCue(3);
                GameMenuBusy = ENGINEER_SHOP_BACK;
                g_MenuOverlayPattern = 2;
            }
        }
    } else {
        if (GameMenuBusy < 0) {
            if (GameMenuBusy == ENGINEER_SHOP_UPGRADE_PROMPT) {
                RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
                if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                    ShopScreenState shopState = CurrentEngineerShopState();
                    ShopScreenInput shopInput;
                    ShopScreenResult dialog;
                    s32 soundIndex;

                    shopInput.pressed = g_GameInput.pressed;
                    shopInput.canOpenPurchase = 1;
                    shopInput.showNoFundsWhenBlocked = 0;
                    shopInput.hasFunds = 1;
                    dialog = ShopReduceInput(&shopState, &shopInput);
                    if ((dialog.effects & SHOP_EFFECT_ACCEPT) != 0) {
                        PlaySoundCue(2);
                    }
                    if ((dialog.effects & SHOP_EFFECT_CANCEL) != 0) {
                        PlaySoundCue(3);
                    }
                    if ((dialog.effects & SHOP_EFFECT_BEGIN_COMMIT) != 0) {
                        RequestUpgradedCarModel(g_PlayerCarIndex);
                    }
                    ApplyEngineerShopState(&dialog.state);
                    for (soundIndex = 0;
                         soundIndex < dialog.moveCount; soundIndex++) {
                        PlaySoundCue(1);
                    }
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 0);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            } else if (GameMenuBusy == ENGINEER_SHOP_UPGRADING) {
                if (g_MenuConfirmTimer <= 0) {
                    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, -1);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
                    if (g_UiScriptProgress2 <= 0) {
                        ShopScreenState shopState = CurrentEngineerShopState();
                        g_MenuViewAngle = 0x927C0;
                        g_MenuViewAngleTarget = 0;
                        shopState = ShopFinishCommit(&shopState);
                        ApplyEngineerShopState(&shopState);
                        g_MenuOverlayPattern = 2;
                        g_CarSwapFromIndex = g_PlayerCarIndex;
                        g_CarSwapToIndex = g_PlayerCarIndex;
                    }
                } else {
                    ShopScreenState shopState = CurrentEngineerShopState();
                    shopState = ShopTickConfirmTimer(&shopState);
                    ApplyEngineerShopState(&shopState);
                    RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
                    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1);
                    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x44, 0x20, 0x20, 1);
                    DrawSprite(ot, 0xC0, 0x4C, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    DrawSprite(ot, 0xE3, 0x4C, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1, 0x3B);
                    GameDrawMenuButton(0xB8, 0x44, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0, &g_MenuBlankCaption);
                    GameDrawMenuButton(0xDA, 0x44, 0x20, 0x20, 0x1E, 0x8E, 0x95, 0, 0, 0, &g_MenuBlankCaption);
                }
            } else {
                RunTimedDrawScript(g_EngineerShopModalScript, &g_UiScriptProgress2, 0);
                if (RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
                    ShopScreenState shopState = CurrentEngineerShopState();
                    ShopScreenInput shopInput;
                    ShopScreenResult dialog;
                    shopInput.pressed = g_GameInput.pressed;
                    shopInput.canOpenPurchase = 0;
                    shopInput.showNoFundsWhenBlocked = 0;
                    shopInput.hasFunds = 0;
                    dialog = ShopReduceInput(&shopState, &shopInput);
                    ApplyEngineerShopState(&dialog.state);
                }
            }
            DrawEngineerShopPricePanel(1, g_PlayerMoney, value);
            DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
            RunTimedDrawScript(&g_EngineerShopScreenScript, &g_UiScriptProgress, 0);
            RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
            return;
        }
        MenuFlowFadeOut(MENU_SCREEN_ENGINEER_SHOP);
        DrawEngineerShopPricePanel(-1, g_PlayerMoney, value);
        RunTimedDrawScript(&g_EngineerShopScreenScript, &g_UiScriptProgress, -1);
        RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
        DrawFadingMenuSprites(g_UiScriptProgress, 1, g_EngineerShopOption);
        if (g_UiScriptProgress <= 0) {
            if (GameMenuBusy == ENGINEER_SHOP_UPGRADED) {
                g_CarTable[g_PlayerCarIndex].modelVariant++;
                if (g_CarTable[g_PlayerCarIndex].modelVariant > g_TimeAttackCars[g_PlayerCarIndex].modelVariant) {
                    g_TimeAttackCars[g_PlayerCarIndex].modelVariant = g_CarTable[g_PlayerCarIndex].modelVariant;
                }
                g_PlayerMoney -= value;
            }
            MenuFlowOpen(MENU_SCREEN_CAR_SELECT);
            g_UiScriptProgress = 0;
            GameMenuBusy = ENGINEER_SHOP_ACTIVE;
            g_EngineerShopOption = 0;
        }
    }
}

void ShopScreenNoOp(void) {
}
