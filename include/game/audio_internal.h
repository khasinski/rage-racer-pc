#ifndef GAME_AUDIO_INTERNAL_H
#define GAME_AUDIO_INTERNAL_H

#include "common.h"

enum {
    BGM_PLAYABLE_TRACK_COUNT = 10,
    BGM_SHUFFLE_CAPACITY = 12,
};

extern s32 g_BgmShuffleIndex;
extern u8 g_BgmShuffleOrder[BGM_SHUFFLE_CAPACITY];

#endif
