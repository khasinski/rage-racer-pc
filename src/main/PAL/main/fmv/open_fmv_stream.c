#include "psyq/cd.h"
#include "game/fmv.h"
#include "game/asset.h"
#include "game/render.h"
#include "game/fmv_decode_internal.h"
#include "game/fmv_internal.h"
#include "psyq/press_internal.h"

void OpenFmvStream(s32 callback) {
    DecDCTReset(0);
    DecDCToutCallback(callback);
    StSetRing((void *)g_FmvRingBuffer, 0x20);
    StSetStream(1, 1, -1, 0, 0);
    StartStreamRead(g_StreamLoc);
}

void UploadFmvSlice(void) {
    Rect rect;
    FmvStripCursorAddress bufferCursor;
    FmvStripCursorAddress bufferAddress;
    FmvWorkBufferAddress uploadAddress;
    s32 oldBuffer;
    s32 state;
    s32 bufferIndex;
    s32 bufferAddr;
    s32 pixelCount;
    s32 next;
    s32 index;
    u16 x;
    u16 step;
    s32 signedStep;

    if (g_StInterruptPending != 0) {
        StCdInterrupt();
        g_StInterruptPending = 0;
    }

    rect.x = g_FmvUploadRectX;
    rect.y = g_FmvUploadRectY;
    rect.w = g_FmvStripWidth;
    rect.h = g_FmvStripHeight;

    bufferCursor.index = &g_FmvStripIndex;
    oldBuffer = *bufferCursor.index;
    state = *bufferCursor.index;
    x = g_FmvUploadRectX;
    step = g_FmvStripWidth;
    *bufferCursor.index = state == 0;

    index = g_FmvStripRectIndex;
    x += step;
    g_FmvUploadRectX = x;

    if ((s16)x < (g_FmvStripRects.rects[index].x +
                  g_FmvStripRects.rects[index].w)) {
        signedStep = (s16)step;
        pixelCount = signedStep * g_FmvStripHeight;
        bufferIndex = g_FmvStripIndex;
        bufferAddr = bufferIndex << 2;
        
        {
            s32 relativeAddress = bufferAddr;
            bufferAddr = bufferCursor.byteAddress + relativeAddress;
        }
        bufferAddress.byteAddress = bufferAddr;
        DecDCTout((u_long *)bufferAddress.bufferEnd[-2], pixelCount / 2);
    } else {
        g_FmvStripDone = 1;
        next = index == 0;
        g_FmvStripRectIndex = next;
        g_FmvUploadRectX = g_FmvStripRects.components[next][0];
        g_FmvUploadRectY = g_FmvStripRects.components[next][1];
    }

    uploadAddress.words = g_FmvStripBuffers[oldBuffer];
    LoadImage(&rect, uploadAddress.data);
}
