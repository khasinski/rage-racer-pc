#ifndef GAME_MUSIC_DIRECTOR_LEGACY_H
#define GAME_MUSIC_DIRECTOR_LEGACY_H

#include "game/music_director.h"

AudioSession MusicDirectorLoadLegacyState(void);
void MusicDirectorStoreLegacyState(const AudioSession *state);

#endif
