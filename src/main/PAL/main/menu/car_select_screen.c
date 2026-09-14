/*
 * The car select screen: the hub every other menu is reached from. It shows
 * the car the player owns, lets them browse the rest of their garage, and from
 * here they start the race, look at the ranking, go to the shop or to the
 * engineer, or back out to the course.
 *
 * The screen has three states: idle and taking
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

#include <stdio.h>

/* The last row of the menu backs out, so its index is also the count of the
 * rows above it: two in time attack, four in a Grand Prix. */
static s32 CarSelectLastRow(void) { return g_GrandPrixMode != 0 ? 4 : 2; }

static s32 CarSelectState(void) {
    return MenuRuntimeScreenState(MENU_SCREEN_CAR_SELECT);
}

static void SetCarSelectState(s32 state) {
    MenuRuntimeSetScreenState(MENU_SCREEN_CAR_SELECT, state);
}

static const TimedDrawCommand *CarSelectMenuScript(void) {
    if (g_GrandPrixMode != 0) {
        return g_CarSelectMenuScriptGp;
    }
    return g_CarSelectMenuScriptTimeAttack;
}

static void UpdateSelectableCarNeighbours(CarBrowse *browse) {
    s32 modelCount;

    if (g_RaceSession.kind != RACE_SESSION_CUSTOM) {
        UpdateOwnedCarNeighbours(browse);
        return;
    }
    modelCount = CustomRaceModelCount(g_RaceSession.classIndex);
    browse->previous = g_RaceSession.model > 0
                           ? g_RaceSession.model - 1
                           : -1;
    browse->next = g_RaceSession.model + 1 < modelCount
                       ? g_RaceSession.model + 1
                       : -1;
}

static void SpinToSelectableCar(s32 selection, s32 target) {
    s32 from = g_PlayerCarIndex;
    s32 preview = CustomRacePreviewCar(selection);

    MenuSpinToCar(&g_PlayerCarIndex, from, preview, target);
    if (g_PlayerCarIndex == preview) {
        g_RaceSession.model = selection;
    }
}

static void DrawCustomCarLabel(void) {
    char text[24];

    if (g_RaceSession.kind != RACE_SESSION_CUSTOM) return;
    snprintf(text, sizeof(text), "%02d / %02d%s",
             g_RaceSession.model + 1,
             CustomRaceModelCount(g_RaceSession.classIndex),
             CustomRaceUsesRivalModel() ? "  RIVAL" : "");
    DrawText8x8(0xE8, 0x15C, text, 0x78CC);
}

static void DrawSelectableCarCounter(s32 direction) {
    if (g_RaceSession.kind == RACE_SESSION_CUSTOM) {
        DrawCarCounter(MenuWidgetState(), direction,
                       g_RaceSession.model + 1,
                       CustomRaceModelCount(g_RaceSession.classIndex));
    } else if (g_GrandPrixMode == 0) {
        DrawOwnedCarCounter(MenuWidgetState(), direction, CountOwnedCars());
    }
}

/* Leaving the screen upwards, back to the course: the same wind-down whether
 * the player chose the last row or pressed cancel. */
static void LeaveCarSelectScreen(void) {
    PlaySoundCue(3);
    SetCarSelectState(5);
    g_MenuOverlayPattern = 2;
    MenuWidgetState()->carNameStep = -10;
    MenuCarSpecGraph()->step = -3;
    g_MenuViewOffsetTarget = 0x3D090;
}

/* A shop that will not take the player shows a modal instead of opening. */
static void RefuseWithModal(const TimedDrawCommand *script, s32 busyState) {
    PlaySoundCue(5);
    MenuCarSelect()->popupScript = script;
    SetCarSelectState(busyState);
    g_UiScriptProgress2 = 0;
}

static void EnterCarShop(CarBrowse *browse) {
    s32 previousTarget = g_MenuViewAngleTarget;

    if (!RequestCarModel(browse->shopIndex)) {
        return;
    }
    PlaySoundCue(2);
    browse->cursor = browse->shopIndex;
    g_MenuViewAngleTarget = MENU_CAR_VIEW_RIGHT_TARGET;
    SetCarSelectState(3);
    g_MenuOverlayPattern = 1;
    g_CarSwapFromIndex = g_PlayerCarIndex;
    g_CarSwapToIndex = browse->cursor;
    g_MenuViewAngle =
        RebaseCarouselValue(g_MenuViewAngle, previousTarget,
                            MENU_CAR_VIEW_REBASE_SPAN);
}

static s32 PlayerCarCanBeUpgraded(void) {
    s32 unlockLevel;

    if ((u32)g_PlayerCarIndex >= GAME_CAR_COUNT || g_CarModelAsset == NULL ||
        g_RaceProgress == NULL || g_CarModelAsset->upgradesAvailable == 0) {
        return 0;
    }
    unlockLevel = GetCarUnlockLevel(g_PlayerCarIndex);
    return unlockLevel >= 0 && g_RaceProgress->maxClassReached >= unlockLevel;
}

/*
 * What the confirm button does depends on the row the cursor is on. The last
 * row is tested before row two, because in time attack they are the same row.
 */
static void ChooseCarSelectRow(CarBrowse *browse, s32 row) {
    if (row == 0) {
        PlaySoundCue(2);
        StartSequenceFadeOut();
        if (g_RaceSession.kind == RACE_SESSION_CUSTOM) {
            ApplyCustomRaceSelection();
            g_GrandPrixSeries = CourseSeries(g_CourseIndex);
        } else if (g_GrandPrixMode != 0) {
            /* Class five is the extra series, which has no round of its own. */
            g_GrandPrixSeries = (s16)GrandPrixAssetSeries(
                g_GrandPrixSeries, g_GrandPrixClass);
        } else {
            g_GrandPrixSeries = g_CourseIndex >> 2;
        }
        RequestRoundAssets();
        SetCarSelectState(1);
        MenuWidgetState()->hintStep = -1;
        MenuWidgetState()->carNameStep = -10;
        g_MenuOverlayPattern = 0;
        MenuCarSpecGraph()->step = -3;
        g_MenuViewOffsetTarget = 0x3D090;
        return;
    }
    if (row == 1) {
        PlaySoundCue(2);
        SetCarSelectState(2);
        g_MenuOverlayPattern = 1;
        MenuWidgetState()->carNameStep = -10;
        return;
    }
    if (row == CarSelectLastRow()) {
        LeaveCarSelectScreen();
        return;
    }
    if (row == 2) {
        if (browse->shopIndex == -1) {
            RefuseWithModal(g_CarShopUnavailableScript, -1);
            return;
        }
        EnterCarShop(browse);
        return;
    }
    if (row == 3) {
        if (PlayerCarCanBeUpgraded()) {
            SetCarSelectState(4);
            g_MenuOverlayPattern = 1;
            PlaySoundCue(2);
            return;
        }
        RefuseWithModal(g_EngineerShopUnavailableScript, -2);
    }
}

/* Idle: the screen is up and the pad drives it. */
static void UpdateCarSelectInput(CarBrowse *browse) {
    CarSelect *screen = MenuCarSelect();
    s32 lastRow = CarSelectLastRow();
    s32 carBeforeSwap;

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        screen->cursor = screen->cursor > 0 ? screen->cursor - 1 : lastRow;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        screen->cursor = screen->cursor < lastRow ? screen->cursor + 1 : 0;
    }
    UpdateSelectableCarNeighbours(browse);
    if (g_RaceSession.kind != RACE_SESSION_CUSTOM) {
        RefreshCarUnlockState(browse);
    }

    carBeforeSwap = g_PlayerCarIndex;
    if ((g_PadHeld & PAD_LEFT) && (browse->previous != -1) &&
        MenuCarViewSettled() && (g_CarSwapToIndex < 0)) {
        if (g_RaceSession.kind == RACE_SESSION_CUSTOM) {
            SpinToSelectableCar(browse->previous, 0);
        } else {
            MenuSpinToCar(&g_PlayerCarIndex, carBeforeSwap, browse->previous, 0);
        }
    }
    if ((g_PadHeld & PAD_RIGHT) && (browse->next != -1) &&
        MenuCarViewSettled() && (g_CarSwapToIndex < 0)) {
        if (g_RaceSession.kind == RACE_SESSION_CUSTOM) {
            SpinToSelectableCar(browse->next, MENU_CAR_VIEW_RIGHT_TARGET);
        } else {
            MenuSpinToCar(&g_PlayerCarIndex, carBeforeSwap, browse->next,
                          MENU_CAR_VIEW_RIGHT_TARGET);
        }
    }

    if (!MenuCarViewSettled() || (g_CarSwapToIndex >= 0)) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseCarSelectRow(browse, screen->cursor);
    } else if ((g_PadPressed & PAD_CANCEL) &&
               ((u32)g_MenuViewAngle - 0x2710U > 0x120160U)) {
        LeaveCarSelectScreen();
    }
}

/* Idle: everything the screen puts on the display, and the input once the
 * chrome has finished sliding in and no modal is on top of it. */
static void UpdateCarSelectIdle(CarBrowse *browse) {
    CarSelect *screen = MenuCarSelect();

    MenuWidgetState()->carNameStep = 0x14;
    MenuCarSpecGraph()->step = 3;
    MenuWidgetState()->carNameModel = g_PlayerCarIndex;
    RunTimedDrawScript(screen->popupScript, &g_UiScriptProgress2, -1);
    RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 0);
    DrawBrowseArrows(MenuBrowseArrows(), 1, 0, browse->previous != -1,
                     browse->next != -1);
    DrawSelectableCarCounter(1);
    DrawFadingMenuSprites(g_UiScriptProgress, CarSelectLastRow(),
                          screen->cursor);
    RunTimedDrawScript(CarSelectMenuScript(), &g_UiScriptProgress, 0);
    if ((RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1) != 0) &&
        (g_UiScriptProgress2 <= 0)) {
        UpdateCarSelectInput(browse);
    }
}

/* A modal is up over the screen; the only thing it takes is dismissal. */
static void UpdateCarSelectModal(const CarBrowse *browse) {
    CarSelect *screen = MenuCarSelect();

    RunTimedDrawScript(screen->popupScript, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_UiChromeScript2, &g_UiScriptProgress2, 1) != 0) {
        if (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) {
            SetCarSelectState(0);
        }
    }
    DrawBrowseArrows(MenuBrowseArrows(), 1, 0, browse->previous != -1,
                     browse->next != -1);
    DrawSelectableCarCounter(1);
    DrawFadingMenuSprites(g_UiScriptProgress, CarSelectLastRow(),
                          screen->cursor);
    RunTimedDrawScript(CarSelectMenuScript(), &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

/* Publish the menu selection only when the persistent race state can accept
 * it.  Asset/bootstrap failures must leave the outgoing screen alive for a
 * later retry instead of dereferencing an absent save-state record. */
static s32 HandOverToRace(void) {
    s32 course = CourseSlot(g_CourseIndex);

    if (g_RaceSession.kind == RACE_SESSION_CUSTOM) {
        ApplyCustomRaceSelection();
        g_SceneId = 9;
        g_CourseIndex = course;
        return 1;
    }
    if (!StoreRaceSelection(g_RaceProgress, g_GrandPrixMode, course,
                            g_PlayerCarIndex, g_GrandPrixClass, g_PlayerMoney,
                            g_GrandPrixSeries)) {
        return 0;
    }

    g_SceneId = 9;
    g_CourseIndex = course;
    return 1;
}

/*
 * On the way out. The chrome slides off, and once it has gone the chosen
 * screen is handed the controls. Returns without doing so while the view is
 * still travelling, so the next frame tries again.
 */
static void EnterChosenScreen(void) {
    CourseSelectScreen *courseSelect = MenuCourseSelect();

    switch (CarSelectState()) {
    case 1:
        if ((MenuOutgoingProgress() > 0) &&
            (g_MenuViewOffset <= 0x3D08F)) {
            return;
        }
        if (!HandOverToRace()) {
            return;
        }
        break;
    case 2:
        MenuActivateScreen(MENU_SCREEN_CUSTOMIZE);
        break;
    case 3:
        MenuActivateScreen(MENU_SCREEN_CAR_SHOP);
        DrawCarShopPricePanel(0, 0, 0);
        DrawBrowseArrows(MenuBrowseArrows(), 0, 0, 0, 0);
        MenuWidgetState()->upperAltPanelStep = 0;
        MenuWidgetState()->lowerAltPanelStep = 0;
        DrawMenuAltPanel(MenuWidgetState());
        ClearTeamNameTexture();
        RestoreTeamLogoClut();
        break;
    case 4:
        MenuActivateScreen(MENU_SCREEN_ENGINEER_SHOP);
        DrawEngineerShopPricePanel(0, 0, 0);
        break;
    case 5:
        if (g_MenuViewOffset <= 0x3D08F) {
            return;
        }
        g_MenuViewAngle = 0x7A120;
        g_MenuViewAngleTarget = 0x7A120;
        MenuActivateScreen(MENU_SCREEN_COURSE_SELECT);
        MenuCarSelect()->cursor = 0;
        courseSelect->pendingCourse = -1;
        g_MenuViewOffset = 0x3D090;
        g_MenuViewOffsetTarget = 0;
        courseSelect->cardSpin = 0x1F4000;
        courseSelect->displayedCourse = g_CourseIndex;
        courseSelect->cardPendingGrade =
            g_CourseProgress != NULL
                ? g_CourseProgress->bestPlace[CourseSlot(g_CourseIndex)]
                : 0;
        MenuWidgetState()->timeAttackStep = 0;
        DrawTimeAttackPlate(MenuWidgetState());
        MenuWidgetState()->timeAttackStep = CourseSeries(g_CourseIndex) != 0 ? 1 : -1;
        break;
    }
    g_UiScriptProgress = 0;
    SetCarSelectState(0);
}

static void UpdateCarSelectOutgoing(const CarBrowse *browse) {
    CarSelect *screen = MenuCarSelect();

    MenuBeginExit(MENU_SCREEN_CAR_SELECT);
    DrawBrowseArrows(MenuBrowseArrows(), -1, 0, browse->previous != -1,
                     browse->next != -1);
    DrawSelectableCarCounter(-1);
    RunTimedDrawScript(CarSelectMenuScript(), &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, CarSelectLastRow(),
                          screen->cursor);
    if (g_UiScriptProgress <= 0) {
        EnterChosenScreen();
    }
}

void UpdateCarSelectScreen(void) {
    CarBrowse *browse = MenuCarBrowse();

    DrawCarNamePlate(MenuWidgetState());
    DrawMenuCarView();
    DrawMenuLightBurst(MenuWidgetState(), -9);
    DrawCustomCarLabel();

    if (CarSelectState() == 0) {
        UpdateCarSelectIdle(browse);
    } else if (CarSelectState() < 0) {
        UpdateCarSelectModal(browse);
    } else {
        UpdateCarSelectOutgoing(browse);
    }
}
