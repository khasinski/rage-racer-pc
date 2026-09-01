#ifndef GAME_FMV_H
#define GAME_FMV_H

#include "common.h"
#include "psyq/gpu.h"

typedef enum FmvPlaybackState {
    FMV_PLAYBACK_INVALID = -1,
    FMV_PLAYBACK_START,
    FMV_PLAYBACK_DECODE,
    FMV_PLAYBACK_FINISH
} FmvPlaybackState;

typedef struct FmvDecodeContext {
    volatile u32 *vlcBuffers[2];
    s32 vlcIndex;
    volatile u32 *stripBuffers[2];
    s32 stripIndex;
    struct {
        u16 x;
        u16 y;
        u16 w;
        u16 h;
    } displayRects[2];
    s32 frameParity;
    u16 stripWidth;
    u16 stripHeight;
    u16 sliceHeight;
    u16 decodedHeight;
    s32 decodeComplete;
} FmvDecodeContext;

typedef struct FmvWorkBuffers {
    volatile u32 vlc[2][0xA000];
    volatile u32 strips[2][0xB40];
    u8 ring[1];
} FmvWorkBuffers;

extern FmvDecodeContext g_FmvDecodeContext;
extern volatile u32 *g_FmvRingBuffer;
extern FmvPlaybackState g_FmvState;

void StartFmvPlayback(FmvWorkBuffers *buffers);

#endif
