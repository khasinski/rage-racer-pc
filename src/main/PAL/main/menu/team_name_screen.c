#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"

typedef enum TeamNameScreenState {
    TEAM_NAME_IDLE = 0,
    TEAM_NAME_EXIT_TO_DESIGN = 1,
} TeamNameScreenState;

s32 DrawTeamNameScreen(s32 step) {
    return AdvanceMenuFade(&g_TeamNameScreenProgress, step);
}

static s32 MoveTeamNameGridCursor(s32 cursor, u16 pressed) {
    s32 row = cursor / MENU_TEAM_NAME_GRID_COLUMNS;
    s32 column = cursor % MENU_TEAM_NAME_GRID_COLUMNS;

    if (pressed & PAD_UP) {
        row = WrapMenuIndex(row, -1, MENU_TEAM_NAME_GRID_ROWS);
    }
    if (pressed & PAD_DOWN) {
        row = WrapMenuIndex(row, 1, MENU_TEAM_NAME_GRID_ROWS);
    }
    if (pressed & PAD_LEFT) {
        column = WrapMenuIndex(column, -1, MENU_TEAM_NAME_GRID_COLUMNS);
    }
    if (pressed & PAD_RIGHT) {
        column = WrapMenuIndex(column, 1, MENU_TEAM_NAME_GRID_COLUMNS);
    }
    return row * MENU_TEAM_NAME_GRID_COLUMNS + column;
}

static void UpdateTeamNameCursor(void) {
    u16 directions = g_PadPressedRepeat & PAD_DPAD;
    s32 cursor;

    if (directions == 0 || GameMenuCursorAnim >= 0) {
        return;
    }
    if (g_TeamNameLength < MENU_TEAM_NAME_MAX_LENGTH) {
        cursor = MoveTeamNameGridCursor(GameMenuCursor, directions);
    } else if (directions & (PAD_LEFT | PAD_RIGHT)) {
        cursor = GameMenuCursor == TEAM_NAME_KEY_RUBOUT ? TEAM_NAME_KEY_END
                                                        : TEAM_NAME_KEY_RUBOUT;
    } else {
        return;
    }

    GameMenuCursor = cursor;
    g_MenuViewAngleTarget = 0;
    g_MenuViewAngle = TEAM_NAME_CURSOR_ENTRY_ANGLE;
    GameMenuCursorAnim = cursor;
    PlaySoundCue(1);
}

static void ApplyTeamNameInput(void) {
    int confirm = (g_PadPressed & PAD_CONFIRM) != 0;
    int rubout = confirm ? GameMenuCursor == TEAM_NAME_KEY_RUBOUT
                         : (g_PadPressed & PAD_CANCEL) != 0;
    s32 newLength;

    if (!confirm && !rubout) {
        return;
    }
    if (rubout) {
        if (g_TeamNameLength == 0) {
            return;
        }
        PlaySoundCue(4);
        g_TeamNameLength--;
        g_TeamNameChars[g_TeamNameLength] = TEAM_NAME_HIDDEN_MODEL_KEY;
        return;
    }
    if (GameMenuCursor == TEAM_NAME_KEY_END) {
        PlaySoundCue(3);
        GameMenuBusy = TEAM_NAME_EXIT_TO_DESIGN;
        g_MenuOverlayPattern = 2;
        g_MenuViewOffsetTarget = MENU_VIEW_OFFSET_MAX;
        return;
    }

    if (g_TeamNameLength >= MENU_TEAM_NAME_MAX_LENGTH ||
        (u32)GameMenuCursor >= TEAM_NAME_KEY_RUBOUT) {
        return;
    }

    PlaySoundCue(2);
    newLength = g_TeamNameLength;
    g_TeamNameChars[newLength] = (u8)GameMenuCursor;
    if (newLength >= MENU_TEAM_NAME_MAX_LENGTH - 1) {
        GameMenuCursor = TEAM_NAME_KEY_END;
    }
    g_TeamNameLength = (u8)(newLength + 1);
}

static void UpdateTeamNameIdle(void) {
    DrawTeamNameEntry(1, GameMenuCursor);
    if (RunTimedDrawScript(g_TeamNameScreenScript, &g_UiScriptProgress, 1) ==
        0) {
        return;
    }
    g_MenuOverlayPattern = -1;
    UpdateTeamNameCursor();
    ApplyTeamNameInput();
}

static void UpdateTeamNameOutgoing(void) {
    g_MenuHandlerIndex = -1;
    g_MenuOutgoingHandlerIndex = MENU_SCREEN_TEAM_NAME;
    DrawTeamNameEntry(-1, GameMenuCursor);
    RunTimedDrawScript(g_TeamNameScreenScript, &g_UiScriptProgress, -1);
    if (g_UiScriptProgress > 0 ||
        g_MenuViewOffset < MENU_VIEW_OFFSET_MAX) {
        return;
    }

    g_MenuScreen = MENU_SCREEN_DESIGN_MODE;
    g_MenuHandlerIndex = MENU_SCREEN_DESIGN_MODE;
    UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
}

void UpdateTeamNameScreen(void) {
    TeamNameScreenState state = (TeamNameScreenState)GameMenuBusy;

    if (g_TeamNameLength > MENU_TEAM_NAME_MAX_LENGTH) {
        g_TeamNameLength = MENU_TEAM_NAME_MAX_LENGTH;
    }
    GameMenuCursor = NormalizeTeamNameCursor(GameMenuCursor);
    if (GameMenuCursorAnim >= 0) {
        GameMenuCursorAnim = NormalizeTeamNameCursor(GameMenuCursorAnim);
    }
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawTeamNameCharModel();
    if (state == TEAM_NAME_IDLE) {
        UpdateTeamNameIdle();
    } else if (state == TEAM_NAME_EXIT_TO_DESIGN) {
        UpdateTeamNameOutgoing();
    } else {
        GameMenuBusy = TEAM_NAME_IDLE;
    }
}
