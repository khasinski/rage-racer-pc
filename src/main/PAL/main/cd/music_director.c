#include "game/music_director.h"

static void MusicDirectorRequestCommand(AudioSession *state,
                                        CdCommandType command) {
    state->commandPending = command;
    state->commandStep = 0;
}

void MusicDirectorRequestTrack(AudioSession *state, s32 track) {
    state->trackPending = (u8)track;
    state->trackStep = 0;
    MusicDirectorRequestCommand(state, CD_COMMAND_NONE);
}

void MusicDirectorRequestPlay(AudioSession *state) {
    MusicDirectorRequestCommand(state, CD_COMMAND_PLAY);
}

void MusicDirectorRequestPause(AudioSession *state) {
    MusicDirectorRequestCommand(state, CD_COMMAND_PAUSE);
}

void MusicDirectorRequestResume(AudioSession *state) {
    if (state->restartOnResume != 0) {
        state->trackStep = 4;
        state->restartOnResume = 0;
        state->trackPending = state->currentTrack;
        MusicDirectorRequestCommand(state, CD_COMMAND_PLAY);
        return;
    }
    MusicDirectorRequestCommand(state, CD_COMMAND_RESUME);
}

void MusicDirectorReset(AudioSession *state) {
    state->trackPending = -1;
    state->trackStep = 0;
    state->currentTrack = 2;
    MusicDirectorRequestCommand(state, CD_COMMAND_NONE);
}

void MusicDirectorLoopCurrent(AudioSession *state) {
    state->trackStep = 4;
    state->trackPending = state->currentTrack;
    MusicDirectorRequestCommand(state, CD_COMMAND_PLAY);
}
