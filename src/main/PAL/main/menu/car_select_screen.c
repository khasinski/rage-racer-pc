/*
 * The car select screen: the hub every other menu is reached from. It shows
 * the car the player owns, lets them browse the rest of their garage, and from
 * here they start the race, look at the ranking, go to the shop or to the
 * engineer, or back out to the course.
 *
 * The screen has three states, told apart by GameMenuBusy: idle and taking
 * input, showing a modal that says a shop is closed, or on its way out to
 * whichever screen was chosen.
 */

#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/save_internal.h"

/* The last row of the menu backs out, so its index is also the count of the
 * rows above it: two in time attack, four in a Grand Prix. */
static s32 CarSelectLastRow(void) { return g_GrandPrixMode != 0 ? 4 : 2; }

static const TimedDrawCommand *CarSelectMenuScript(void) {
    if (g_GrandPrixMode != 0) {
        return g_CarSelectMenuScriptGp;
    }
    return g_CarSelectMenuScriptTimeAttack;
}

/* Leaving the screen upwards, back to the course: the same wind-down whether
 * the player chose the last row or pressed cancel. */
static void LeaveCarSelectScreen(void) {
    PlaySoundCue(3);
    GameMenuBusy = 5;
    g_MenuOverlayPattern = 2;
    g_CarNamePlateStep = -10;
    g_CarSpecGraphStep = -3;
    g_MenuViewOffsetTarget = 0x3D090;
}

/* A shop that will not take the player shows a modal instead of opening. */
static void RefuseWithModal(const TimedDrawCommand *script, s32 busyState) {
    PlaySoundCue(5);
    g_CarSelectPopupScript = script;
    GameMenuBusy = busyState;
    g_UiScriptProgress2 = 0;
}

static void EnterCarShop(void) {
    s32 previousTarget = g_MenuViewAngleTarget;

    PlaySoundCue(2);
    g_CarListCursor = g_ShopCarIndex;
    RequestCarModel(g_CarListCursor);
    g_MenuViewAngleTarget = 0x124F80;
    GameMenuBusy = 3;
    g_MenuOverlayPattern = 1;
    g_CarSwapFromIndex = g_PlayerCarIndex;
    g_CarSwapToIndex = g_CarListCursor;
    g_MenuViewAngle = 0x927C0 - (previousTarget - g_MenuViewAngle);
}

/*
 * What the confirm button does depends on the row the cursor is on. The last
 * row is tested before row two, because in time attack they are the same row.
 */
static void ChooseCarSelectRow(s32 row) {
    if (row == 0) {
        PlaySoundCue(2);
        StartSequenceFadeOut();
        if (g_GrandPrixMode != 0) {
            /* Class five is the extra series, which has no round of its own. */
            g_GrandPrixSeries = (s16)GrandPrixAssetSeries(
                g_GrandPrixSeries, g_GrandPrixClass);
        } else {
            g_GrandPrixSeries = g_CourseIndex >> 2;
        }
        RequestRoundAssets();
        GameMenuBusy = 1;
        g_MenuHintBarStep = -1;
        g_CarNamePlateStep = -10;
        g_MenuOverlayPattern = 0;
        g_CarSpecGraphStep = -3;
        g_MenuViewOffsetTarget = 0x3D090;
        return;
    }
    if (row == 1) {
        PlaySoundCue(2);
        GameMenuBusy = 2;
        g_MenuOverlayPattern = 1;
        g_CarNamePlateStep = -10;
        return;
    }
    if (row == CarSelectLastRow()) {
        LeaveCarSelectScreen();
        return;
    }
    if (row == 2) {
        if (g_ShopCarIndex == -1) {
            RefuseWithModal(g_CarShopUnavailableScript, -1);
            return;
        }
        EnterCarShop();
        return;
    }
    if (row == 3) {
        if ((g_CarModelAsset->upgradesAvailable != 0) &&
            (g_RaceProgress->maxClassReached >=
             GetCarUnlockLevel(g_PlayerCarIndex))) {
            GameMenuBusy = 4;
            g_MenuOverlayPattern = 1;
            PlaySoundCue(2);
            return;
        }
        RefuseWithModal(g_EngineerShopUnavailableScript, -2);
    }
}

/* Idle: the screen is up and the pad drives it. */
static void UpdateCarSelectInput(void) {
    s32 lastRow = CarSelectLastRow();
    s32 carBeforeSwap;

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_CarSelectCursor =
            (g_CarSelectCursor > 0) ? g_CarSelectCursor - 1 : lastRow;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_CarSelectCursor =
            (g_CarSelectCursor < lastRow) ? g_CarSelectCursor + 1 : 0;
    }
    UpdateOwnedCarNeighbours();
    RefreshCarUnlockState();

    carBeforeSwap = g_PlayerCarIndex;
    if ((g_PadHeld & PAD_LEFT) && (g_PrevOwnedCarIndex != -1) &&
        MenuCarViewSettled() && (g_CarSwapToIndex < 0)) {
        MenuSpinToCar(&g_PlayerCarIndex, carBeforeSwap, g_PrevOwnedCarIndex, 0);
    }
    if ((g_PadHeld & PAD_RIGHT) && (g_NextOwnedCarIndex != -1) &&
        MenuCarViewSettled() && (g_CarSwapToIndex < 0)) {
        MenuSpinToCar(&g_PlayerCarIndex, carBeforeSwap, g_NextOwnedCarIndex,
                      0x124F80);
    }

    if (!MenuCarViewSettled() || (g_CarSwapToIndex >= 0)) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseCarSelectRow(g_CarSelectCursor);
    } else if ((g_PadPressed & PAD_CANCEL) &&
               ((u32)(g_MenuViewAngle - 0x2710) > 0x120160U)) {
        LeaveCarSelectScreen();
    }
}

/* Idle: everything the screen puts on the display, and the input once the
 * chrome has finished sliding in and no modal is on top of it. */
static void UpdateCarSelectIdle(void) {
    g_CarNamePlateStep = 0x14;
    g_CarSpecGraphStep = 3;
    g_MenuPlateCarIndex = g_PlayerCarIndex;
    RunTimedDrawScript(g_CarSelectPopupScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    DrawBrowseArrows(1, 0, g_PrevOwnedCarIndex != -1,
                     g_NextOwnedCarIndex != -1);
    if (g_GrandPrixMode == 0) {
        DrawOwnedCarCounter(1, CountOwnedCars());
    }
    DrawFadingMenuSprites(g_UiScriptProgress, CarSelectLastRow(),
                          g_CarSelectCursor);
    RunTimedDrawScript(CarSelectMenuScript(), &g_UiScriptProgress, 0);
    if ((RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) &&
        (g_UiScriptProgress2 <= 0)) {
        UpdateCarSelectInput();
    }
}

/* A modal is up over the screen; the only thing it takes is dismissal. */
static void UpdateCarSelectModal(void) {
    RunTimedDrawScript(g_CarSelectPopupScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
        if (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) {
            GameMenuBusy = 0;
        }
    }
    DrawBrowseArrows(1, 0, g_PrevOwnedCarIndex != -1,
                     g_NextOwnedCarIndex != -1);
    if (g_GrandPrixMode == 0) {
        DrawOwnedCarCounter(1, CountOwnedCars());
    }
    DrawFadingMenuSprites(g_UiScriptProgress, CarSelectLastRow(),
                          g_CarSelectCursor);
    RunTimedDrawScript(CarSelectMenuScript(), &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

/*
 * On the way out. The chrome slides off, and once it has gone the chosen
 * screen is handed the controls. Returns without doing so while the view is
 * still travelling, so the next frame tries again.
 */
static void EnterChosenScreen(void) {
    switch (GameMenuBusy) {
    case 1:
        if ((g_MenuOutgoingScreenProgress > 0) &&
            (g_MenuViewOffset <= 0x3D08F)) {
            return;
        }
        g_SceneId = 9;
        g_CourseIndex &= 3;
        g_RaceProgress->course = g_CourseIndex;
        g_RaceProgress->carIndex = g_PlayerCarIndex;
        g_RaceProgress->classIndex = g_GrandPrixClass;
        if (g_GrandPrixMode != 0) {
            g_RaceProgress->money = g_PlayerMoney;
        } else {
            g_RaceProgress->timeAttackSeries = g_GrandPrixSeries;
        }
        break;
    case 2:
        g_MenuScreen = MENU_SCREEN_CUSTOMIZE;
        g_MenuHandlerIndex = MENU_SCREEN_CUSTOMIZE;
        break;
    case 3:
        g_MenuScreen = MENU_SCREEN_CAR_SHOP;
        g_MenuHandlerIndex = MENU_SCREEN_CAR_SHOP;
        DrawCarShopPricePanel(0, 0, 0);
        DrawBrowseArrows(0, 0, 0, 0);
        DrawMenuAltPanel(0, 0);
        g_MenuUpperAltPanelStep = 0;
        g_MenuLowerAltPanelStep = 0;
        ClearTeamNameTexture();
        RestoreTeamLogoClut();
        break;
    case 4:
        g_MenuScreen = MENU_SCREEN_ENGINEER_SHOP;
        g_MenuHandlerIndex = MENU_SCREEN_ENGINEER_SHOP;
        DrawEngineerShopPricePanel(0, 0, 0);
        break;
    case 5:
        if (g_MenuViewOffset <= 0x3D08F) {
            return;
        }
        g_MenuViewAngle = 0x7A120;
        g_MenuViewAngleTarget = 0x7A120;
        g_MenuScreen = MENU_SCREEN_COURSE_SELECT;
        g_MenuHandlerIndex = MENU_SCREEN_COURSE_SELECT;
        g_CarSelectCursor = 0;
        g_MenuPendingCourseIndex = -1;
        g_MenuViewOffset = 0x3D090;
        g_MenuViewOffsetTarget = 0;
        g_CourseCardSpin = 0x1F4000;
        g_MenuCourseModelIndex = g_CourseIndex;
        g_CourseCardPendingGrade =
            g_CourseProgress->bestPlace[g_CourseIndex & 3];
        DrawTimeAttackPlate(0);
        g_TimeAttackPlateStep = (g_CourseIndex >= 4) ? 1 : -1;
        break;
    }
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
}

static void UpdateCarSelectOutgoing(void) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_CAR_SELECT;
    DrawBrowseArrows(-1, 0, g_PrevOwnedCarIndex != -1,
                     g_NextOwnedCarIndex != -1);
    if (g_GrandPrixMode == 0) {
        DrawOwnedCarCounter(-1, CountOwnedCars());
    }
    RunTimedDrawScript(CarSelectMenuScript(), &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, CarSelectLastRow(),
                          g_CarSelectCursor);
    if (g_UiScriptProgress <= 0) {
        EnterChosenScreen();
    }
}

void UpdateCarSelectScreen(void) {
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCarView();
    DrawMenuLightBurst(-9);

    if (GameMenuBusy == 0) {
        UpdateCarSelectIdle();
    } else if (GameMenuBusy < 0) {
        UpdateCarSelectModal();
    } else {
        UpdateCarSelectOutgoing();
    }
}
