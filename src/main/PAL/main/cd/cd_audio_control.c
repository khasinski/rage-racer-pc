#include "game/cd.h"
#include "game/cd_internal.h"
void RequestCdTrack(s32 track) {
    g_CdTrackPending = (u8)track;
    g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
    g_CdCommandPending = CD_COMMAND_NONE;
    g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
}

void StartCdAudio(void) {
    g_CdCommandPending = CD_COMMAND_PLAY;
    g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
}

void PauseCdAudio(void) {
    g_CdCommandPending = CD_COMMAND_PAUSE;
    g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
}

void ResumeCdAudio(void) {
    if (g_CdRestartOnResume != 0) {
        g_CdTrackStep = CD_TRACK_RESTART_WAIT_FOR_DRIVE;
        g_CdRestartOnResume = 0;
        g_CdCommandPending = CD_COMMAND_PLAY;
        g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
        g_CdTrackPending = g_CdCurrentTrack;
    } else {
        g_CdCommandPending = CD_COMMAND_RESUME;
        g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
    }
}

void ResetCdAudioState(void) {
    g_CdTrackPending = -1;
    g_CdCommandPending = CD_COMMAND_NONE;
    g_CdTrackStep = CD_TRACK_WAIT_FOR_DRIVE;
    g_CdCommandStep = CD_PLAY_WAIT_FOR_DRIVE;
    g_CdCurrentTrack = 2;
}
