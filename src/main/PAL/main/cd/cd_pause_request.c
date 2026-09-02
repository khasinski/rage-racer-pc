#include "game/cd.h"
#include "game/cd_internal.h"
#include "psyq/cd.h"

static void CaptureCdPauseLocation(void) {
    g_CdTrackElapsedLoc.minute = g_CdLocResult[2];
    g_CdTrackElapsedLoc.sector = 0;
    g_CdTrackElapsedLoc.second = g_CdLocResult[3];

    if (!CdTrackIndexValid(g_CdCurrentTrack)) {
        g_CdRestartOnResume = 0;
        return;
    }

    const s32 loopPoint =
        CdPosToInt_Local(&g_CdTrackLoopPoint[g_CdCurrentTrack]);
    const s32 firstLoopPoint = CdPosToInt_Local(&g_CdTrackLoopPoint[0]);

    g_CdRestartOnResume = CdPlaybackPassedLoopPoint(
        firstLoopPoint, loopPoint, CdPosToInt_Local(&g_CdTrackElapsedLoc));
}

void StepCdPauseRequest(void) {
    s32 syncResult;

    switch (g_CdCommandStep) {
    case CD_PAUSE_WAIT_FOR_DRIVE:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdCommandStep = CD_PAUSE_GET_LOCATION;
        RAGE_FALLTHROUGH;

    case CD_PAUSE_GET_LOCATION:
        if (CdControl(CD_DRIVE_GET_LOCATION, 0, g_CdLocResult) != 0) {
            g_CdCommandStep = CD_PAUSE_WAIT_FOR_LOCATION;
        }
        break;

    case CD_PAUSE_WAIT_FOR_LOCATION:
        syncResult = CdSync(1, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_CdCommandStep = CD_PAUSE_CAPTURE_LOCATION;
        } else if (syncResult == CD_SYNC_DISK_ERROR) {
            g_CdCommandStep = CD_PAUSE_GET_LOCATION;
        }
        break;

    case CD_PAUSE_CAPTURE_LOCATION:
        CaptureCdPauseLocation();
        g_CdCommandStep = CD_PAUSE_SEND_COMMAND;
        RAGE_FALLTHROUGH;

    case CD_PAUSE_SEND_COMMAND:
        if (CdControl(CD_DRIVE_PAUSE, 0, 0) != 0) {
            g_CdCommandStep = CD_PAUSE_WAIT_FOR_COMMAND;
        }
        break;

    case CD_PAUSE_WAIT_FOR_COMMAND:
        syncResult = CdSync(1, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_CdCommandStep = CD_PAUSE_FINISH;
        } else if (syncResult == CD_SYNC_DISK_ERROR) {
            g_CdCommandStep = CD_PAUSE_SEND_COMMAND;
        }
        break;

    case CD_PAUSE_FINISH:
        g_CdCommandPending = CD_COMMAND_NONE;
        g_CdCommandStep = CD_PAUSE_WAIT_FOR_DRIVE;
        break;
    }
}
