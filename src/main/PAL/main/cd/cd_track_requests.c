#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"

typedef enum CdTrackRequestStep {
    CD_TRACK_WAIT_FOR_DRIVE = 0,
    CD_TRACK_SEND_SEEK = 1,
    CD_TRACK_WAIT_FOR_SEEK = 2,
    CD_TRACK_FINISH_SELECTION = 3,
    CD_TRACK_RESTART_WAIT_FOR_DRIVE = 4,
    CD_TRACK_RESTART_SEND_SEEK = 5,
    CD_TRACK_RESTART_WAIT_FOR_SEEK = 6,
    CD_TRACK_FINISH_RESTART = 7,
    CD_TRACK_WAIT_FOR_PAUSE = 8,
} CdTrackRequestStep;

typedef enum CdPlayRequestStep {
    CD_PLAY_WAIT_FOR_DRIVE = 0,
    CD_PLAY_SEND_COMMAND = 1,
    CD_PLAY_WAIT_FOR_COMMAND = 2,
    CD_PLAY_FINISH = 3,
} CdPlayRequestStep;

void StepCdTrackRequest(void) {
    s32 syncResult;

    switch (g_CdTrackStep) {
    case CD_TRACK_WAIT_FOR_DRIVE:
        if (CdSync(1, 0) == 0) {
            break;
        }
        /* Selecting a new track must silence the currently streaming one
         * before CdlSetloc. PsyQ's CD drive did that as part of the seek;
         * host backends otherwise keep the old CD-DA stream alive and the
         * volume reset below makes it audible again just before replay. */
        if (CdControl(CD_DRIVE_PAUSE, 0, 0) == 0) {
            break;
        }
        g_CdTrackStep = CD_TRACK_WAIT_FOR_PAUSE;
        break;
    case CD_TRACK_WAIT_FOR_PAUSE:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdFadeFrames = 0;
        g_CdTrackStep = CD_TRACK_SEND_SEEK;
        /* fall through */
    case CD_TRACK_SEND_SEEK:
        if (CdControl(CD_DRIVE_SEEK_PLAY,
                      &g_CdTrackLocs[g_CdTrackPending], 0) == 0) {
            break;
        }
        g_CdTrackStep = CD_TRACK_WAIT_FOR_SEEK;
        break;
    case CD_TRACK_WAIT_FOR_SEEK:
        syncResult = CdSync(1, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_CdTrackStep = CD_TRACK_FINISH_SELECTION;
            break;
        }
        if (syncResult == CD_SYNC_DISK_ERROR) {
            g_CdTrackStep = CD_TRACK_SEND_SEEK;
            break;
        }
        break;
    case CD_TRACK_FINISH_SELECTION:
        g_CdCurrentTrack = (u8)g_CdTrackPending;
        g_CdTrackPending = -1;
        g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
        SetCdVolume(g_CdVolume);
        break;
    case CD_TRACK_RESTART_WAIT_FOR_DRIVE:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdTrackStep = CD_TRACK_RESTART_SEND_SEEK;
        /* fall through */
    case CD_TRACK_RESTART_SEND_SEEK:
        if (CdControl(CD_DRIVE_SEEK_PLAY,
                      &g_CdTrackLocs[g_CdTrackPending], 0) == 0) {
            break;
        }
        g_CdTrackStep = CD_TRACK_RESTART_WAIT_FOR_SEEK;
        break;
    case CD_TRACK_RESTART_WAIT_FOR_SEEK:
        syncResult = CdSync(1, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_CdTrackStep = CD_TRACK_FINISH_RESTART;
            break;
        }
        if (syncResult == CD_SYNC_DISK_ERROR) {
            g_CdTrackStep = CD_TRACK_RESTART_SEND_SEEK;
            break;
        }
        break;
    case CD_TRACK_FINISH_RESTART:
        g_CdCurrentTrack = (u8)g_CdTrackPending;
        g_CdTrackPending = -1;
        g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
        break;
    }
}

void StepCdPlayRequest(void) {
    s32 syncResult;

    switch (g_CdCommandStep) {
    case CD_PLAY_WAIT_FOR_DRIVE:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdCommandStep = CD_PLAY_SEND_COMMAND;
        /* fall through */
    case CD_PLAY_SEND_COMMAND:
        if (CdControl(CD_DRIVE_PLAY, 0, 0) == 0) {
            break;
        }
        g_CdCommandStep = CD_PLAY_WAIT_FOR_COMMAND;
        break;
    case CD_PLAY_WAIT_FOR_COMMAND:
        syncResult = CdSync(1, 0);
        if (syncResult == CD_SYNC_COMPLETE) {
            g_CdCommandStep = CD_PLAY_FINISH;
            break;
        }
        if (syncResult == CD_SYNC_DISK_ERROR) {
            g_CdCommandStep = CD_PLAY_SEND_COMMAND;
            break;
        }
        break;
    case CD_PLAY_FINISH:
        g_CdCommandPending = CD_COMMAND_NONE;
        g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
        break;
    }
}
