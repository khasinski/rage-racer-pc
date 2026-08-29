/*
 * The course select screen: where a race is started from.
 *
 * The player steps through the courses, picks one of three rows, and from here
 * either goes racing, backs out to the ranking, or opens the two prompts this
 * screen owns: saving the game, and changing the Grand Prix class, which
 * resets their progress through the series and so is asked twice.
 *
 * GameMenuBusy tells the states apart: zero while it is idle, one of five
 * negatives while a prompt is up, and a positive on the way to whichever
 * screen was chosen.
 */

#include "game/asset.h"
#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"
#include "game/save_internal.h"

/* The course card has to have come to rest before it will take another one. */
#define CARD_REST_WINDOW 0x3D08F

static int CourseCardSettled(void) {
    s32 target = g_MenuViewAngleTarget;
    s32 angle = g_MenuViewAngle;

    return target < angle ? (angle - target <= CARD_REST_WINDOW)
                          : (target - angle <= CARD_REST_WINDOW);
}

/*
 * Steps to the course either side. Both directions run the same arithmetic on
 * the view angle and on the card's spin, and differ only in which way the
 * index moves and where the view is asked to stop.
 */
static void BrowseToCourse(s32 step, s32 newTarget) {
    s32 previousTarget = g_MenuViewAngleTarget;
    s32 previousSpin = g_CourseCardSpinTarget;
    s32 course = g_CourseIndex;

    PlaySoundCue(8);
    g_MenuViewAngleTarget = newTarget;
    g_CourseSwapDelay = 0;
    g_MenuCourseModelIndex = course;
    course += step;
    g_MenuViewAngle = (g_MenuViewAngle - previousTarget) + 0x7A120;
    g_CourseCardSpin = (g_CourseCardSpin - previousSpin) + 0x1F4000;
    g_CourseIndex = course;
    g_MenuPendingCourseIndex = course;
    g_CourseCardPendingGrade = g_CourseProgress->bestPlace[course & 3];
    /* The first four courses are the Grand Prix ones; the rest are time
     * attack, and only those show the plate. */
    g_TimeAttackPlateStep = (course < 4) ? -1 : 1;
}

/* Which way out of this screen is reachable is asked three times a frame. */
static void DrawCourseArrows(s32 step) {
    s32 previous = CanSelectPrevCourse();

    DrawBrowseArrows(step, 1, previous, CanSelectNextCourse());
}

static void *CourseSelectMenuScript(void) {
    if (g_GrandPrixMode != 0) {
        return (u8 *)&g_CourseSelectGpScript;
    }
    return (u8 *)&g_CourseSelectTimeAttackScript;
}

/* The save prompt's two buttons, and the box round whichever is picked. */
static void DrawSavePromptButtons(void *ot, s32 flash) {
    DrawMenuCursorBox((g_MenuSubCursor != 0) ? 0xB8 : 0xDA, 0x8C, 0x20, 0x20,
                      flash);
    DrawSprite(ot, 0xC0, 0x94, 0x10, 0x10, 0x9D, 0x7C, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    DrawSprite(ot, 0xE3, 0x94, 0x10, 0x10, 0xAD, 0x7C, 0, 0, 0, 0x244, 1, 1,
               0x3B);
    GameDrawMenuButton(0xB8, 0x8C, 0x20, 0x20, 0x95, 0x25, 0x1E, 0, 0, 0,
                       &g_MenuBlankCaption);
    GameDrawMenuButton(0xDA, 0x8C, 0x20, 0x20, 0x1E, 0x4E, 0x95, 0, 0, 0,
                       &g_MenuBlankCaption);
}

/* One row per class the player has reached, with the cursor on the chosen. */
static void DrawClassList(void *ot, s32 flash) {
    s32 i;

    DrawMenuCursorBox(0xB8, g_MenuSubCursor * 0x1E + 0x6C, 0x38, 0x20, flash);
    for (i = 0; i < g_RaceProgress->maxClassReached + 1; i++) {
        DrawSprite(ot, 0xC0, i * 0x1E + 0x74, 0x1A, 0x10, 0x60, 0xCC, 0, 0, 0,
                   0x244, 1, 1, 0x3B);
        DrawSprite(ot, 0xE0, i * 0x1E + 0x74, 8, 0x10, i * 8 + 8, 0x18, 0, 0, 0,
                   0x244, 1, 1, 0x3B);
        GameDrawMenuButton(0xB8, i * 0x1E + 0x6C, 0x38, 0x20, 0x95, 0x25, 0x1E,
                           0, 0, 0, &g_MenuBlankCaption);
    }
}

/* Leaving the screen downwards, into the race or the ranking: the card spins
 * back to where the next screen wants it. */
static void SpinCardAway(void) {
    g_MenuViewOffsetTarget = 0x3D090;
    g_CourseCardPendingGrade = 0;
    g_CourseCardSpin = (g_CourseCardSpin - g_CourseCardSpinTarget) + 0x1F4000;
}

/* Confirm on the row the cursor is on. */
static void ChooseCourseSelectRow(s32 row) {
    if (row == 0) {
        PlaySoundCue(2);
        GameMenuBusy = 1;
        g_MenuOverlayPattern = 1;
        g_TimeAttackPlateStep = -1;
        SpinCardAway();
        return;
    }
    if (row == 2) {
        if (g_GrandPrixMode != 0) {
            /* Saving is only offered in a Grand Prix; class five is the extra
             * series, which has no round of its own to record. */
            PlaySoundCue(2);
            g_CourseSelectModalScript = (u8 *)&g_CourseSelectSavePromptScript;
            GameMenuBusy = -1;
            g_GrandPrixSeries =
                (g_GrandPrixClass < 5) ? (u16)g_GrandPrixSeries : 0;
            g_UiScriptProgress2 = 0;
            g_MenuSubCursor = 1;
            return;
        }
        PlaySoundCue(3);
        StartSequenceFadeOut();
        g_MenuHintBarStep = -1;
        g_TimeAttackPlateStep = -1;
        g_MenuViewOffsetTarget = 0x3D090;
        GameMenuBusy = row;
        g_CourseCardPendingGrade = 0;
        g_GrandPrixSeries = g_CourseIndex >> 2;
        g_CourseCardSpin = (g_CourseCardSpin - g_CourseCardSpinTarget) + 0x1F4000;
        return;
    }
    PlaySoundCue(2);
    if (g_GrandPrixMode != 0) {
        g_CourseSelectModalScript = (u8 *)&g_MenuDialogPanelLowerScript;
        GameMenuBusy = -2;
        g_UiScriptProgress2 = 0;
        g_MenuSubCursor = g_GrandPrixClass;
        return;
    }
    GameMenuBusy = 3;
    g_MenuOverlayPattern = 1;
    g_TimeAttackPlateStep = -1;
}

/* Idle: the pad steps through the courses and picks a row. */
static void UpdateCourseSelectInput(void) {
    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_CourseSelectOption =
            (g_CourseSelectOption > 0) ? g_CourseSelectOption - 1 : 2;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_CourseSelectOption =
            (g_CourseSelectOption < 2) ? g_CourseSelectOption + 1 : 0;
    }
    if ((g_PadHeld & PAD_LEFT) && (CanSelectPrevCourse() != 0) &&
        CourseCardSettled() && (g_MenuPendingCourseIndex < 0)) {
        BrowseToCourse(-1, 0);
    }
    if ((g_PadHeld & PAD_RIGHT) && (CanSelectNextCourse() != 0) &&
        CourseCardSettled() && (g_MenuPendingCourseIndex < 0)) {
        BrowseToCourse(1, 0xF4240);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        ChooseCourseSelectRow(g_CourseSelectOption);
    }
}

static void UpdateCourseSelectIdle(void) {
    g_MenuHintBarStep = 1;
    RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, -1);
    DrawCourseArrows(1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_CourseSelectOption);
    RunTimedDrawScript(CourseSelectMenuScript(), &g_UiScriptProgress, 0);
    DrawMenuLightBurst(7);
    if ((RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1) != 0) &&
        (g_UiScriptProgress2 <= 0)) {
        UpdateCourseSelectInput();
    }
}

/* "Save the game?" with its own yes/no cursor. */
static void UpdateSavePrompt(void *ot) {
    RunTimedDrawScript(&g_CourseSelectSavePromptBanner, &g_UiScriptProgress2, 0);
    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
    if (RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, 1)
        == 0) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue((g_MenuSubCursor != 0) ? 2 : 3);
        GameMenuBusy = -3;
        g_MenuConfirmTimer = 0x23;
    }
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = -4;
    }
    if (g_PadPressed & PAD_LEFT) {
        PlaySoundCue(1);
        g_MenuSubCursor = 1;
    }
    if (g_PadPressed & PAD_RIGHT) {
        PlaySoundCue(1);
        g_MenuSubCursor = 0;
    }
    DrawSavePromptButtons(ot, 0);
}

/* The class list. Picking the class already in use just closes it. */
static void UpdateClassPrompt(void *ot) {
    if (RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, 1)
        == 0) {
        return;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        PlaySoundCue(2);
        if (g_MenuSubCursor == g_GrandPrixClass) {
            GameMenuBusy = 0;
        } else {
            GameMenuBusy = -5;
            g_ClassChangeApplied = 0;
            g_MenuConfirmTimer = 0x23;
            DrawClassChangeCurtain(0);
        }
    }
    if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 0;
    }
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_MenuSubCursor = (g_MenuSubCursor != 0)
                              ? g_MenuSubCursor - 1
                              : g_RaceProgress->maxClassReached;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_MenuSubCursor = (g_MenuSubCursor < g_RaceProgress->maxClassReached)
                              ? g_MenuSubCursor + 1
                              : 0;
    }
    DrawClassList(ot, 0);
}

/* The save going through: the prompt flashes for a while, then the screen
 * starts on its way out, to the race or to the record entry. */
static void UpdateSaveCountdown(void *ot) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer -= 1;
        RunTimedDrawScript(&g_CourseSelectSavePromptBanner,
                           &g_UiScriptProgress2, 0);
        RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
        RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, 1);
        DrawSavePromptButtons(ot, 1);
        return;
    }
    RunTimedDrawScript(&g_CourseSelectSavePromptBanner, &g_UiScriptProgress2,
                       -1);
    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
    RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, 0);
    if (g_UiScriptProgress2 <= 0) {
        StartSequenceFadeOut();
        GameMenuBusy = (g_MenuSubCursor != 0) ? 4 : 2;
        g_MenuHintBarStep = -1;
        SpinCardAway();
    }
}

/* The save prompt refused: wait for it to slide off, then go back to idle. */
static void UpdateSaveDismissed(void) {
    RunTimedDrawScript(&g_CourseSelectSavePromptBanner, &g_UiScriptProgress2,
                       -1);
    RunTimedDrawScript(&g_UiChromeScript2, &g_UiScriptProgress2, 0);
    RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, 0);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = 0;
    }
}

/*
 * Changing class. A curtain draws across, and once it has covered the screen
 * the new class is applied and the player's progress through the series is
 * reset; then the curtain comes back and the screen returns to idle.
 */
static void UpdateClassChange(void *ot) {
    if (g_MenuConfirmTimer > 0) {
        g_MenuConfirmTimer -= 1;
        RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, 1);
        DrawClassList(ot, 1);
        return;
    }
    if (g_ClassChangeApplied != 0) {
        if (DrawClassChangeCurtain(-1) == 0) {
            GameMenuBusy = 0;
            g_UiScriptProgress2 = 0;
        }
        return;
    }
    if (DrawClassChangeCurtain(1) >= 0x19) {
        g_ClassChangeApplied = 1;
        g_GrandPrixClass = g_MenuSubCursor;
        ResetCourseProgress(g_MenuSubCursor);
        g_MenuViewAngle = 0x7A120;
        g_MenuViewAngleTarget = 0x7A120;
        g_CourseSelectOption = 0;
        g_MenuPendingCourseIndex = -1;
        g_CourseCardSpin = 0;
        g_CourseIndex = g_CourseIndex & ~3;
        g_MenuCourseModelIndex = g_CourseIndex;
        g_CourseCardPendingGrade = g_CourseProgress->bestPlace[0];
    }
    RunTimedDrawScript(g_CourseSelectModalScript, &g_UiScriptProgress2, 1);
    DrawClassList(ot, 1);
}

static void UpdateCourseSelectModal(void *ot, s32 state) {
    if (state == -1) {
        UpdateSavePrompt(ot);
    } else if (state == -2) {
        UpdateClassPrompt(ot);
    } else if (state == -3) {
        UpdateSaveCountdown(ot);
    } else if (state == -4) {
        UpdateSaveDismissed();
    } else if (state == -5) {
        UpdateClassChange(ot);
    }
    DrawCourseArrows(1);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_CourseSelectOption);
    RunTimedDrawScript(CourseSelectMenuScript(), &g_UiScriptProgress, 0);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 1);
    DrawMenuLightBurst(7);
}

/* What the race is started with, once the screen has finished sliding off. */
static void HandOverToRace(s32 sceneId, s32 course) {
    g_SceneId = sceneId;
    g_CourseIndex = course;
    g_RaceProgress->course = course;
    g_RaceProgress->carIndex = g_PlayerCarIndex;
    g_RaceProgress->classIndex = g_GrandPrixClass;
    /* Outside a Grand Prix the slot carries the series instead of a balance,
     * because there is no money in time attack. */
    g_RaceProgress->money.value =
        (g_GrandPrixMode != 0) ? g_PlayerMoney : g_GrandPrixSeries;
}

static void EnterChosenScreen(void) {
    switch (GameMenuBusy) {
    case 1:
        /* To the car select screen, with the showroom put back at its start. */
        if (g_MenuViewOffset <= 0x3D08F) {
            return;
        }
        g_MenuScreen = 3;
        g_MenuHandlerIndex = 4;
        DrawOwnedCarCounter(0, 0);
        DrawBrowseArrows(0, 0, 0, 0);
        g_CarSwapToIndex = -1;
        g_MenuViewAngle = 0;
        g_MenuViewAngleTarget = 0;
        g_MenuViewOffset = 0x3D090;
        g_MenuViewOffsetTarget = 0;
        g_CarSwapFromIndex = g_PlayerCarIndex;
        break;
    case 2:
        if ((g_MenuOutgoingScreenProgress > 0) ||
            (g_MenuViewOffset <= 0x3D08F)) {
            return;
        }
        HandOverToRace(2, g_CourseIndex & 3);
        break;
    case 3:
        g_MenuScreen = 2;
        g_MenuHandlerIndex = 2;
        break;
    case 4:
        /* The saved-game route reaches the race through the record screen. */
        if ((g_MenuOutgoingScreenProgress > 0) ||
            (g_MenuViewOffset <= 0x3D08F)) {
            return;
        }
        HandOverToRace(0x18, SeriesCourseIndex());
        break;
    }
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
}

static void UpdateCourseSelectOutgoing(void) {
    g_MenuHandlerIndex = -1;
    g_MenuHandlerIndex2 = 1;
    DrawCourseArrows(-1);
    RunTimedDrawScript(CourseSelectMenuScript(), &g_UiScriptProgress, -1);
    RunTimedDrawScript(&g_UiChromeScript, &g_UiScriptProgress, 0);
    DrawFadingMenuSprites(g_UiScriptProgress, 2, g_CourseSelectOption);
    DrawMenuLightBurst(-9);
    if (g_UiScriptProgress <= 0) {
        EnterChosenScreen();
    }
}

void UpdateCourseSelectScreen(void) {
    void *ot = SCRATCH_OT_BASE_AS(void);
    s32 state = GameMenuBusy;

    g_MenuAltLayout = g_MenuAltLayoutSetting;
    if (g_GrandPrixMode != 0) {
        FlipCourseCard(&g_CourseCardSpinTarget, &g_CourseCardSpin,
                       &g_CourseCardPendingGrade);
    } else {
        DrawTimeAttackPlate(g_TimeAttackPlateStep);
    }
    DrawCarNamePlate(g_CarNamePlateStep, g_MenuPlateCarIndex, 0);
    DrawMenuCourseView();

    if (state == 0) {
        UpdateCourseSelectIdle();
    } else if (state < 0) {
        UpdateCourseSelectModal(ot, state);
    } else {
        UpdateCourseSelectOutgoing();
    }
}
