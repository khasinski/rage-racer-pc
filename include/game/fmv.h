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

/* Select a stream and enter the FMV scene. */
void BeginIntroFmv(s32 returnScene);
void BeginClassFmv(s32 returnScene);
void BeginEndingFmv(s32 returnScene);

/* Shared scene lifecycle used by the stream-selection wrappers above. */
void BeginFmv(s32 returnScene);
void UpdateFmv(void);
void StartFmvPlayback(void);
void DecodeFmvFrame(void);
void EndFmv(void);

void ReturnFromClassFmv(void);
void ReturnFromEndingFmv(void);

#endif
