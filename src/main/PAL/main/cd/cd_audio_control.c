#include "game/cd.h"
#include "game/cd_internal.h"

static void QueueCdCommand(CdCommandType command, s32 firstStep) {
    g_CdCommandPending = command;
    g_CdCommandStep = firstStep;
}

void QueueCdTrackRestart(s32 track) {
    if (!CdTrackIndexValid(track)) {
        return;
    }

    g_CdTrackStep = CD_TRACK_RESTART_WAIT_FOR_DRIVE;
    QueueCdCommand(CD_COMMAND_PLAY, CD_PLAY_WAIT_FOR_DRIVE);
    g_CdTrackPending = track;
}

void RequestCdTrack(s32 track) {
    if (!CdTrackIndexValid(track)) {
        return;
    }
    g_CdTrackPending = track;
    g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
    QueueCdCommand(CD_COMMAND_NONE, CD_PLAY_WAIT_FOR_DRIVE);
}

void StartCdAudio(void) {
    QueueCdCommand(CD_COMMAND_PLAY, CD_PLAY_WAIT_FOR_DRIVE);
}

void PauseCdAudio(void) {
    QueueCdCommand(CD_COMMAND_PAUSE, CD_PAUSE_WAIT_FOR_DRIVE);
}

void ResumeCdAudio(void) {
    s32 restartTrack = g_CdRestartOnResume != 0 &&
                       CdTrackIndexValid(g_CdCurrentTrack);

    g_CdRestartOnResume = 0;
    if (restartTrack) {
        QueueCdTrackRestart(g_CdCurrentTrack);
    } else {
        QueueCdCommand(CD_COMMAND_RESUME, CD_PLAY_WAIT_FOR_DRIVE);
    }
}

void ResetCdAudioState(void) {
    g_CdTrackPending = -1;
    g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
    QueueCdCommand(CD_COMMAND_NONE, CD_PLAY_WAIT_FOR_DRIVE);
    g_CdCurrentTrack = CD_INITIAL_TRACK;
}
