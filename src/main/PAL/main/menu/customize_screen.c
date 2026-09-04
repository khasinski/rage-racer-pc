#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/save_internal.h"

enum CustomizeScreenState {
    CUSTOMIZE_IDLE = 0,
    CUSTOMIZE_EXIT_TO_DESIGN = 1,
    CUSTOMIZE_EXIT_TO_CAR_SELECT = 2,
    CUSTOMIZE_TIRE_DIALOG = -1,
    CUSTOMIZE_TRANSMISSION_DIALOG = -2,
    CUSTOMIZE_TRANSMISSION_UNAVAILABLE = -3,
    CUSTOMIZE_TRANSMISSION_UNAVAILABLE_CLOSING = -4,
    CUSTOMIZE_TIRE_CONFIRMING = -5,
    CUSTOMIZE_TRANSMISSION_CONFIRMING = -6
};

enum {
    CUSTOMIZE_CONFIRM_FRAMES = 35,
    TIRE_COMPOUND_LAST = CAR_TIRE_COMPOUND_COUNT - 1,
};

enum CustomizeOption {
    CUSTOMIZE_OPTION_TIRES,
    CUSTOMIZE_OPTION_TRANSMISSION,
    CUSTOMIZE_OPTION_DESIGN,
    CUSTOMIZE_OPTION_EXIT,
};

static void DrawTransmissionChoice(void *ot, s32 flash) {
    DrawMenuCursorBox(g_MenuSubCursor != 0 ? 0xDA : 0xB8, 0x68, 0x20, 0x20,
                      flash);
    DrawSprite(ot, 0xC2, 0x70, 0xC, 0x10, 0x60, 0x7C, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    DrawSprite(ot, 0xE3, 0x70, 0x10, 0x10, 0x6C, 0x7C, 0, 0, 0, 0x244, 1,
               1, 0x3B);
    GameDrawMenuButton(0xB8, 0x68, 0x20, 0x20, 0x95, 0x25, 0x1E);
    GameDrawMenuButton(0xDA, 0x68, 0x20, 0x20, 0x1E, 0x8E, 0x95);
}

static s32 CustomizeCarSetupAvailable(void) {
    return (u32)g_PlayerCarIndex < GAME_CAR_COUNT && g_CarTable != NULL;
}

static s32 CustomizeTransmissionAvailable(void) {
    return CustomizeCarSetupAvailable() && g_CarModelAsset != NULL &&
           g_CarModelAsset->transmissionAvailable != 0;
}

static void HandleCustomizeMenuInput(s32 exitOption, s32 carAvailable) {
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_CustomizeOption =
            WrapMenuIndex(g_CustomizeOption, -1, exitOption + 1);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_CustomizeOption =
            WrapMenuIndex(g_CustomizeOption, 1, exitOption + 1);
    }

    if (g_PadPressed & PAD_CONFIRM) {
        if (g_CustomizeOption == CUSTOMIZE_OPTION_TIRES) {
            if (!carAvailable) {
                return;
            }
            PlaySoundCue(2);
            g_CustomizePopupScript = g_MenuDialogPanelUpperScript;
            GameMenuBusy = CUSTOMIZE_TIRE_DIALOG;
            g_UiScriptProgress2 = 0;
            g_MenuSubCursor = (u8)AddClampedMenuValue(
                g_CarTable[g_PlayerCarIndex].tireCompound, 0, 0,
                TIRE_COMPOUND_LAST);
            return;
        }
        if (g_CustomizeOption == CUSTOMIZE_OPTION_TRANSMISSION) {
            g_UiScriptProgress2 = 0;
            if (CustomizeTransmissionAvailable()) {
                PlaySoundCue(2);
                g_CustomizePopupScript = g_MenuDialogPanelLowerScript;
                GameMenuBusy = CUSTOMIZE_TRANSMISSION_DIALOG;
                g_MenuSubCursor =
                    g_CarTable[g_PlayerCarIndex].transmission != 0;
            } else {
                PlaySoundCue(5);
                g_CustomizePopupScript = g_TransmissionUnavailableScript;
                GameMenuBusy = CUSTOMIZE_TRANSMISSION_UNAVAILABLE;
            }
            return;
        }
        if (g_CustomizeOption == exitOption) {
            PlaySoundCue(3);
            GameMenuBusy = CUSTOMIZE_EXIT_TO_CAR_SELECT;
            g_MenuOverlayPattern = 2;
            return;
        }
        if (g_CustomizeOption == CUSTOMIZE_OPTION_DESIGN) {
            PlaySoundCue(2);
            GameMenuBusy = CUSTOMIZE_EXIT_TO_DESIGN;
            g_MenuOverlayPattern = 1;
            g_CarSpecGraphStep = -3;
            g_MenuViewOffsetTarget = MENU_VIEW_OFFSET_MAX;
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = CUSTOMIZE_EXIT_TO_CAR_SELECT;
        g_MenuOverlayPattern = 2;
    }
}

static void UpdateTireDialog(void) {
    MenuDialogAction action;

    if (!CustomizeCarSetupAvailable()) {
        GameMenuBusy = CUSTOMIZE_IDLE;
        return;
    }
    g_MenuSubCursor = (u8)AddClampedMenuValue(
        g_MenuSubCursor, 0, 0, TIRE_COMPOUND_LAST);
    if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) ==
        0) {
        return;
    }
    action = ChooseMenuDialogAction(g_PadPressed);
    if (action == MENU_DIALOG_CONFIRM) {
        PlaySoundCue(2);
        GameMenuBusy = CUSTOMIZE_TIRE_CONFIRMING;
        g_MenuConfirmTimer = CUSTOMIZE_CONFIRM_FRAMES;
    } else if (action == MENU_DIALOG_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = CUSTOMIZE_IDLE;
    } else if (action == MENU_DIALOG_LEFT &&
               g_MenuSubCursor < TIRE_COMPOUND_LAST) {
        PlaySoundCue(1);
        g_MenuSubCursor++;
    } else if (action == MENU_DIALOG_RIGHT && g_MenuSubCursor != 0) {
        PlaySoundCue(1);
        g_MenuSubCursor--;
    }
    DrawTireCompoundSlider(g_MenuSubCursor, 0);
}

static void UpdateTransmissionDialog(void *ot) {
    MenuDialogAction action;

    if (!CustomizeTransmissionAvailable()) {
        GameMenuBusy = CUSTOMIZE_IDLE;
        return;
    }
    g_MenuSubCursor = g_MenuSubCursor != 0;
    if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) ==
        0) {
        return;
    }
    action = ChooseMenuDialogAction(g_PadPressed);
    if (action == MENU_DIALOG_CONFIRM) {
        PlaySoundCue(2);
        GameMenuBusy = CUSTOMIZE_TRANSMISSION_CONFIRMING;
        g_MenuConfirmTimer = CUSTOMIZE_CONFIRM_FRAMES;
        g_CarTable[g_PlayerCarIndex].transmission = g_MenuSubCursor;
        g_TimeAttackCars[g_PlayerCarIndex].transmission = g_MenuSubCursor;
    } else if (action == MENU_DIALOG_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = CUSTOMIZE_IDLE;
    } else if (action == MENU_DIALOG_LEFT && g_MenuSubCursor != 0) {
        PlaySoundCue(1);
        g_MenuSubCursor = 0;
    } else if (action == MENU_DIALOG_RIGHT && g_MenuSubCursor == 0) {
        PlaySoundCue(1);
        g_MenuSubCursor = 1;
    }
    DrawTransmissionChoice(ot, 0);
}

static void UpdateUnavailableDialog(void) {
    RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0 &&
        (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL))) {
        GameMenuBusy = CUSTOMIZE_TRANSMISSION_UNAVAILABLE_CLOSING;
    }
}

static void CloseUnavailableDialog(void) {
    RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = CUSTOMIZE_IDLE;
    }
}

static void UpdateTireConfirmation(void) {
    if (!CustomizeCarSetupAvailable()) {
        GameMenuBusy = CUSTOMIZE_IDLE;
        return;
    }
    g_MenuSubCursor = (u8)AddClampedMenuValue(
        g_MenuSubCursor, 0, 0, TIRE_COMPOUND_LAST);
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer--;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1);
        DrawTireCompoundSlider(g_MenuSubCursor, 1);
        return;
    }
    RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = CUSTOMIZE_IDLE;
        g_CarTable[g_PlayerCarIndex].tireCompound = g_MenuSubCursor;
        g_TimeAttackCars[g_PlayerCarIndex].tireCompound = g_MenuSubCursor;
    }
}

static void UpdateTransmissionConfirmation(void *ot) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer--;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1);
        DrawTransmissionChoice(ot, 1);
        return;
    }
    RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = CUSTOMIZE_IDLE;
    }
}

static void UpdateCustomizeDialog(void *ot) {
    switch (GameMenuBusy) {
    case CUSTOMIZE_TIRE_DIALOG:
        UpdateTireDialog();
        break;
    case CUSTOMIZE_TRANSMISSION_DIALOG:
        UpdateTransmissionDialog(ot);
        break;
    case CUSTOMIZE_TRANSMISSION_UNAVAILABLE:
        UpdateUnavailableDialog();
        break;
    case CUSTOMIZE_TRANSMISSION_UNAVAILABLE_CLOSING:
        CloseUnavailableDialog();
        break;
    case CUSTOMIZE_TIRE_CONFIRMING:
        UpdateTireConfirmation();
        break;
    case CUSTOMIZE_TRANSMISSION_CONFIRMING:
        UpdateTransmissionConfirmation(ot);
        break;
    default:
        GameMenuBusy = CUSTOMIZE_IDLE;
        break;
    }
}

void UpdateCustomizeScreen(void) {
    void *ot;
    s32 exitOption;
    const TimedDrawCommand *cmdList;

    ot = RENDER_OT_BASE;
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    exitOption = g_GrandPrixMode != 0 ? CUSTOMIZE_OPTION_EXIT
                                     : CUSTOMIZE_OPTION_DESIGN;
    g_CustomizeOption =
        AddClampedMenuValue(g_CustomizeOption, 0, 0, exitOption);
    cmdList = g_GrandPrixMode != 0 ? g_CustomizeMenuScriptGp
                                   : g_CustomizeMenuScriptTimeAttack;

    if (GameMenuBusy == CUSTOMIZE_IDLE) {
        g_CarSpecGraphStep = 3;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
        DrawFadingMenuSprites(g_UiScriptProgress, exitOption,
                              g_CustomizeOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0 &&
            g_UiScriptProgress2 <= 0) {
            g_MenuOverlayPattern = -1;
            HandleCustomizeMenuInput(exitOption,
                                     CustomizeCarSetupAvailable());
        }
        return;
    }

    if (GameMenuBusy < 0) {
        UpdateCustomizeDialog(ot);
        DrawFadingMenuSprites(g_UiScriptProgress, exitOption,
                              g_CustomizeOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_CUSTOMIZE;
    RunTimedDrawScript(cmdList, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, exitOption, g_CustomizeOption);
    if (g_UiScriptProgress <= 0) {
        switch (GameMenuBusy) {
        case CUSTOMIZE_EXIT_TO_DESIGN:
            if (g_MenuViewOffset < MENU_VIEW_OFFSET_MAX) {
                return;
            }
            g_MenuScreen = MENU_SCREEN_DESIGN_MODE;
            g_MenuHandlerIndex = MENU_SCREEN_DESIGN_MODE;
            break;
        case CUSTOMIZE_EXIT_TO_CAR_SELECT:
            g_MenuScreen = MENU_SCREEN_CAR_SELECT;
            g_MenuHandlerIndex = MENU_SCREEN_CAR_SELECT;
            g_CustomizeOption = CUSTOMIZE_OPTION_TIRES;
            break;
        }
        g_UiScriptProgress = 0;
        GameMenuBusy = CUSTOMIZE_IDLE;
    }
}
