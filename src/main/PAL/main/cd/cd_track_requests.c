#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"

static void SendTrackSeek(CdTrackRequestStep waitStep) {
    if (CdControl(CD_DRIVE_SEEK_PLAY,
                  &g_CdTrackLocs[g_Cd.pendingTrack], 0) != 0) {
        g_Cd.trackStep = waitStep;
    }
}

static void WaitForTrackSeek(CdTrackRequestStep retryStep,
                             CdTrackRequestStep finishStep) {
    s32 syncResult = CdSync(CD_SYNC_POLL, 0);

    if (syncResult == CD_SYNC_COMPLETE) {
        g_Cd.trackStep = finishStep;
    } else if (syncResult == CD_SYNC_DISK_ERROR) {
        g_Cd.trackStep = retryStep;
    }
}

static void FinishTrackRequest(int restoreVolume) {
    g_Cd.currentTrack = (u8)g_Cd.pendingTrack;
    g_Cd.pendingTrack = -1;
    g_Cd.trackStep = CD_TRACK_WAIT_FOR_DRIVE;
    if (restoreVolume) {
        SetCdVolume(g_Cd.volume);
    }
}

void StepCdTrackRequest(void) {
    if (!CdTrackIndexValid(g_Cd.pendingTrack)) {
        g_Cd.pendingTrack = -1;
        g_Cd.trackStep = CD_TRACK_WAIT_FOR_DRIVE;
        return;
    }

    switch (g_Cd.trackStep) {
    case CD_TRACK_WAIT_FOR_DRIVE:
        if (CdSync(CD_SYNC_POLL, 0) == CD_SYNC_PENDING) {
            break;
        }
        /* Selecting a new track must silence the currently streaming one
         * before CdlSetloc. PsyQ's CD drive did that as part of the seek;
         * host backends otherwise keep the old CD-DA stream alive and the
         * volume reset below makes it audible again just before replay. */
        if (CdControl(CD_DRIVE_PAUSE, 0, 0) == 0) {
            break;
        }
        g_Cd.trackStep = CD_TRACK_WAIT_FOR_PAUSE;
        break;
    case CD_TRACK_WAIT_FOR_PAUSE:
        if (CdSync(CD_SYNC_POLL, 0) == CD_SYNC_PENDING) {
            break;
        }
        g_Cd.fade = 0;
        g_Cd.trackStep = CD_TRACK_SEND_SEEK;
        RAGE_FALLTHROUGH;
    case CD_TRACK_SEND_SEEK:
        SendTrackSeek(CD_TRACK_WAIT_FOR_SEEK);
        break;
    case CD_TRACK_WAIT_FOR_SEEK:
        WaitForTrackSeek(CD_TRACK_SEND_SEEK, CD_TRACK_FINISH_SELECTION);
        break;
    case CD_TRACK_FINISH_SELECTION:
        FinishTrackRequest(1);
        break;
    case CD_TRACK_RESTART_WAIT_FOR_DRIVE:
        if (CdSync(CD_SYNC_POLL, 0) == CD_SYNC_PENDING) {
            break;
        }
        g_Cd.trackStep = CD_TRACK_RESTART_SEND_SEEK;
        RAGE_FALLTHROUGH;
    case CD_TRACK_RESTART_SEND_SEEK:
        SendTrackSeek(CD_TRACK_RESTART_WAIT_FOR_SEEK);
        break;
    case CD_TRACK_RESTART_WAIT_FOR_SEEK:
        WaitForTrackSeek(CD_TRACK_RESTART_SEND_SEEK,
                         CD_TRACK_FINISH_RESTART);
        break;
    case CD_TRACK_FINISH_RESTART:
        FinishTrackRequest(0);
        break;
    default:
        g_Cd.pendingTrack = -1;
        g_Cd.trackStep = CD_TRACK_WAIT_FOR_DRIVE;
        break;
    }
}
