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
        RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }
    if (state < 0) {
        switch (state) {
        case RANKING_MENU:
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, 1) != 0) {
                g_MenuOverlayPattern = -1;
                if (g_PadPressed & PAD_UP) {
                    PlaySoundCue(1);
                    g_RankingCursor = (g_RankingCursor > 0) ? g_RankingCursor - 1 : 2;
                }
                if (g_PadPressed & PAD_DOWN) {
                    PlaySoundCue(1);
                    g_RankingCursor = (g_RankingCursor < 2) ? g_RankingCursor + 1 : 0;
                }
                if (g_PadPressed & PAD_CONFIRM) {
                    if (g_RankingCursor == 0) {
                        PlaySoundCue(2);
                        GameMenuBusy = RANKING_MENU_CLOSING;
                        g_RankingPendingState = RANKING_TOTAL_TABLE;
                    } else if (g_RankingCursor == 1) {
                        PlaySoundCue(2);
                        GameMenuBusy = RANKING_MENU_CLOSING;
                        g_RankingPendingState = RANKING_LAP_TABLE;
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
            break;
        case RANKING_MENU_CLOSING:
            RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
            DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = g_RankingPendingState;
            break;
        case RANKING_TOTAL_TABLE:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 0) == 0) {
                break;
            }
            if (!(g_PadPressed & (PAD_CONFIRM | PAD_CANCEL))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = RANKING_TOTAL_TABLE_CLOSING;
            break;
        case RANKING_TOTAL_TABLE_CLOSING:
            DrawRankingTable(&g_UiScriptProgress2, -1, 0);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = RANKING_MENU;
            break;
        case RANKING_LAP_TABLE:
            if (DrawRankingTable(&g_UiScriptProgress2, 1, 1) == 0) {
                break;
            }
            if (!(g_PadPressed & (PAD_CONFIRM | PAD_CANCEL))) {
                break;
            }
            PlaySoundCue(3);
            GameMenuBusy = RANKING_LAP_TABLE_CLOSING;
            break;
        case RANKING_LAP_TABLE_CLOSING:
            DrawRankingTable(&g_UiScriptProgress2, -1, 1);
            if (g_UiScriptProgress2 > 0) {
                break;
            }
            GameMenuBusy = RANKING_MENU;
            break;
        }
        RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, 0);
        RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 1);
        return;
    }
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = 2;
    RunTimedDrawScript(g_RankingMenuScript, &g_UiScriptProgress2, -1);
    DrawFadingMenuSprites(g_UiScriptProgress2, 2, g_RankingCursor);
    RunTimedDrawScript(g_RankingPanelScript, &g_UiScriptProgress, -1);
    RunTimedDrawScript(g_UiChromeScript, &g_UiScriptProgress, 0);
    if (g_UiScriptProgress > 0) {
        return;
    }
    g_MenuScreen = 1;
    g_MenuHandlerIndex = 1;
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
