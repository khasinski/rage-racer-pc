#include "common.h"
#include "game/cd.h"
#include "game/music_director_legacy.h"

void RequestCdTrack(s32 track) {
    MusicDirectorState state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestTrack(&state, track);
    MusicDirectorStoreLegacyState(&state);
}

void StartCdAudio(void) {
    MusicDirectorState state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestPlay(&state);
    MusicDirectorStoreLegacyState(&state);
}

void PauseCdAudio(void) {
    MusicDirectorState state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestPause(&state);
    MusicDirectorStoreLegacyState(&state);
}

void ResumeCdAudio(void) {
    MusicDirectorState state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestResume(&state);
    MusicDirectorStoreLegacyState(&state);
}

void ResetCdAudioState(void) {
    MusicDirectorState state = MusicDirectorLoadLegacyState();
    MusicDirectorReset(&state);
    MusicDirectorStoreLegacyState(&state);
}
