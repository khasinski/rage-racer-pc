#ifndef RAGE_PC_PSYQ_GPU_TYPES_H
#define RAGE_PC_PSYQ_GPU_TYPES_H

#include <sys/types.h>

#include "common.h"

typedef struct Rect {
    short x;
    short y;
    short w;
    short h;
} Rect;

typedef struct GpuRectPacked {
    u32 xy;
    short w;
    short h;
} GpuRectPacked;

typedef struct GpuTexWindow {
    u8 x;
    u8 pad1;
    u8 y;
    u8 pad3;
    short w;
    short h;
} GpuTexWindow;

typedef char GpuRectPackedSizeCheck[sizeof(GpuRectPacked) == 8 ? 1 : -1];
typedef char GpuTexWindowSizeCheck[sizeof(GpuTexWindow) == 8 ? 1 : -1];

#endif
