#include "game/cd.h"
#include "game/music_director_legacy.h"

MusicDirectorState MusicDirectorLoadLegacyState(void) {
    MusicDirectorState state;
    state.trackPending = g_CdTrackPending;
    state.trackStep = g_CdTrackStep;
    state.commandPending = g_CdCommandPending;
    state.commandStep = g_CdCommandStep;
    state.currentTrack = g_CdCurrentTrack;
    state.restartOnResume = g_CdRestartOnResume;
    return state;
}

void MusicDirectorStoreLegacyState(const MusicDirectorState *state) {
    g_CdTrackPending = state->trackPending;
    g_CdTrackStep = state->trackStep;
    g_CdCommandPending = state->commandPending;
    g_CdCommandStep = state->commandStep;
    g_CdCurrentTrack = state->currentTrack;
    g_CdRestartOnResume = state->restartOnResume;
}
