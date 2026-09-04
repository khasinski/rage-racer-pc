#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"

enum RankingScreenState {
    RANKING_ENTER = 0,
    RANKING_EXIT_TO_COURSE_SELECT = 1,
    RANKING_MENU = -1,
    RANKING_MENU_CLOSING = -2,
    RANKING_TOTAL_TABLE = -3,
    RANKING_TOTAL_TABLE_CLOSING = -4,
    RANKING_LAP_TABLE = -5,
    RANKING_LAP_TABLE_CLOSING = -6
};

enum RankingOption {
    RANKING_OPTION_TOTAL,
    RANKING_OPTION_LAP,
    RANKING_OPTION_EXIT,
    RANKING_OPTION_COUNT,
};

enum RankingTable {
    RANKING_TABLE_TOTAL,
    RANKING_TABLE_LAP,
};

/* Screen-fade callback used by the host menu renderer. */
s32 DrawRankingScreen(s32 step) {
    return AdvanceMenuFade(&g_RankingScrollState, step);
}

static void DrawRankingScreenChrome(s32 panelStep) {
    RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, panelStep);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress,
                       panelStep >= 0);
}

static void UpdateRankingMenu(void) {
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    if (RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, 1) ==
        0) {
        return;
    }

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        g_RankingCursor = WrapMenuIndex(
            g_RankingCursor, -1, RANKING_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_RankingCursor = WrapMenuIndex(
            g_RankingCursor, 1, RANKING_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        if (g_RankingCursor == RANKING_OPTION_TOTAL ||
            g_RankingCursor == RANKING_OPTION_LAP) {
            PlaySoundCue(2);
            GameMenuBusy = RANKING_MENU_CLOSING;
            g_RankingPendingState = g_RankingCursor == RANKING_OPTION_TOTAL
                ? RANKING_TOTAL_TABLE
                : RANKING_LAP_TABLE;
        } else if (g_RankingCursor == RANKING_OPTION_EXIT) {
            PlaySoundCue(3);
            GameMenuBusy = RANKING_EXIT_TO_COURSE_SELECT;
            g_MenuOverlayPattern = 2;
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = RANKING_EXIT_TO_COURSE_SELECT;
        g_MenuOverlayPattern = 2;
    }
}

static void CloseRankingMenu(void) {
    RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    if (g_UiScriptProgress2 <= 0) {
        if (g_RankingPendingState == RANKING_TOTAL_TABLE ||
            g_RankingPendingState == RANKING_LAP_TABLE) {
            GameMenuBusy = g_RankingPendingState;
        } else {
            GameMenuBusy = RANKING_MENU;
        }
    }
}

static void UpdateRankingTable(s32 table, s32 closingState) {
    if (DrawRankingTable(&g_UiScriptProgress2, 1, table) != 0 &&
        (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) != 0) {
        PlaySoundCue(3);
        GameMenuBusy = closingState;
    }
}

static void CloseRankingTable(s32 table) {
    DrawRankingTable(&g_UiScriptProgress2, -1, table);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = RANKING_MENU;
    }
}

void UpdateRankingScreen(void) {
    s32 state;

    g_MenuAltLayout = 0;
    g_RankingCursor = AddClampedMenuValue(
        g_RankingCursor, 0, 0, RANKING_OPTION_COUNT - 1);
    DrawMenuCourseView();
    DrawMenuLightBurst(-9);
    state = GameMenuBusy;
    if (state == RANKING_ENTER) {
        g_UiScriptProgress2 = 0;
        GameMenuBusy = RANKING_MENU;
        DrawFadingMenuSprites(0, 2, g_RankingCursor);
        RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, 1);
        /*
         * Having just arrived, draw the frame and wait for the next one.
         * Falling through from here reaches the code that leaves the screen,
         * which ran on the very frame the screen opened and sent the player
         * straight back to the course select: the ranking could not be
         * entered at all.
         */
        DrawRankingScreenChrome(0);
        return;
    }
    if (state < 0) {
        switch (state) {
        case RANKING_MENU:
            UpdateRankingMenu();
            break;
        case RANKING_MENU_CLOSING:
            CloseRankingMenu();
            break;
        case RANKING_TOTAL_TABLE:
            UpdateRankingTable(RANKING_TABLE_TOTAL,
                               RANKING_TOTAL_TABLE_CLOSING);
            break;
        case RANKING_TOTAL_TABLE_CLOSING:
            CloseRankingTable(RANKING_TABLE_TOTAL);
            break;
        case RANKING_LAP_TABLE:
            UpdateRankingTable(RANKING_TABLE_LAP, RANKING_LAP_TABLE_CLOSING);
            break;
        case RANKING_LAP_TABLE_CLOSING:
            CloseRankingTable(RANKING_TABLE_LAP);
            break;
        default:
            GameMenuBusy = RANKING_MENU;
            break;
        }
        DrawRankingScreenChrome(0);
        return;
    }
    if (state != RANKING_EXIT_TO_COURSE_SELECT) {
        GameMenuBusy = RANKING_MENU;
        DrawRankingScreenChrome(0);
        return;
    }
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_RANKING;
    RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    if (g_UiScriptProgress > 0) {
        return;
    }
    g_MenuScreen = MENU_SCREEN_COURSE_SELECT;
    g_MenuHandlerIndex = MENU_SCREEN_COURSE_SELECT;
    g_RankingCursor = 0;
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
    DrawTimeAttackPlate(0);
    g_TimeAttackPlateStep = CourseSeries(g_CourseIndex) != 0 ? 1 : -1;
}
