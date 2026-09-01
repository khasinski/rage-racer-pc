#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "game/race.h"
#include "game/menu.h"
#include "psyq/snd.h"

long HostCdAudioEnded(void);


void StepCdPauseRequest(void) {
    s32 syncResult;

    switch (g_CdCommandStep) {
    case 0:
        if (CdSync(1, 0) == 0) {
            break;
        }
        g_CdCommandStep = 1;
        /* fallthrough */

    case 1:
        if (CdControl(CD_DRIVE_GET_LOCATION, 0, g_CdLocResult) != 0) {
            g_CdCommandStep = 2;
        }
        break;

    case 2:
        syncResult = CdSync(1, 0);
        if (syncResult == 2) {
            g_CdCommandStep = 3;
        } else if (syncResult == 5) {
            g_CdCommandStep = 1;
        }
        break;

    case 3:
    {
        const s32 loopPoint =
            CdPosToInt_Local(&g_CdTrackLoopPoint[g_CdCurrentTrack]);
        const s32 firstLoopPoint = CdPosToInt_Local(&g_CdTrackLoopPoint[0]);
        s32 elapsed;

        g_CdTrackElapsedLoc.minute = g_CdLocResult[2];
        g_CdTrackElapsedLoc.sector = 0;
        g_CdTrackElapsedLoc.second = g_CdLocResult[3];
        elapsed = CdPosToInt_Local(&g_CdTrackElapsedLoc);

        g_CdRestartOnResume =
            CdPlaybackPassedLoopPoint(firstLoopPoint, loopPoint, elapsed);

        g_CdCommandStep = 4;
        /* fallthrough */
    }

    case 4:
        if (CdControl(CD_DRIVE_PAUSE, 0, 0) != 0) {
            g_CdCommandStep = 5;
        }
        break;

    case 5:
        syncResult = CdSync(1, 0);
        if (syncResult == 2) {
            g_CdCommandStep = 6;
        } else if (syncResult == 5) {
            g_CdCommandStep = 4;
        }
        break;

    case 6:
        g_CdCommandPending = CD_COMMAND_NONE;
        g_CdCommandStep = 0;
        break;
    }
}

void InitCdAudio(void) {
    SsSetSpuInputAttr(0, 0, 1);
    SsSetSerialVol(0, 0x7FFF, 0x7FFF);
    g_CdModeParam = 7;
    CdControl(CD_DRIVE_SET_MODE, &g_CdModeParam, 0);
    BuildCdTrackTable();

    g_CdTrackPending = -1;
    g_CdCommandPending = CD_COMMAND_NONE;
    g_CdCurrentTrack = 2;
    g_CdTrackStep = 0;
    g_CdCommandStep = 0;
    g_CdMixPreset = 0;
    g_CdRestartOnResume = 0;
    g_CdVolume = 0x7F;
    g_CdFadeFrames = 0;
    SetCdVolume(0x7F);
}

void TickCdAudio(void) {
    if (g_CdTrackPending < 0) {
        switch (g_CdCommandPending) {
        case CD_COMMAND_NONE:
            break;
        /* Resuming and starting are the same command sequence: both issue
         * CdlPlay and wait on the same three steps. They were written out
         * twice, in two files, identical but for whether the status test
         * used else or a second break. */
        case CD_COMMAND_PLAY:
        case CD_COMMAND_RESUME:
            StepCdPlayRequest();
            break;
        case CD_COMMAND_PAUSE:
            StepCdPauseRequest();
            break;
        }
    } else {
        StepCdTrackRequest();
    }

    /* The host EOF flag stays asserted until CdlPlay opens the track again.
     * Do not let repeated ticks rewind an in-flight restart back to its first
     * seek step, or playback can never reach the command that clears EOF. */
    if (CdAudioRequestsIdle(g_CdTrackPending, g_CdCommandPending) &&
        HostCdAudioEnded()) {
        if (g_SceneId == 0x1C) {
            g_CdTrackEnded = 1;
        } else {
            const s32 loopPoint =
                CdPosToInt_Local(&g_CdTrackLoopPoint[g_CdCurrentTrack]);
            const s32 firstLoopPoint =
                CdPosToInt_Local(&g_CdTrackLoopPoint[0]);

            if (CdTrackHasLoopPoint(firstLoopPoint, loopPoint)) {
                g_CdTrackStep = 4;
                g_CdCommandPending = CD_COMMAND_PLAY;
                g_CdCommandStep = 0;
                g_CdTrackPending = g_CdCurrentTrack;
            }
        }
    }

    StepCdVolumeFade();
}
