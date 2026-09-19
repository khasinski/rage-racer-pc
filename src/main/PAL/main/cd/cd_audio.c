#include <psyz/cd.h>

#include "psyq/cd.h"
#include "game/cd.h"
#include "game/cd_internal.h"
#include "game/race.h"
#include "game/menu.h"
#include "game/scene.h"
#include "psyq/snd.h"

enum {
    CD_AUDIO_MODE = CD_MODE_CDDA | CD_MODE_AUTO_PAUSE | CD_MODE_REPORT,
};

void InitCdAudio(void) {
    SsSetSerialVol(0, 0x7FFF, 0x7FFF);
    g_Cd.mode = CD_AUDIO_MODE;
    CdControl(CD_DRIVE_SET_MODE, &g_Cd.mode, 0);
    BuildCdTrackTable();

    ResetCdAudioState();
    g_Cd.restart = 0;
    g_Cd.volume = CD_VOLUME_MAX;
    g_Cd.fade = 0;
    SetCdVolume(CD_VOLUME_MAX);
}

void TickCdAudio(void) {
    if (g_Cd.pendingTrack < 0) {
        switch (g_Cd.pendingCommand) {
        case CD_COMMAND_NONE:
            break;
        case CD_COMMAND_PLAY:
            StepCdPlayRequest();
            break;
        case CD_COMMAND_PAUSE:
            StepCdPauseRequest();
            break;
        default:
            g_Cd.pendingCommand = CD_COMMAND_NONE;
            g_Cd.commandStep = CD_PLAY_WAIT_FOR_DRIVE;
            break;
        }
    } else {
        StepCdTrackRequest();
    }

    if (Psyz_CdAudioPlaying()) {
        g_Cd.playedSinceSelect = 1;
    }

    /* The host EOF flag stays asserted until CdlPlay opens the track again.
     * Do not let repeated ticks rewind an in-flight restart back to its first
     * seek step, or playback can never reach the command that clears EOF.
     * It also survives the pause and seek of a new selection, so an end
     * reached by the previous track must not start this one early: the race
     * BGM is seeked at scene entry and played only after the countdown. */
    if (CdAudioRequestsIdle(g_Cd.pendingTrack, g_Cd.pendingCommand) &&
        g_Cd.playedSinceSelect && Psyz_CdAudioEnded() &&
        CdTrackIndexValid(g_Cd.currentTrack)) {
        if (g_SceneId == GAME_SCENE_BGM_SELECT) {
            g_CdTrackEnded = 1;
        } else {
            const s32 loopPoint =
                CdPosToInt(&g_CdTrackLoopPoint[g_Cd.currentTrack]);
            const s32 firstLoopPoint =
                CdPosToInt(&g_CdTrackLoopPoint[0]);

            if (CdTrackHasLoopPoint(firstLoopPoint, loopPoint)) {
                QueueCdTrackRestart(g_Cd.currentTrack);
            }
        }
    }

    StepCdVolumeFade();
}
