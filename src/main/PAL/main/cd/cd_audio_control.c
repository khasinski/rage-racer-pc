#include "game/cd.h"
#include "game/cd_internal.h"

static void QueueCdCommand(CdCommandType command, s32 firstStep) {
    g_Cd.pendingCommand = command;
    g_Cd.commandStep = firstStep;
}

void QueueCdTrackRestart(s32 track) {
    if (!CdTrackIndexValid(track)) {
        return;
    }

    g_Cd.trackStep = CD_TRACK_RESTART_WAIT_FOR_DRIVE;
    QueueCdCommand(CD_COMMAND_PLAY, CD_PLAY_WAIT_FOR_DRIVE);
    g_Cd.pendingTrack = track;
}

void RequestCdTrack(s32 track) {
    if (!CdTrackIndexValid(track)) {
        return;
    }
    g_Cd.pendingTrack = track;
    g_Cd.trackStep = CD_TRACK_WAIT_FOR_DRIVE;
    QueueCdCommand(CD_COMMAND_NONE, CD_PLAY_WAIT_FOR_DRIVE);
}

void StartCdAudio(void) {
    QueueCdCommand(CD_COMMAND_PLAY, CD_PLAY_WAIT_FOR_DRIVE);
}

void PauseCdAudio(void) {
    QueueCdCommand(CD_COMMAND_PAUSE, CD_PAUSE_WAIT_FOR_DRIVE);
}

void ResumeCdAudio(void) {
    s32 restartTrack = g_Cd.restart != 0 &&
                       CdTrackIndexValid(g_Cd.currentTrack);

    g_Cd.restart = 0;
    if (restartTrack) {
        QueueCdTrackRestart(g_Cd.currentTrack);
    } else {
        QueueCdCommand(CD_COMMAND_RESUME, CD_PLAY_WAIT_FOR_DRIVE);
    }
}

void ResetCdAudioState(void) {
    g_Cd.pendingTrack = -1;
    g_Cd.trackStep = CD_TRACK_WAIT_FOR_DRIVE;
    QueueCdCommand(CD_COMMAND_NONE, CD_PLAY_WAIT_FOR_DRIVE);
    g_Cd.currentTrack = CD_INITIAL_TRACK;
}
