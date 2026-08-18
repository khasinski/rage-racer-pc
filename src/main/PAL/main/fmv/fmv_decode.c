#include "common.h"
#include "game/game_input.h"
#include <stdio.h>
#include "game/fmv.h"
#include "game/asset.h"
#include "game/state.h"
#include "psyq/gpu.h"
#include "game/cd.h"
#include "game/render.h"
#include "game/fmv_decode_internal.h"
#include "game/fmv_internal.h"
#include "game/game_context.h"

void DecodeFmvFrame(void) {
    s32 value;
    u32 sector;
    u8 streamLoc[16];

    g_SceneTimer++;
    if (g_SceneTimer == 4) {
        SetDispMask(1);
    }

    DecDCTin(g_FmvDecodeContext.vlcBuffers[g_FmvDecodeContext.vlcIndex], 3);
    DecDCTout(g_FmvStripBuffers[g_FmvStripIndex], (g_FmvStripWidth * g_FmvStripHeight) / 2);

    while (PresentFmvFrame(&g_FmvDecodeContext) == -1) {
        value = StGetBackloc(streamLoc);
        printf(g_MsgFmvSector, value);
        sector = value;
        if ((g_StreamSectorCount < sector) || (value < 0)) {
            StartStreamRead(g_StreamLoc);
        } else {
            StartStreamRead(streamLoc);
        }
    }

    WaitFmvDecode(&g_FmvDecodeContext, 0);
    if (g_FmvStreamEnded == 1) {
        g_FmvState = FMV_PLAYBACK_FINISH;
    }
    if (g_GameInput.pressed & PAD_START) {
        StartCdVolumeFade(1);
        g_FmvState = FMV_PLAYBACK_FINISH;
    }
}

void EndFmv(void) {
    DecDCToutCallback(0);
    StUnSetRing();
    GameSceneSet((SceneId)g_StreamReturnScene);
    g_StreamReturnScene = g_FmvStreamEnded;
}

void InitFmvContext(volatile FmvDecodeContext *ctx, s32 width, s32 height,
                    s32 displayX, s32 displayWidth) {
    FmvDecodeContextAddress contextAddress;
    volatile u32 *word0;
    volatile u32 *word1;
    volatile u32 *word3;
    volatile u32 *word4;
    u16 half18;
    u16 half1A;
    u16 half20;
    u32 word28;
    u16 half22;

    contextAddress.volatilePointer = ctx;
    word0 = g_FmvVlcBuffer0;
    word1 = g_FmvVlcBuffer1;
    word3 = g_FmvStripBuffer0;
    word4 = g_FmvStripBuffer1;
    ctx->vlcIndex = 0;
    ctx->stripIndex = 0;
    ctx->vlcBuffers[0] = word0;
    ctx->vlcBuffers[1] = word1;
    ctx->stripBuffers[0] = word3;
    ctx->stripBuffers[1] = word4;
    half18 = g_DispEnv0X;
    ctx->displayRects[0].x = half18;
    half1A = g_DispEnv0Y;
    ctx->displayRects[0].y = half1A;
    half20 = g_DispEnv1X;
    ctx->displayRects[1].x = half20;
    word28 = g_FrameParity;
    half22 = g_DispEnv1Y;
    ctx->stripWidth = width;
    ctx->stripHeight = height;
    ctx->decodeComplete = 0;
    ctx->frameParity = word28;
    ctx->sliceHeight = 0x18;
    contextAddress.pointer->displayRects[1].y = half22;
}
