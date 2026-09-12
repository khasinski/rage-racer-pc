#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

void StepCdPlayRequest(void) {
    s32 syncResult;

    switch (g_Cd.commandStep) {
    case CD_PLAY_WAIT_FOR_DRIVE:
        if (CdSync(CD_SYNC_POLL, 0) == CD_SYNC_PENDING) {
            break;
        }
        g_Cd.commandStep = CD_PLAY_SEND_COMMAND;
        RAGE_FALLTHROUGH;

    case CD_PLAY_SEND_COMMAND:
        if (CdControl(CD_DRIVE_PLAY, 0, 0) != 0) {
            g_Cd.commandStep = CD_PLAY_WAIT_FOR_COMMAND;
        }
        break;

    case CD_PLAY_WAIT_FOR_COMMAND:
        syncResult = CdSync(CD_SYNC_POLL, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_Cd.commandStep = CD_PLAY_FINISH;
        } else if (syncResult == CD_SYNC_DISK_ERROR) {
            g_Cd.commandStep = CD_PLAY_SEND_COMMAND;
        }
        break;

    case CD_PLAY_FINISH:
        g_Cd.pendingCommand = CD_COMMAND_NONE;
        g_Cd.commandStep = CD_PLAY_WAIT_FOR_DRIVE;
        break;
    default:
        g_Cd.pendingCommand = CD_COMMAND_NONE;
        g_Cd.commandStep = CD_PLAY_WAIT_FOR_DRIVE;
        break;
    }
}
