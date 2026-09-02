#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/save_internal.h"

enum CustomizeScreenState {
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
    CUSTOMIZE_DESIGN_VIEW_OFFSET = 0x3D090
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

static void HandleCustomizeMenuInput(s32 lastOption) {
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_RankingOption = g_RankingOption > 0 ? g_RankingOption - 1
                                              : lastOption;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_RankingOption = g_RankingOption < lastOption ? g_RankingOption + 1
                                                       : 0;
    }

    if (g_PadPressed & PAD_CONFIRM) {
        if (g_RankingOption == 0) {
            PlaySoundCue(2);
            g_CustomizePopupScript = g_MenuDialogPanelUpperScript;
            GameMenuBusy = CUSTOMIZE_TIRE_DIALOG;
            g_UiScriptProgress2 = 0;
            g_MenuSubCursor = g_CarTable[g_PlayerCarIndex].tireCompound;
            return;
        }
        if (g_RankingOption == 1) {
            g_UiScriptProgress2 = 0;
            if (g_CarModelAsset->transmissionAvailable != 0) {
                PlaySoundCue(2);
                g_CustomizePopupScript = g_MenuDialogPanelLowerScript;
                GameMenuBusy = CUSTOMIZE_TRANSMISSION_DIALOG;
                g_MenuSubCursor = g_CarTable[g_PlayerCarIndex].transmission;
            } else {
                PlaySoundCue(5);
                g_CustomizePopupScript = g_TransmissionUnavailableScript;
                GameMenuBusy = CUSTOMIZE_TRANSMISSION_UNAVAILABLE;
            }
            return;
        }
        if (g_RankingOption == lastOption) {
            PlaySoundCue(3);
            GameMenuBusy = CUSTOMIZE_EXIT_TO_CAR_SELECT;
            g_MenuOverlayPattern = 2;
            return;
        }
        if (g_RankingOption == 2) {
            PlaySoundCue(2);
            GameMenuBusy = CUSTOMIZE_EXIT_TO_DESIGN;
            g_MenuOverlayPattern = 1;
            g_CarSpecGraphStep = -3;
            g_MenuViewOffsetTarget = CUSTOMIZE_DESIGN_VIEW_OFFSET;
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = CUSTOMIZE_EXIT_TO_CAR_SELECT;
        g_MenuOverlayPattern = 2;
    }
}

static void UpdateTireDialog(void) {
    if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) ==
        0) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        GameMenuBusy = CUSTOMIZE_TIRE_CONFIRMING;
        g_MenuConfirmTimer = CUSTOMIZE_CONFIRM_FRAMES;
    }
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 0;
    }
    if ((g_PadPressed & PAD_LEFT) && g_MenuSubCursor < 4) {
        PlaySoundCue(1);
        g_MenuSubCursor++;
    }
    if ((g_PadPressed & PAD_RIGHT) && g_MenuSubCursor != 0) {
        PlaySoundCue(1);
        g_MenuSubCursor--;
    }
    DrawTireCompoundSlider(g_MenuSubCursor, 0);
}

static void UpdateTransmissionDialog(void *ot) {
    if (RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1) ==
        0) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        GameMenuBusy = CUSTOMIZE_TRANSMISSION_CONFIRMING;
        g_MenuConfirmTimer = CUSTOMIZE_CONFIRM_FRAMES;
        g_CarTable[g_PlayerCarIndex].transmission = g_MenuSubCursor;
        g_TimeAttackCarTransmissions[g_PlayerCarIndex * 8] = g_MenuSubCursor;
    }
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 0;
    }
    if ((g_PadPressed & PAD_LEFT) && g_MenuSubCursor != 0) {
        PlaySoundCue(1);
        g_MenuSubCursor = 0;
    }
    if ((g_PadPressed & PAD_RIGHT) && g_MenuSubCursor == 0) {
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
        GameMenuBusy = 0;
    }
}

static void UpdateTireConfirmation(void) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer--;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, 1);
        DrawTireCompoundSlider(g_MenuSubCursor, 1);
        return;
    }
    RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = 0;
        g_CarTable[g_PlayerCarIndex].tireCompound = g_MenuSubCursor;
        g_TimeAttackCarTires[g_PlayerCarIndex * 8] = g_MenuSubCursor;
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
        GameMenuBusy = 0;
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
    }
}

void UpdateCustomizeScreen(void) {
    void *ot;
    s32 lastOption;
    const TimedDrawCommand *cmdList;

    ot = RENDER_OT_BASE;
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    lastOption = g_GrandPrixMode != 0 ? 3 : 2;
    cmdList = g_GrandPrixMode != 0 ? g_CustomizeMenuScriptGp
                                   : g_CustomizeMenuScriptTimeAttack;

    if (GameMenuBusy == 0) {
        g_CarSpecGraphStep = 3;
        RunTimedDrawScript(g_CustomizePopupScript, &g_UiScriptProgress2, -1);
        DrawFadingMenuSprites(g_UiScriptProgress, lastOption, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        if (RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0 &&
            g_UiScriptProgress2 <= 0) {
            g_MenuOverlayPattern = -1;
            HandleCustomizeMenuInput(lastOption);
        }
        return;
    }

    if (GameMenuBusy < 0) {
        UpdateCustomizeDialog(ot);
        DrawFadingMenuSprites(g_UiScriptProgress, lastOption, g_RankingOption);
        RunTimedDrawScript(cmdList, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }

    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = 5;
    RunTimedDrawScript(cmdList, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, lastOption, g_RankingOption);
    if (g_UiScriptProgress <= 0) {
        switch (GameMenuBusy) {
        case CUSTOMIZE_EXIT_TO_DESIGN:
            if (g_MenuViewOffset < CUSTOMIZE_DESIGN_VIEW_OFFSET) {
                return;
            }
            g_MenuScreen = 6;
            g_MenuHandlerIndex = 6;
            break;
        case CUSTOMIZE_EXIT_TO_CAR_SELECT:
            g_MenuScreen = 4;
            g_MenuHandlerIndex = 4;
            g_RankingOption = 0;
            break;
        }
        g_UiScriptProgress = 0;
        GameMenuBusy = 0;
    }
}
