#include "game/fmv.h"
#include "game/fmv_internal.h"

/* Carves the five FMV working buffers out of one block: two 0x28000 VLC
 * buffers, two 0x2D00 strip buffers and whatever is left as the sector ring. */
void SetupFmvBuffers(FmvWorkBuffers *buffers) {
    FmvWorkBufferAddress cursor;
    FmvWorkBufferAddress nextStrip;
    u32 step;

    cursor.buffers = buffers;
    step = 0x28000;
    g_FmvVlcBuffer0 = cursor.words;
    cursor.address += step;
    g_FmvVlcBuffer1 = cursor.words;
    cursor.address += step;
    nextStrip.address = cursor.address + 0x2D00;
    g_FmvStripBuffer0 = cursor.words;
    cursor.address += 0x5A00;
    g_FmvStripBuffer1 = nextStrip.words;
    g_FmvRingBuffer = cursor.words;
}
