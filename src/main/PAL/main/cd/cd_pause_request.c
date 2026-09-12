#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

static void CaptureCdPauseLocation(void) {
    s32 loopPoint;
    s32 firstLoopPoint;

    g_Cd.elapsed.minute = g_Cd.result[2];
    g_Cd.elapsed.sector = 0;
    g_Cd.elapsed.second = g_Cd.result[3];

    if (!CdTrackIndexValid(g_Cd.currentTrack)) {
        g_Cd.restart = 0;
        return;
    }

    loopPoint = CdPosToInt(&g_CdTrackLoopPoint[g_Cd.currentTrack]);
    firstLoopPoint = CdPosToInt(&g_CdTrackLoopPoint[0]);

    g_Cd.restart = CdPlaybackPassedLoopPoint(
        firstLoopPoint, loopPoint, CdPosToInt(&g_Cd.elapsed));
}

void StepCdPauseRequest(void) {
    s32 syncResult;

    switch (g_Cd.commandStep) {
    case CD_PAUSE_WAIT_FOR_DRIVE:
        if (CdSync(CD_SYNC_POLL, 0) == CD_SYNC_PENDING) {
            break;
        }
        g_Cd.commandStep = CD_PAUSE_GET_LOCATION;
        RAGE_FALLTHROUGH;

    case CD_PAUSE_GET_LOCATION:
        if (CdControl(CD_DRIVE_GET_LOCATION, 0, g_Cd.result) != 0) {
            g_Cd.commandStep = CD_PAUSE_WAIT_FOR_LOCATION;
        }
        break;

    case CD_PAUSE_WAIT_FOR_LOCATION:
        syncResult = CdSync(CD_SYNC_POLL, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_Cd.commandStep = CD_PAUSE_CAPTURE_LOCATION;
        } else if (syncResult == CD_SYNC_DISK_ERROR) {
            g_Cd.commandStep = CD_PAUSE_GET_LOCATION;
        }
        break;

    case CD_PAUSE_CAPTURE_LOCATION:
        CaptureCdPauseLocation();
        g_Cd.commandStep = CD_PAUSE_SEND_COMMAND;
        RAGE_FALLTHROUGH;

    case CD_PAUSE_SEND_COMMAND:
        if (CdControl(CD_DRIVE_PAUSE, 0, 0) != 0) {
            g_Cd.commandStep = CD_PAUSE_WAIT_FOR_COMMAND;
        }
        break;

    case CD_PAUSE_WAIT_FOR_COMMAND:
        syncResult = CdSync(CD_SYNC_POLL, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_Cd.commandStep = CD_PAUSE_FINISH;
        } else if (syncResult == CD_SYNC_DISK_ERROR) {
            g_Cd.commandStep = CD_PAUSE_SEND_COMMAND;
        }
        break;

    case CD_PAUSE_FINISH:
        g_Cd.pendingCommand = CD_COMMAND_NONE;
        g_Cd.commandStep = CD_PAUSE_WAIT_FOR_DRIVE;
        break;
    default:
        g_Cd.pendingCommand = CD_COMMAND_NONE;
        g_Cd.commandStep = CD_PAUSE_WAIT_FOR_DRIVE;
        break;
    }
}
