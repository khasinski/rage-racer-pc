#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"

enum RankingScreenState {
    RANKING_MENU = -1,
    RANKING_MENU_CLOSING = -2,
    RANKING_TOTAL_TABLE = -3,
    RANKING_TOTAL_TABLE_CLOSING = -4,
    RANKING_LAP_TABLE = -5,
    RANKING_LAP_TABLE_CLOSING = -6
};

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
        g_RankingCursor = g_RankingCursor > 0 ? g_RankingCursor - 1 : 2;
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        g_RankingCursor = g_RankingCursor < 2 ? g_RankingCursor + 1 : 0;
    }
    if (g_PadPressed & PAD_CONFIRM) {
        if (g_RankingCursor == 0 || g_RankingCursor == 1) {
            PlaySoundCue(2);
            GameMenuBusy = RANKING_MENU_CLOSING;
            g_RankingPendingState = g_RankingCursor == 0
                ? RANKING_TOTAL_TABLE
                : RANKING_LAP_TABLE;
        } else if (g_RankingCursor == 2) {
            PlaySoundCue(3);
            GameMenuBusy = 1;
            g_MenuOverlayPattern = 2;
        }
    } else if (g_PadPressed & PAD_CANCEL) {
        PlaySoundCue(3);
        GameMenuBusy = 1;
        g_MenuOverlayPattern = 2;
    }
}

static void CloseRankingMenu(void) {
    RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = g_RankingPendingState;
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
    DrawMenuCourseView();
    DrawMenuLightBurst(-9);
    state = GameMenuBusy;
    if (state == 0) {
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
            UpdateRankingTable(0, RANKING_TOTAL_TABLE_CLOSING);
            break;
        case RANKING_TOTAL_TABLE_CLOSING:
            CloseRankingTable(0);
            break;
        case RANKING_LAP_TABLE:
            UpdateRankingTable(1, RANKING_LAP_TABLE_CLOSING);
            break;
        case RANKING_LAP_TABLE_CLOSING:
            CloseRankingTable(1);
            break;
        }
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
    if (g_CourseIndex >= 4) {
        g_TimeAttackPlateStep = 1;
    } else {
        g_TimeAttackPlateStep = -1;
    }
}
