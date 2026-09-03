#include "game/audio.h"
#include "game/menu.h"
#include "game/menu_internal.h"

/* The two keys on the name grid that are not characters. */
#define TEAM_NAME_KEY_RUBOUT 0x2A
#define TEAM_NAME_KEY_END 0x2B

enum {
    TEAM_NAME_GRID_COLUMNS = 11,
    TEAM_NAME_GRID_ROW_STRIDE = 11,
    TEAM_NAME_GRID_LAST_ROW_OFFSET = 33,
};

s32 DrawTeamNameScreen(s32 step) {
    return AdvanceMenuFade(&g_TeamNameScreenProgress, step);
}

static s32 MoveTeamNameGridCursor(s32 cursor, u16 pressed) {
    if (pressed & PAD_UP) {
        cursor = cursor < TEAM_NAME_GRID_COLUMNS
                     ? cursor + TEAM_NAME_GRID_LAST_ROW_OFFSET
                     : cursor - TEAM_NAME_GRID_ROW_STRIDE;
    }
    if (pressed & PAD_DOWN) {
        cursor = cursor < TEAM_NAME_GRID_LAST_ROW_OFFSET
                     ? cursor + TEAM_NAME_GRID_ROW_STRIDE
                     : cursor - TEAM_NAME_GRID_LAST_ROW_OFFSET;
    }
    if (pressed & PAD_LEFT) {
        cursor = cursor % TEAM_NAME_GRID_COLUMNS != 0
                     ? cursor - 1
                     : cursor + TEAM_NAME_GRID_COLUMNS - 1;
    }
    if (pressed & PAD_RIGHT) {
        cursor = (cursor + 1) % TEAM_NAME_GRID_COLUMNS != 0
                     ? cursor + 1
                     : cursor - (TEAM_NAME_GRID_COLUMNS - 1);
    }
    return cursor;
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
    g_MenuViewAngle = 0x3E8000;
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
        g_TeamNameChars[g_TeamNameLength] = 0xA;
        return;
    }
    if (GameMenuCursor == TEAM_NAME_KEY_END) {
        PlaySoundCue(3);
        GameMenuBusy = 1;
        g_MenuOverlayPattern = 2;
        g_MenuViewOffsetTarget = 0x3D090;
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
    if (g_UiScriptProgress > 0 || g_MenuViewOffset <= 0x3D08F) {
        return;
    }

    g_MenuScreen = MENU_SCREEN_DESIGN_MODE;
    g_MenuHandlerIndex = MENU_SCREEN_DESIGN_MODE;
    UploadTeamNameTexture(g_TeamNameChars, g_TeamNameLength);
    g_UiScriptProgress = 0;
    GameMenuBusy = 0;
}

void UpdateTeamNameScreen(void) {
    if (g_TeamNameLength > MENU_TEAM_NAME_MAX_LENGTH) {
        g_TeamNameLength = MENU_TEAM_NAME_MAX_LENGTH;
    }
    g_MenuAltLayout = g_MenuAltLayoutSetting;
    DrawTeamNameCharModel();
    if (GameMenuBusy == 0) {
        UpdateTeamNameIdle();
    } else {
        UpdateTeamNameOutgoing();
    }
}
