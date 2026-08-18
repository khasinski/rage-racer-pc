#include "common.h"
#include "game/cd.h"
#include "game/music_director_legacy.h"

void RequestCdTrack(s32 track) {
    AudioSession state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestTrack(&state, track);
    MusicDirectorStoreLegacyState(&state);
}

void StartCdAudio(void) {
    AudioSession state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestPlay(&state);
    MusicDirectorStoreLegacyState(&state);
}

void PauseCdAudio(void) {
    AudioSession state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestPause(&state);
    MusicDirectorStoreLegacyState(&state);
}

void ResumeCdAudio(void) {
    AudioSession state = MusicDirectorLoadLegacyState();
    MusicDirectorRequestResume(&state);
    MusicDirectorStoreLegacyState(&state);
}

void ResetCdAudioState(void) {
    AudioSession state = MusicDirectorLoadLegacyState();
    MusicDirectorReset(&state);
    MusicDirectorStoreLegacyState(&state);
}
