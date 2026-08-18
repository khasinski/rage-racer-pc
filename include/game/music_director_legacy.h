#ifndef GAME_MUSIC_DIRECTOR_LEGACY_H
#define GAME_MUSIC_DIRECTOR_LEGACY_H

#include "game/music_director.h"

MusicDirectorState MusicDirectorLoadLegacyState(void);
void MusicDirectorStoreLegacyState(const MusicDirectorState *state);

#endif
