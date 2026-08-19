#include "game/team_name_controller.h"
#include "game/pad.h"

enum {
    TEAM_NAME_GRID_COLUMNS = 11,
    TEAM_NAME_GRID_LAST_CHARACTER = 41,
    TEAM_NAME_DELETE_CURSOR = 42,
    TEAM_NAME_BACK_CURSOR = 43,
    TEAM_NAME_MAX_LENGTH = 6
};

static s32 MoveCharacterCursor(s32 cursor, u16 pressedRepeat) {
    if ((pressedRepeat & PAD_UP) != 0) {
        cursor = cursor < 11 ? cursor + 33 : cursor - 11;
    }
    if ((pressedRepeat & PAD_DOWN) != 0) {
        cursor = cursor < 33 ? cursor + 11 : cursor - 33;
    }
    if ((pressedRepeat & PAD_LEFT) != 0) {
        cursor = cursor % TEAM_NAME_GRID_COLUMNS != 0
            ? cursor - 1 : cursor + 10;
    }
    if ((pressedRepeat & PAD_RIGHT) != 0) {
        cursor = (cursor + 1) % TEAM_NAME_GRID_COLUMNS != 0
            ? cursor + 1 : cursor - 10;
    }
    return cursor;
}

TeamNameInputResult TeamNameHandleInput(s32 cursor, s32 nameLength,
                                        s32 cursorAnimation,
                                        u16 pressedRepeat, u16 pressed) {
    TeamNameInputResult result;

    result.cursor = cursor;
    result.moved = 0;
    result.command = TEAM_NAME_COMMAND_NONE;
    if (cursorAnimation < 0) {
        if (nameLength < TEAM_NAME_MAX_LENGTH &&
            (pressedRepeat & PAD_DPAD) != 0) {
            result.cursor = MoveCharacterCursor(cursor, pressedRepeat);
            result.moved = 1;
        } else if (nameLength >= TEAM_NAME_MAX_LENGTH &&
                   (pressedRepeat & (PAD_LEFT | PAD_RIGHT)) != 0) {
            result.cursor = cursor == TEAM_NAME_DELETE_CURSOR
                ? TEAM_NAME_BACK_CURSOR : TEAM_NAME_DELETE_CURSOR;
            result.moved = 1;
        }
    }

    if ((pressed & PAD_CONFIRM) != 0) {
        if (result.cursor == TEAM_NAME_DELETE_CURSOR) {
            result.command = TEAM_NAME_COMMAND_DELETE;
        } else if (result.cursor == TEAM_NAME_BACK_CURSOR) {
            result.command = TEAM_NAME_COMMAND_BACK;
        } else if (result.cursor >= 0 &&
                   result.cursor <= TEAM_NAME_GRID_LAST_CHARACTER) {
            result.command = TEAM_NAME_COMMAND_APPEND;
        }
    } else if ((pressed & PAD_CANCEL) != 0) {
        result.command = TEAM_NAME_COMMAND_DELETE;
    }
    return result;
}
