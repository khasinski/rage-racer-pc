#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "game/race.h"
#include "game/menu.h"
#include "psyq/snd.h"

enum {
    CD_AUDIO_MODE = CD_MODE_CDDA | CD_MODE_AUTO_PAUSE | CD_MODE_REPORT,
    BGM_SELECT_SCENE = 0x1C,
};

void InitCdAudio(void) {
    SsSetSpuInputAttr(0, 0, 1);
    SsSetSerialVol(0, 0x7FFF, 0x7FFF);
    g_CdModeParam = CD_AUDIO_MODE;
    CdControl(CD_DRIVE_SET_MODE, &g_CdModeParam, 0);
    BuildCdTrackTable();

    ResetCdAudioState();
    g_CdMixPreset = 0;
    g_CdRestartOnResume = 0;
    g_CdVolume = CD_VOLUME_MAX;
    g_CdFadeFrames = 0;
    SetCdVolume(CD_VOLUME_MAX);
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
        HostCdAudioEnded() && CdTrackIndexValid(g_CdCurrentTrack)) {
        if (g_SceneId == BGM_SELECT_SCENE) {
            g_CdTrackEnded = 1;
        } else {
            const s32 loopPoint =
                CdPosToInt_Local(&g_CdTrackLoopPoint[g_CdCurrentTrack]);
            const s32 firstLoopPoint =
                CdPosToInt_Local(&g_CdTrackLoopPoint[0]);

            if (CdTrackHasLoopPoint(firstLoopPoint, loopPoint)) {
                QueueCdTrackRestart(g_CdCurrentTrack);
            }
        }
    }

    StepCdVolumeFade();
}
