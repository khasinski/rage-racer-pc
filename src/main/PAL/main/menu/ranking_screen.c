#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/menu_scripts_internal.h"
#include "game/race.h"

typedef enum RankingScreenState {
    RANKING_ENTER = 0,
    RANKING_EXIT_TO_COURSE_SELECT = 1,
    RANKING_MENU = -1,
    RANKING_MENU_CLOSING = -2,
    RANKING_TOTAL_TABLE = -3,
    RANKING_TOTAL_TABLE_CLOSING = -4,
    RANKING_LAP_TABLE = -5,
    RANKING_LAP_TABLE_CLOSING = -6
} RankingScreenState;

enum RankingOption {
    RANKING_OPTION_TOTAL,
    RANKING_OPTION_LAP,
    RANKING_OPTION_EXIT,
    RANKING_OPTION_COUNT,
};

static void DrawRankingScreenChrome(void) {
    RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, 0);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
}

static void UpdateRankingMenu(Ranking *ranking) {
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, ranking->cursor);
    if (RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, 1) ==
        0) {
        return;
    }

    g_MenuOverlayPattern = -1;
    if (g_PadPressed & PAD_UP) {
        PlaySoundCue(1);
        ranking->cursor = WrapMenuIndex(
            ranking->cursor, -1, RANKING_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_DOWN) {
        PlaySoundCue(1);
        ranking->cursor = WrapMenuIndex(
            ranking->cursor, 1, RANKING_OPTION_COUNT);
    }
    if (g_PadPressed & PAD_CONFIRM) {
        if (ranking->cursor == RANKING_OPTION_TOTAL ||
            ranking->cursor == RANKING_OPTION_LAP) {
            PlaySoundCue(2);
            GameMenuBusy = RANKING_MENU_CLOSING;
        } else if (ranking->cursor == RANKING_OPTION_EXIT) {
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

static void CloseRankingMenu(Ranking *ranking) {
    RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, ranking->cursor);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = ranking->cursor == RANKING_OPTION_TOTAL
            ? RANKING_TOTAL_TABLE
            : ranking->cursor == RANKING_OPTION_LAP
                ? RANKING_LAP_TABLE
                : RANKING_MENU;
    }
}

static void UpdateRankingTable(RankingTableKind table,
                               RankingScreenState closingState) {
    if (DrawRankingTable(&g_UiScriptProgress2, 1, table) != 0 &&
        (g_PadPressed & (PAD_CONFIRM | PAD_CANCEL)) != 0) {
        PlaySoundCue(3);
        GameMenuBusy = closingState;
    }
}

static void CloseRankingTable(RankingTableKind table) {
    DrawRankingTable(&g_UiScriptProgress2, -1, table);
    if (g_UiScriptProgress2 <= 0) {
        GameMenuBusy = RANKING_MENU;
    }
}

void UpdateRankingScreen(void) {
    Ranking *ranking = MenuRanking();
    RankingScreenState state;

    g_MenuAltLayout = 0;
    ranking->cursor = AddClampedMenuValue(
        ranking->cursor, 0, 0, RANKING_OPTION_COUNT - 1);
    DrawMenuCourseView(MenuCourseSelect());
    DrawMenuLightBurst(MenuWidgetState(), -9);
    state = (RankingScreenState)GameMenuBusy;
    if (state == RANKING_ENTER) {
        g_UiScriptProgress2 = 0;
        GameMenuBusy = RANKING_MENU;
        DrawFadingMenuSprites(0, 2, ranking->cursor);
        RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, 1);
        /*
         * Having just arrived, draw the frame and wait for the next one.
         * Falling through from here reaches the code that leaves the screen,
         * which ran on the very frame the screen opened and sent the player
         * straight back to the course select: the ranking could not be
         * entered at all.
         */
        DrawRankingScreenChrome();
        return;
    }
    if (state < 0) {
        switch (state) {
        case RANKING_MENU:
            UpdateRankingMenu(ranking);
            break;
        case RANKING_MENU_CLOSING:
            CloseRankingMenu(ranking);
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
        DrawRankingScreenChrome();
        return;
    }
    if (state != RANKING_EXIT_TO_COURSE_SELECT) {
        GameMenuBusy = RANKING_MENU;
        DrawRankingScreenChrome();
        return;
    }
    MenuBeginExit(MENU_SCREEN_RANKING);
    RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, ranking->cursor);
    RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    if (g_UiScriptProgress > 0) {
        return;
    }
    MenuActivateScreen(MENU_SCREEN_COURSE_SELECT);
    ranking->cursor = 0;
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
    MenuWidgetState()->timeAttackStep = 0;
    DrawTimeAttackPlate(MenuWidgetState());
    MenuWidgetState()->timeAttackStep = CourseSeries(g_CourseIndex) != 0 ? 1 : -1;
}
