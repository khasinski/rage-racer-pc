#include <sys/types.h>

#include "common.h"
#include "psyq/gpu.h"
#include "psyq/gpu_internal.h"

/* The libgpu copy of the hardware pointers, initialised in the data
 * segment: GP0 0x1F801810, GP1 0x1F801814, DMA channel 2 (GPU) MADR/BCR/CHCR
 * 0x1F8010A0/A4/A8. */

void Gpu_WriteGp1(u_long command) {
    *g_GpuGp1 = command;
    g_GpuGp1Mirror[command / 16777216] = command;
}

u_char Gpu_GetControlMirrorByte(long index) {
    return g_GpuGp1Mirror[index];
}

long Gpu_WriteGp0Words(u_long *src, long count) {
    long i;

    *g_GpuGp1 = 0x04000000;
    for (i = count - 1; i != -1; i--) {
        *g_GpuGp0 = *src;
        src++;
    }
    return 0;
}

void Gpu_StartDmaTransfer(u_long packet) {
    *g_GpuGp1 = 0x4000002;
    *g_GpuDmaMadr = packet;
    *g_GpuDmaBcr = 0;
    *g_GpuDmaChcr = 0x1000401;
}

u_long _param(u_long index) {
    *g_GpuGp1 = index | 0x10000000;
    return *g_GpuGp0 & 0xFFFFFF;
}

long _addque(void *callback, void *data, long size) {
    return Gpu_AddQueue(callback, data, 0, size);
}
