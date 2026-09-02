#ifndef GAME_FMV_H
#define GAME_FMV_H

#include "common.h"
typedef enum FmvPlaybackState {
    FMV_PLAYBACK_INVALID = -1,
    FMV_PLAYBACK_START,
    FMV_PLAYBACK_DECODE,
    FMV_PLAYBACK_FINISH
} FmvPlaybackState;

extern FmvPlaybackState g_FmvState;
extern s32 g_FmvStreamEnded;

void StartFmvPlayback(void);
void ReturnFromClassFmv(void);
void ReturnFromEndingFmv(void);

#endif
