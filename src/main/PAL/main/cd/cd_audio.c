#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "game/race.h"
#include "game/menu.h"
#include "psyq/snd.h"

long HostCdAudioEnded(void);

void InitCdAudio(void) {
    SsSetSpuInputAttr(0, 0, 1);
    SsSetSerialVol(0, 0x7FFF, 0x7FFF);
    g_CdModeParam = 7;
    CdControl(CD_DRIVE_SET_MODE, &g_CdModeParam, 0);
    BuildCdTrackTable();

    g_CdTrackPending = -1;
    g_CdCommandPending = CD_COMMAND_NONE;
    g_CdCurrentTrack = 2;
    g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
    g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
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
                g_CdTrackStep = CD_TRACK_RESTART_WAIT_FOR_DRIVE;
                g_CdCommandPending = CD_COMMAND_PLAY;
                g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
                g_CdTrackPending = g_CdCurrentTrack;
            }
        }
    }

    StepCdVolumeFade();
}
