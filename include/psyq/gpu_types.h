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
    u_long xy;
    short w;
    short h;
} GpuRectPacked;

typedef struct GpuTexWindow {
    u_char x;
    u_char pad1;
    u_char y;
    u_char pad3;
    short w;
    short h;
} GpuTexWindow;

#endif
