#ifndef GAME_TEAM_NAME_CONTROLLER_H
#define GAME_TEAM_NAME_CONTROLLER_H

#include "common.h"

typedef enum TeamNameCommand {
    TEAM_NAME_COMMAND_NONE,
    TEAM_NAME_COMMAND_APPEND,
    TEAM_NAME_COMMAND_DELETE,
    TEAM_NAME_COMMAND_BACK
} TeamNameCommand;

typedef struct TeamNameInputResult {
    s32 cursor;
    u8 moved;
    TeamNameCommand command;
} TeamNameInputResult;

TeamNameInputResult TeamNameHandleInput(s32 cursor, s32 nameLength,
                                        s32 cursorAnimation,
                                        u16 pressedRepeat, u16 pressed);

#endif
